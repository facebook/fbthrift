from enum import Enum
from thrift.py3.types cimport Struct
from thrift.py3.types import Struct
from libcpp.memory cimport unique_ptr
from folly.iobuf import IOBuf
from folly.iobuf cimport IOBuf

from thrift.py3.exceptions import Error


class Protocol(Enum):
    COMPACT = 0
    BINARY = 1
    JSON = 3
    COMPACT_JSON = 4


def serialize(tstruct, protocol=Protocol.COMPACT):
    return b''.join(serialize_iobuf(tstruct, protocol))

def serialize_iobuf(tstruct, protocol=Protocol.COMPACT):
    assert isinstance(tstruct, Struct), "Must be a py3 thrift struct instance"
    assert isinstance(protocol, Protocol), "protocol must of type Protocol"
    cdef Struct cy_struct = <Struct> tstruct
    return cy_struct._serialize(protocol)

def deserialize(structKlass, buf not None, protocol=Protocol.COMPACT):
    assert issubclass(structKlass, Struct), "Must be a py3 thrift struct class"
    assert isinstance(protocol, Protocol), "protocol must of type Protocol"
    cdef Struct cy_struct = <Struct> structKlass.__new__(structKlass)

    cdef IOBuf iobuf = buf if isinstance(buf, IOBuf) else IOBuf(buf)
    try:
        cy_struct._deserialize(iobuf._this, protocol)
        return cy_struct
    except Exception as e:
        raise Error.__new__(Error, *e.args) from None
