from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import functools
import logging
import trollius as asyncio

from asyncio import From
from collections import defaultdict
from thrift.Thrift import TMessageType, TApplicationException
from thrift.transport.TTransport import TTransportException
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory
from thrift.util.async_common import (
    FramedProtocol,
    THeaderProtocol,
    TReadOnlyBuffer,
    TWriteOnlyBuffer,
    WrappedTransportFactory,
)


__all__ = [
    'ThriftClientProtocolFactory',
]

logger = logging.getLogger(__name__)


def ThriftClientProtocolFactory(client_class, loop=None, timeouts=None,
        client_type=None):
    return functools.partial(
        ThriftHeaderClientProtocol, client_class, loop, timeouts, client_type
    )


class SenderTransport(object):
    MAX_QUEUE_SIZE = 1024

    def __init__(self, trans, loop=None):
        self._queue = asyncio.Queue(maxsize=self.MAX_QUEUE_SIZE)
        self._trans = trans
        self._loop = loop or asyncio.get_event_loop()
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


class ThriftHeaderClientProtocol(FramedProtocol):
    DEFAULT_TIMEOUT = 60.0
    _exception_serializer = None

    def __init__(self, client_class,
                 loop=None,
                 timeouts=None,
                 client_type=None):
        FramedProtocol.__init__(self, loop=loop)
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
        timeout_task = asyncio.Task(
            self.message_received(serialized_exc, delay=timeout),
            loop=self.loop,
        )
        self.update_pending_tasks(seqid, timeout_task)

    @asyncio.coroutine
    def message_received(self, frame, delay=0):
        tmi = TReadOnlyBuffer(frame)
        iprot = THeaderProtocol(tmi)
        (fname, mtype, rseqid) = iprot.readMessageBegin()

        if delay:
            yield From(asyncio.sleep(delay))
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
            except (
                asyncio.futures.InvalidStateError,
                asyncio.CancelledError,
            ) as e:
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
