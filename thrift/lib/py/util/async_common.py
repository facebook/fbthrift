from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

try:
    import asyncio
except ImportError:
    import trollius as asyncio

import six
import logging
import struct

from collections import defaultdict
from io import BytesIO
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory
from thrift.transport.TTransport import (
    TTransportBase,
    TTransportException,
)
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.Thrift import (
    TApplicationException,
    TMessageType,
)

# We support the deprecated FRAMED transport for old fb303
# clients that were otherwise failing miserably.
THEADER_CLIENT_TYPES = {
    THeaderTransport.HEADERS_CLIENT_TYPE,
    THeaderTransport.FRAMED_DEPRECATED,
}
_default_thpfactory = THeaderProtocolFactory(client_types=THEADER_CLIENT_TYPES)
THeaderProtocol = _default_thpfactory.getProtocol

logger = logging.getLogger(__name__)


if six.PY2:
    class PermissionError(IOError):
        pass


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
            asyncio.Task(self.message_received(frame), loop=self.loop)

    @asyncio.coroutine
    def message_received(self, frame):
        raise NotImplementedError


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
        super(WrappedTransport, self).__init__()
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
        super(WrappedTransport, self).close()
        self._trans.close()


class WrappedTransportFactory(object):
    def __init__(self, proto):
        self._proto = proto

    def getTransport(self, trans):
        return WrappedTransport(trans, self._proto)


class AsyncThriftFactory(object):

    def __init__(self, client_type=None):
        super(AsyncThriftFactory, self).__init__()
        self.client_type = client_type

    def getSenderTransport(self, asyncio_transport, asyncio_loop):
        raise NotImplementedError

    def getTransport(self, asyncio_transport, asyncio_loop, client_protocol):
        '''
        Given asyncio.Transport and asyncio.BaseEventLoop instances,
        this method should create a Thrift transport object that is able
        to interact with the asyncio transport one using the given event loop.
        '''
        #FIXME(dresende): let's avoid a dependency that WrappedTransport
        # has on client_protocol.
        sender_transport = self.getSenderTransport(
            asyncio_transport,
            asyncio_loop,
        )
        return WrappedTransport(sender_transport, client_protocol)

    def getProtocol(self, thrift_transport):
        '''
        Given a Thrift transport instance, which is compatible with
        asyncio.Transport, this method should create a Thrift protocol object.
        '''
        return THeaderProtocolFactory(client_type=self.client_type).getProtocol(
            thrift_transport,
        )


class AsyncClientProtocolBase(FramedProtocol):
    DEFAULT_TIMEOUT = 60.0
    _exception_serializer = None

    def __init__(
        self,
        client_class,
        thrift_factory,
        loop=None,
        timeouts=None,
        client_type=None,
    ):
        super(AsyncClientProtocolBase, self).__init__(loop=loop)
        self._client_class = client_class
        self.thrift_factory = thrift_factory
        self.client = None
        if timeouts is None:
            timeouts = {}
        default_timeout = timeouts.get('') or self.DEFAULT_TIMEOUT
        self.timeouts = defaultdict(lambda: default_timeout)
        self.timeouts.update(timeouts)
        self.pending_tasks = {}
        self.client_type = client_type
        self.thrift_transport = None

    def _handle_message_received(self, iprot, fname, mtype, seqid):
        method = getattr(self.client, "recv_" + fname.decode(), None)
        if method is None:
            logger.error("Method %r is not supported", fname)
            self.abort()
        else:
            try:
                method(iprot, mtype, seqid)
            except (
                asyncio.futures.InvalidStateError,
                asyncio.CancelledError,
            ) as e:
                logger.warning("Method %r cancelled: %s", fname, str(e))

    def connection_made(self, transport):
        assert self.thrift_transport is None, (
            "Thrift transport already instantiated here."
        )
        assert self.client is None, "Client already instantiated here."
        # transport is an asyncio.Transport instance
        self.thrift_transport = self.thrift_factory.getTransport(
            transport,
            self.loop,
            self,
        )
        thrift_protocol = self.thrift_factory.getProtocol(self.thrift_transport)
        self.client = self._client_class(thrift_protocol, self.loop)

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

    def close(self):
        for task in self.pending_tasks.values():
            if not task.done() and not task.cancelled():
                task.cancel()
        if self.thrift_transport:
            self.thrift_transport.close()

    def abort(self):
        if self.thrift_transport:
            self.thrift_transport.abort()

    @classmethod
    def serialize_texception(cls, fname, seqid, exception):
        """This saves us a bit of processing time for timeout handling by
        reusing the Thrift structs involved in exception serialization.

        NOTE: this is not thread-safe nor it is meant to be.
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
