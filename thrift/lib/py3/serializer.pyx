from thrift.py3.types cimport Struct
from thrift.py3.types import Struct
from libcpp.memory cimport unique_ptr
from folly.iobuf import IOBuf
from folly.iobuf cimport IOBuf
import folly.iobuf as _iobuf
cimport folly.iobuf as _iobuf

from thrift.py3.exceptions import Error
from thrift.py3.common import Protocol
from thrift.py3.common cimport Protocol2PROTOCOL_TYPES


def serialize(tstruct, protocol=Protocol.COMPACT):
    return b''.join(serialize_iobuf(tstruct, protocol))


def serialize_iobuf(Struct tstruct not None, protocol=Protocol.COMPACT):
    if not isinstance(protocol, Protocol):
        raise TypeError(f"{protocol} must of type Protocol")
    cdef Struct cy_struct = <Struct> tstruct
    return cy_struct._serialize(protocol)


def deserialize(structKlass, buf not None, protocol=Protocol.COMPACT):
    if not issubclass(structKlass, Struct):
        raise TypeError(f"{structKlass} Must be a py3 thrift struct class")
    if not isinstance(protocol, Protocol):
        raise TypeError(f"{protocol} must of type Protocol")
    cdef Struct cy_struct = <Struct> structKlass.__new__(structKlass)

    cdef IOBuf iobuf = buf if isinstance(buf, IOBuf) else IOBuf(buf)
    try:
        cy_struct._deserialize(iobuf._this, protocol)
        return cy_struct
    except Exception as e:
        raise Error.__new__(Error, *e.args) from None


def serialize_with_header(tstruct, protocol=Protocol.COMPACT, transform=Transform.NONE):
    return b''.join(serialize_with_header_iobuf(tstruct, protocol, transform))

def serialize_with_header_iobuf(Struct tstruct not None, protocol=Protocol.COMPACT, Transform transform=Transform.NONE):
    cdef cTHeader header
    cdef map[string, string] pheaders
    cdef IOBuf buf = <IOBuf>serialize_iobuf(tstruct, protocol)
    header.setProtocolId(Protocol2PROTOCOL_TYPES(protocol))
    if transform is not Transform.NONE:
        header.setTransform(transform)
    return _iobuf.from_unique_ptr(header.addHeader(_iobuf.move(buf._ours), pheaders))


def deserialize_from_header(structKlass, buf not None):
    # Clone because we will take the guts.
    cdef IOBuf iobuf = buf.clone() if isinstance(buf, IOBuf) else IOBuf(buf)
    cdef _iobuf.cIOBufQueue queue = _iobuf.cIOBufQueue(_iobuf.cacheChainLength())
    queue.append(_iobuf.move(iobuf._ours))
    cdef cTHeader header
    cdef map[string, string] pheaders
    cdef size_t needed = 0
    cbuf = header.removeHeader(&queue, needed, pheaders)
    protoid = <PROTOCOL_TYPES>header.getProtocolId()
    if protoid == PROTOCOL_TYPES.T_COMPACT_PROTOCOL:
        protocol = Protocol.COMPACT
    elif protoid == PROTOCOL_TYPES.T_BINARY_PROTOCOL:
        protocol = Protocol.BINARY
    elif protoid == PROTOCOL_TYPES.T_SIMPLE_JSON_PROTOCOL:
        protocol = Protocol.JSON
    elif protoid == PROTOCOL_TYPES.T_JSON_PROTOCOL:
        protocol = Protocol.COMPACT_JSON
    else:
        raise ValueError(f"unknown protocol id: {protoid} in header")
    return deserialize(structKlass, _iobuf.from_unique_ptr(_iobuf.move(cbuf)), protocol=protocol)

