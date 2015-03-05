# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

import asyncio
import functools
import logging
from io import BytesIO
import struct
import warnings

from .TServer import TServer, TServerEventHandler, TConnectionContext
from thrift.Thrift import TProcessor
from thrift.transport.TTransport import TMemoryBuffer, TTransportException
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.protocol.THeaderProtocol import (
    THeaderProtocol,
    THeaderProtocolFactory,
)


__all__ = [
    'ThriftAsyncServerFactory', 'ThriftClientProtocolFactory',
    'ThriftServerProtocolFactory',
]


logger = logging.getLogger(__name__)


@asyncio.coroutine
def ThriftAsyncServerFactory(
    processor, *, interface=None, port=0, loop=None, nthreads=None
):
    """
    ThriftAsyncServerFactory(processor) -> asyncio.Server

    asyncio.Server factory for Thrift processors. In the spirit of "there
    should be one obvious way to do it", this server only supports the new
    THeader protocol.

    If `interface` is None (the default), listens on all interfaces. `port` is
    0 by default, which makes the OS allocate the port. Enumerate the returned
    server's "sockets" attribute to know the port in this case.

    If not given, the default event loop is used. If `nthreads` is given, the
    default executor for the event loop is set to a thread pool of up to
    `nthreads`.

    Notes about the processor method handling:

    1. By default all methods are executed synchronously on the event loop.
       This can lead to poor performance if a single run takes long to process.

    2. Mark coroutines with @asyncio.coroutine if you wish to use "yield from"
       to call async services, schedule tasks with customized executors, etc.

    3. Mark methods with @run_on_thread if you wish to run them on the thread
       pool executor. Note that unless you're accessing C extensions which free
       the GIL, this is not going to win you any performance.

    Use this to initialize multiple servers asynchronously::

        loop = asyncio.get_event_loop()
        servers = [
            ThriftAsyncServerFactory(handler1, port=9090, loop=loop),
            ThriftAsyncServerFactory(handler2, port=9091, loop=loop),
        ]
        for server in servers:
            asyncio.async(server)
        try:
            loop.run_forever()   # Servers are initialized now, might be that
                                 # one will start serving requests before
                                 # others are ready to accept them.
        finally:
            for server in servers:
                server.close()
    """

    if not isinstance(processor, TProcessor):
        try:
            processor = processor._processor_type(processor)
        except AttributeError:
            raise TypeError(
                "Unsupported processor type: {}".format(type(processor)),
            )

    if loop is None:
        loop = asyncio.get_event_loop()
    if nthreads:
        from concurrent.futures import ThreadPoolExecutor
        loop.set_default_executor(
            ThreadPoolExecutor(max_workers=nthreads),
        )
    event_handler = TServerEventHandler()
    pfactory = ThriftServerProtocolFactory(processor, event_handler)
    try:
        server = yield from loop.create_server(pfactory, interface, port)
    except Exception:
        logger.exception(
            "Could not start server at %s:%d", interface or '*', port,
        )
        raise

    if server.sockets:
        for socket in server.sockets:
            event_handler.preServe(socket.getsockname())

    return server


def ThriftClientProtocolFactory(client_class):
    return functools.partial(
        ThriftHeaderClientProtocol, client_class,
    )


def ThriftServerProtocolFactory(processor, server_event_handler):
    return functools.partial(
        ThriftHeaderServerProtocol, processor, server_event_handler,
    )


class AsyncioRpcConnectionContext(TConnectionContext):

    def __init__(self, client_socket):
        self._client_socket = client_socket

    def getPeerName(self):
        return self._client_socket.getpeername()


class FramedProtocol(asyncio.Protocol):
    MAX_LENGTH = THeaderTransport.MAX_FRAME_SIZE

    def __init__(self):
        self.recvd = b""

    def data_received(self, data):
        self.recvd = self.recvd + data
        while len(self.recvd) >= 4:
            length, = struct.unpack("!I", self.recvd[:4])
            if length > self.MAX_LENGTH:
                logger.error(
                    "Frame size %d too large for THeaderProtocol",
                    length,
                )
                self.transport.close()
                return
            elif length == 0:
                logger.error("Empty frame")
                self.transport.close()
                return

            if len(self.recvd) < length + 4:
                return

            frame = self.recvd[0:4 + length]
            self.recvd = self.recvd[4 + length:]
            asyncio.async(self.message_received(frame))

    def message_received(self, frame):
        raise NotImplementedError


