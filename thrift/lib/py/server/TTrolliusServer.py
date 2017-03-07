#!/usr/bin/env python2

"""Implements what TAsyncioServer does, using the Trollius backport for
Python 2."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import functools
import logging
import trollius as asyncio
import traceback

from trollius import From, Return

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
    TReadOnlyBuffer,
    TReadWriteBuffer,
    WrappedTransport,
)


__all__ = [
    'ThriftClientProtocolFactory',
]

logger = logging.getLogger(__name__)


#
# Thrift server support
#


@asyncio.coroutine
def ThriftAsyncServerFactory(
    processor, interface=None, port=0, loop=None, nthreads=None, sock=None,
    backlog=100
):
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
    server = yield From(loop.create_server(
        pfactory,
        interface,
        port,
        sock=sock,
        backlog=backlog,
    ))

    if server.sockets:
        for socket in server.sockets:
            event_handler.preServe(socket.getsockname())

    raise Return(server)


def ThriftServerProtocolFactory(processor, server_event_handler, loop=None):
    return functools.partial(
        ThriftHeaderServerProtocol, processor, server_event_handler, loop,
    )


class ThriftHeaderServerProtocol(FramedProtocol):

    def __init__(self, processor, server_event_handler, loop=None):
        super(ThriftHeaderServerProtocol, self).__init__(loop=loop)
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
            yield From(self.processor.process(
                prot, prot, self.server_context,
            ))
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
    thrift_factory=None,
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
            msg = yield From(self._queue.get())
            self._clean_producers()
            self._trans.write(msg)


class ThriftHeaderClientProtocol(ThriftHeaderClientProtocolBase):

    @asyncio.coroutine
    def message_received(self, frame, delay=0):
        tmi = TReadOnlyBuffer(frame)
        iprot = self.THEADER_PROTOCOL_FACTORY(
            client_type=self.client_type,
        ).getProtocol(tmi)
        (fname, mtype, seqid) = iprot.readMessageBegin()

        if delay:
            yield From(asyncio.sleep(delay))
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

    def wrapAsyncioTransport(self, asyncio_transport):
        return SenderTransport(asyncio_transport, self, self.loop)
