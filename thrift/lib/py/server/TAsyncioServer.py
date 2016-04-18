#!/usr/bin/env python3

import asyncio
import functools
import logging
from io import BytesIO
import struct
import traceback
import warnings

from .TServer import TServer, TServerEventHandler, TConnectionContext
from thrift.Thrift import (
    TException,
    TProcessor,
)
from thrift.util.async_common import (
    AsyncClientProtocolBase,
    AsyncThriftFactory,
    FramedProtocol,
    THeaderProtocol,
    TReadOnlyBuffer,
    TReadWriteBuffer,
    WrappedTransport,
)


__all__ = [
    'ThriftAsyncServerFactory', 'ThriftClientProtocolFactory',
    'ThriftServerProtocolFactory',
]


logger = logging.getLogger(__name__)


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

    if loop is None:
        loop = asyncio.get_event_loop()

    if not isinstance(processor, TProcessor):
        try:
            processor = processor._processor_type(processor, loop=loop)
        except AttributeError:
            raise TypeError(
                "Unsupported processor type: {}".format(type(processor)),
            )

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


def ThriftClientProtocolFactory(
    client_class,
    thrift_factory=None,
    loop=None,
    timeouts=None,
    client_type=None,
):
    if not thrift_factory:
        thrift_factory = AsyncioThriftFactory(client_type)
    return functools.partial(
        ThriftHeaderClientProtocol,
        client_class,
        thrift_factory,
        loop,
        timeouts,
        client_type,
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


class SenderTransport:
    MAX_QUEUE_SIZE = 1024

    def __init__(self, trans, loop=None):
        self._loop = loop or asyncio.get_event_loop()
        self._queue = asyncio.Queue(
            maxsize=self.MAX_QUEUE_SIZE,
            loop=self._loop,
        )
        self._trans = trans
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
        if self._trans:
            self._trans.close()


class AsyncioThriftFactory(AsyncThriftFactory):

    def getSenderTransport(self, asyncio_transport, asyncio_loop):
        return SenderTransport(asyncio_transport, asyncio_loop)


class ThriftHeaderClientProtocol(AsyncClientProtocolBase):

    @asyncio.coroutine
    def message_received(self, frame, delay=0):
        tmi = TReadOnlyBuffer(frame)
        iprot = THeaderProtocol(tmi)
        (fname, mtype, seqid) = iprot.readMessageBegin()

        if delay:
            yield from asyncio.sleep(delay, loop=self.loop)
        else:
            try:
                timeout_task = self.pending_tasks.pop(seqid)
            except KeyError:
                # Task doesn't have a timeout or has already been cancelled
                # and pruned from `pending_tasks`.
                pass
            else:
                timeout_task.cancel()

        self._handle_message_received(iprot, fname, mtype, seqid)


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
        except asyncio.CancelledError:
            self.transport.close()
        except BaseException as e:
            logger.error("Exception while processing request: %s", str(e))
            logger.error(traceback.format_exc())
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
