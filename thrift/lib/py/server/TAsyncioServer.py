#!/usr/bin/env python3

import asyncio
import functools
import logging
import traceback

from thrift.server.TServer import TServerEventHandler
from thrift.Thrift import (
    TException,
    TProcessor,
)
from thrift.util.async_common import (
    AsyncioRpcConnectionContext,
    FramedProtocol,
    THeaderProtocol,
    ThriftHeaderClientProtocolBase,
    TReadWriteBuffer,
    WrappedTransport,
)


__all__ = [
    'ThriftAsyncServerFactory', 'ThriftClientProtocolFactory',
    'ThriftServerProtocolFactory',
]


logger = logging.getLogger(__name__)


#
# Thrift server support
#


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


def ThriftServerProtocolFactory(processor, server_event_handler, loop=None):
    return functools.partial(
        ThriftHeaderServerProtocol, processor, server_event_handler, loop,
    )


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


#
# Thrift client support
#


def ThriftClientProtocolFactory(
    client_class,
    loop=None,
    timeouts=None,
    client_type=None,
):
    return functools.partial(
        ThriftHeaderClientProtocol,
        client_class,
        loop,
        timeouts,
        client_type,
    )


class SenderTransport(WrappedTransport):
    @asyncio.coroutine
    def _send(self):
        while True:
            msg = yield from self._queue.get()
            self._clean_producers()
            self._trans.write(msg)


class ThriftHeaderClientProtocol(ThriftHeaderClientProtocolBase):
    @asyncio.coroutine
    def timeout_task(self, fname, seqid, delay):
        yield from asyncio.sleep(delay, loop=self.loop)
        self._handle_timeout(fname, seqid)

    def wrapAsyncioTransport(self, asyncio_transport):
        return SenderTransport(asyncio_transport, self, self.loop)
