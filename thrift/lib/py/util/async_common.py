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

from io import BytesIO
from thrift.transport.TTransport import TTransportBase
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.protocol.THeaderProtocol import THeaderProtocolFactory

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
