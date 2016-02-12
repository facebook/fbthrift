from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import functools
import logging
import trollius as asyncio

from trollius import From
from thrift.util.async_common import (
    AsyncClientProtocolBase,
    AsyncThriftFactory,
    THeaderProtocol,
    TReadOnlyBuffer,
)


__all__ = [
    'ThriftClientProtocolFactory',
]

logger = logging.getLogger(__name__)


def ThriftClientProtocolFactory(
    client_class,
    thrift_factory=None,
    loop=None,
    timeouts=None,
    client_type=None,
):
    if not thrift_factory:
        thrift_factory = TrolliusThriftFactory(client_type)
    return functools.partial(
        ThriftHeaderClientProtocol,
        client_class,
        thrift_factory,
        loop,
        timeouts,
        client_type,
    )


class SenderTransport(object):
    MAX_QUEUE_SIZE = 1024

    def __init__(self, trans, loop=None):
        self._loop = loop or asyncio.get_event_loop()
        self._queue = asyncio.Queue(
            maxsize=self.MAX_QUEUE_SIZE,
            loop=self._loop,
        )
        self._trans = trans
        self._consumer = asyncio.Task(self._send(), loop=self._loop)
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
            asyncio.Task(self._queue.put(msg), loop=self._loop),
        )

    def _clean_producers(self):
        self._producers = [
            p for p in self._producers if not p.done() and not p.cancelled()
        ]

    @asyncio.coroutine
    def _send(self):
        while True:
            msg = yield From(self._queue.get())
            self._clean_producers()
            self._trans.write(msg)

    def close(self):
        self._consumer.cancel()
        for producer in self._producers:
            if not producer.done() and not producer.cancelled():
                producer.cancel()
        if self._trans:
            self._trans.close()


class TrolliusThriftFactory(AsyncThriftFactory):

    def getSenderTransport(self, asyncio_transport, asyncio_loop):
        return SenderTransport(asyncio_transport, asyncio_loop)


class ThriftHeaderClientProtocol(AsyncClientProtocolBase):

    @asyncio.coroutine
    def message_received(self, frame, delay=0):
        tmi = TReadOnlyBuffer(frame)
        iprot = THeaderProtocol(tmi)
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
