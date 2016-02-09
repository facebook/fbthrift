# @lint-avoid-pyflakes2
# @lint-avoid-python-3-compatibility-imports

import asyncio
from collections import defaultdict
import functools
import logging
from io import BytesIO
import struct
import warnings

from .TServer import TServer, TServerEventHandler, TConnectionContext
from thrift.Thrift import (
    TApplicationException,
    TException,
    TMessageType,
    TProcessor,
)
from thrift.transport.TTransport import TTransportBase, TTransportException
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.protocol.THeaderProtocol import (
    THeaderProtocolFactory,
)


__all__ = [
    'ThriftAsyncServerFactory', 'ThriftClientProtocolFactory',
    'ThriftServerProtocolFactory',
]


logger = logging.getLogger(__name__)


# We support the deprecated FRAMED transport for old fb303
# clients that were otherwise failing miserably.
THEADER_CLIENT_TYPES = {
    THeaderTransport.HEADERS_CLIENT_TYPE,
    THeaderTransport.FRAMED_DEPRECATED,
}
_default_thpfactory = THeaderProtocolFactory(client_types=THEADER_CLIENT_TYPES)
THeaderProtocol = _default_thpfactory.getProtocol


@asyncio.coroutine
def ThriftAsyncServerFactory(
    processor, *, interface=None, port=0, loop=None, nthreads=None, sock=None,
    backlog=100
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
        loop.run_until_complete(asyncio.wait(servers))
        try:
            loop.run_forever()   # Servers are initialized now
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
    pfactory = ThriftServerProtocolFactory(processor, event_handler, loop)
    server = yield from loop.create_server(
        pfactory,
        interface,
        port,
        sock=sock,
        backlog=backlog,
    )

    if server.sockets:
        for socket in server.sockets:
            event_handler.preServe(socket.getsockname())

    return server


def ThriftClientProtocolFactory(client_class, loop=None, timeouts=None,
        client_type=None):
    return functools.partial(
        ThriftHeaderClientProtocol, client_class, loop, timeouts, client_type
    )


def ThriftServerProtocolFactory(processor, server_event_handler, loop=None):
    return functools.partial(
        ThriftHeaderServerProtocol, processor, server_event_handler, loop,
    )


class AsyncioRpcConnectionContext(TConnectionContext):

    def __init__(self, client_socket):
        self._client_socket = client_socket

    def getPeerName(self):
        return self._client_socket.getpeername()


class FramedProtocol(asyncio.Protocol):
    MAX_LENGTH = THeaderTransport.MAX_FRAME_SIZE

    def __init__(self, loop=None):
        self.recvd = b""
        self.loop = loop or asyncio.get_event_loop()

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
            self.loop.create_task(self.message_received(frame))

    @asyncio.coroutine
    def message_received(self, frame):
        raise NotImplementedError


class SenderTransport:
    MAX_QUEUE_SIZE = 1024

    def __init__(self, trans, loop=None):
        self._queue = asyncio.Queue(maxsize=self.MAX_QUEUE_SIZE)
        self._trans = trans
        self._loop = loop or asyncio.get_event_loop()
        self._consumer = self._loop.create_task(self._send())
        self._producers = []

    def __del__(self):
        if not self._consumer.done() or not self._consumer.cancelled():
            logger.debug(
                'SenderTransport did not finish properly'
                ' as the consumer asyncio.Task is still pending.'
                ' Please make sure to call .close() on this object.'
            )

    def send_message(self, msg):
        self._producers.append(
            self._loop.create_task(self._queue.put(msg)),
        )

    def _clean_producers(self):
        self._producers = [
            p for p in self._producers if not p.done() and not p.cancelled()
        ]

    @asyncio.coroutine
    def _send(self):
        while True:
            msg = yield from self._queue.get()
            self._clean_producers()
            self._trans.write(msg)

    def close(self):
        self._consumer.cancel()
        for producer in self._producers:
            if not producer.done() and not producer.cancelled():
                producer.cancel()


class TReadOnlyBuffer(TTransportBase):
    """Leaner version of TMemoryBuffer that is resettable."""

    def __init__(self, value=b""):
        self._open = True
        self._value = value
        self.reset()

    def isOpen(self):
        return self._open

    def close(self):
        self._io.close()
        self._open = False

    def read(self, sz):
        return self._io.read(sz)

    def write(self, buf):
        raise PermissionError("This is a read-only buffer")

    def reset(self):
        self._io = BytesIO(self._value)


class TWriteOnlyBuffer(TTransportBase):
    """Leaner version of TMemoryBuffer that is resettable."""

    def __init__(self):
        self._open = True
        self.reset()

    def isOpen(self):
        return self._open

    def close(self):
        self._io.close()
        self._open = False

    def read(self, sz):
        raise EOFError("This is a write-only buffer")

    def write(self, buf):
        self._io.write(buf)

    def getvalue(self):
        return self._io.getvalue()

    def reset(self):
        self._io = BytesIO()


class TReadWriteBuffer(TTransportBase):
    def __init__(self, value=b""):
        self._read_io = TReadOnlyBuffer(value=value)
        self._write_io = TWriteOnlyBuffer()
        self.read = self._read_io.read
        self.write = self._write_io.write
        self.getvalue = self._write_io.getvalue
        self.reset()

    def isOpen(self):
        return self._read_io._open and self._write_io._open

    def close(self):
        self._read_io.close()
        self._write_io.close()

    def reset(self):
        self._read_io.reset()
        self._write_io.reset()

    # Note: read()/write()/getvalue() methods are bound in __init__().


class WrappedTransport(TWriteOnlyBuffer):

    def __init__(self, trans, proto):
        super().__init__()
        self._trans = trans
        self._proto = proto

    def __del__(self):
        if self.isOpen():
            logger.debug(
                'A WrappedTransport object should not be garbage collected'
                ' while the transport channel is open.',
            )

    def flush(self):
        msg = self.getvalue()
        tmi = TReadOnlyBuffer(msg)
        iprot = THeaderProtocol(tmi)
        fname, mtype, seqid = iprot.readMessageBegin()
        fname = fname.decode()
        self._proto.schedule_timeout(fname, seqid)
        self._trans.send_message(msg)
        self.reset()

    def close(self):
        super().close()
        self._trans.close()


class WrappedTransportFactory:
    def __init__(self, proto):
        self._proto = proto

    def getTransport(self, trans):
        return WrappedTransport(trans, self._proto)


class ThriftHeaderClientProtocol(FramedProtocol):
    DEFAULT_TIMEOUT = 60.0
    _exception_serializer = None

    def __init__(self, client_class,
                 loop=None,
                 timeouts=None,
                 client_type=None):
        super().__init__(loop=loop)
        self._client_class = client_class
        self.client = None
        self.transport = None
        if timeouts is None:
            timeouts = {}
        default_timeout = timeouts.get('') or self.DEFAULT_TIMEOUT
        self.timeouts = defaultdict(lambda: default_timeout)
        self.timeouts.update(timeouts)
        self.pending_tasks = {}
        self.client_type = client_type

    def connection_made(self, transport):
        assert self.transport is None, "Transport already instantiated here."
        assert self.client is None, "Client already instantiated here."
        # asyncio.Transport
        self.transport = transport
        # Thrift transport
        self.thrift_transport = SenderTransport(self.transport, self.loop)
        self.client = self._client_class(
            self.thrift_transport,
            WrappedTransportFactory(self),
            THeaderProtocolFactory(client_type=self.client_type))

    def connection_lost(self, exc):
        for fut in self.client._futures.values():
            te = TTransportException(
                type=TTransportException.END_OF_FILE,
                message="Connection closed")
            if not fut.done():
                fut.set_exception(te)

    def update_pending_tasks(self, seqid, task):
        no_longer_pending = [
            _seqid for _seqid, _task in self.pending_tasks.items()
            if _task.done() or _task.cancelled()
        ]
        for _seqid in no_longer_pending:
            del self.pending_tasks[_seqid]
        assert seqid not in self.pending_tasks, (
            "seqid already pending for timeout"
        )
        self.pending_tasks[seqid] = task

    def schedule_timeout(self, fname, seqid):
        timeout = self.timeouts[fname]
        if not timeout:
            return

        exc = TApplicationException(
            TApplicationException.TIMEOUT, "Call to {} timed out".format(fname)
        )
        serialized_exc = self.serialize_texception(fname, seqid, exc)
        timeout_task = self.loop.create_task(
            self.message_received(serialized_exc, delay=timeout),
        )
        self.update_pending_tasks(seqid, timeout_task)

    @asyncio.coroutine
    def message_received(self, frame, delay=0):
        tmi = TReadOnlyBuffer(frame)
        iprot = THeaderProtocol(tmi)
        (fname, mtype, rseqid) = iprot.readMessageBegin()

        if delay:
            yield from asyncio.sleep(delay)
        else:
            try:
                timeout_task = self.pending_tasks.pop(rseqid)
            except KeyError:
                # Task doesn't have a timeout or has already been cancelled
                # and pruned from `pending_tasks`.
                pass
            else:
                timeout_task.cancel()

        method = getattr(self.client, "recv_" + fname.decode(), None)
        if method is None:
            logger.error("Method %r is not supported", fname)
            self.transport.abort()
        else:
            try:
                method(iprot, mtype, rseqid)
            except (asyncio.futures.InvalidStateError, asyncio.CancelledError) as e:
                logger.warning("Method %r cancelled: %s", fname, str(e))

    def close(self):
        for task in self.pending_tasks.values():
            if not task.done() and not task.cancelled():
                task.cancel()
        self.transport.abort()
        self.thrift_transport.close()

    @classmethod
    def serialize_texception(cls, fname, seqid, exception):
        """This saves us a bit of processing time for timeout handling by
        reusing the Thrift structs involved in exception serialization.

        NOTE: this is not thread-safe nor is it meant to be.
        """
        # the serializer is a singleton
        if cls._exception_serializer is None:
            buffer = TWriteOnlyBuffer()
            transport = THeaderTransport(buffer)
            cls._exception_serializer = THeaderProtocol(transport)
        else:
            transport = cls._exception_serializer.trans
            buffer = transport.getTransport()
            buffer.reset()

        serializer = cls._exception_serializer
        serializer.writeMessageBegin(fname, TMessageType.EXCEPTION, seqid)
        exception.write(serializer)
        serializer.writeMessageEnd()
        serializer.trans.flush()
        return buffer.getvalue()


class ThriftHeaderServerProtocol(FramedProtocol):

    def __init__(self, processor, server_event_handler, loop=None):
        super().__init__(loop=loop)
        self.processor = processor
        self.server_event_handler = server_event_handler
        self.server_context = None

    @asyncio.coroutine
    def message_received(self, frame):
        # Note: we are using a single `prot` for in and out so that
        # we can support legacy clients that only understand FRAMED.
        # The discovery of what the client supports happens in iprot's
        # transport so we have to reuse a single one here.
        buf = TReadWriteBuffer(frame)
        prot = THeaderProtocol(buf)

        try:
            yield from self.processor.process(
                prot, prot, self.server_context,
            )
            msg = buf.getvalue()
            if len(msg) > 0:
                self.transport.write(msg)
        except TException as e:
            logger.warning("TException while processing request: %s", str(e))
            msg = buf.getvalue()
            if len(msg) > 0:
                self.transport.write(msg)
        except BaseException as e:
            logger.error("Exception while processing request: %s", str(e))
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
            logging.info("Server killed, exiting.")
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
            self.processor, self.server_event_handler, self.loop,
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