class SenderTransport:
    MAX_QUEUE_SIZE = 1024

    def __init__(self, trans):
        self._queue = asyncio.Queue(maxsize=self.MAX_QUEUE_SIZE)
        self._trans = trans
        asyncio.async(self._send())

    @asyncio.coroutine
    def _send(self,):
        while True:
            msg = yield from self._queue.get()
            self._trans.write(msg)

    def send_message(self, msg):
        asyncio.async(self._queue.put(msg))


class WrappedTransport(TMemoryBuffer):

    def __init__(self, trans):
        super().__init__()
        self._trans = trans

    def flush(self):
        self._trans.send_message(self._writeBuffer.getvalue())
        self._writeBuffer = BytesIO()


class WrappedTransportFactory:

    def getTransport(self, trans):
        return WrappedTransport(trans)


class ThriftHeaderClientProtocol(FramedProtocol):

    def __init__(self, client_class):
        super().__init__()
        self._client_class = client_class

    def connection_made(self, transport):
        self.transport = transport
        self.client = self._client_class(
            SenderTransport(self.transport),
            WrappedTransportFactory(),
            THeaderProtocolFactory())

    def connection_lost(self, exc):
        for fut in self.client._futures.values():
            te = TTransportException(
                type=TTransportException.END_OF_FILE,
                message="Connection closed")
            if not fut.done():
                fut.set_exception(te)

    @asyncio.coroutine
    def message_received(self, frame):
        tmi = TMemoryBuffer(frame)
        iprot = THeaderProtocol(tmi)
        (fname, mtype, rseqid) = iprot.readMessageBegin()

        method = getattr(self.client, "recv_" + fname.decode(), None)
        if method is None:
            logger.error("Method %r is not supported", method)
            self.transport.close()
        else:
            method(iprot, mtype, rseqid)


class ThriftHeaderServerProtocol(FramedProtocol):

    def __init__(self, processor, server_event_handler):
        super().__init__()
        self.processor = processor
        self.server_event_handler = server_event_handler
        self.server_context = None

    @asyncio.coroutine
    def message_received(self, frame):
        # We support the deprecated FRAMED transport for old fb303
        # clients that were otherwise failing miserably.
        client_types = {
            THeaderTransport.HEADERS_CLIENT_TYPE,
            THeaderTransport.FRAMED_DEPRECATED,
        }

        tm = TMemoryBuffer(frame)
        prot = THeaderProtocol(tm, client_types=client_types)

        try:
            yield from self.processor.process(
                prot, prot, self.server_context,
            )
            msg = tm.getvalue()
            if len(msg) > 0:
                self.transport.write(msg)
        except Exception:
            logger.exception("Exception while processing request")
            self.transport.close()

    def connection_made(self, transport):
        self.transport = transport
        socket = self.transport.get_extra_info("socket")
        if socket is not None:
            self.server_context = AsyncioRpcConnectionContext(socket)
        self.server_event_handler.newConnection(self.server_context)

    def connection_lost(self, exc):
        self.server_event_handler.connectionDestroyed(self.server_context)


class TAsyncioServer(TServer):

    """DEPRECATED. Use ThriftAsyncServerFactory instead."""

    def __init__(self, processor, port, nthreads=None, host=None):
        warnings.warn(
            'TAsyncioServer is deprecated, use ThriftAsyncServerFactory.',
            DeprecationWarning,
        )
        self.processor = self._getProcessor(processor)
        self.port = port
        self.host = host
        self.nthreads = nthreads
        self.server_event_handler = TServerEventHandler()

    def setServerEventHandler(self, handler):
        self.server_event_handler = handler

    def serve(self):
        self._start_server()

        try:
            self.loop.run_forever()
        except KeyboardInterrupt:
            logging.info("Server killed, exitting.")
        finally:
            self.server.close()
            self.loop.close()

    def _start_server(self):
        self.loop = asyncio.get_event_loop()
        if self.nthreads is not None:
            from concurrent.futures import ThreadPoolExecutor
            self.loop.set_default_executor(ThreadPoolExecutor(
                max_workers=int(self.nthreads)))
        pfactory = ThriftServerProtocolFactory(
            self.processor, self.server_event_handler,
        )
        try:
            coro = self.loop.create_server(pfactory, self.host, self.port)
            self.server = self.loop.run_until_complete(coro)
        except Exception as e:
            logging.error("Could not start server at port {}"
                          .format(self.port), e)

        if hasattr(self.server, "sockets"):
            for socket in self.server.sockets:
                self.server_event_handler.preServe(socket.getsockname())
