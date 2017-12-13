from enum import Enum
from thrift.py3.types cimport Struct
from thrift.py3.types import Struct
from libcpp.memory cimport unique_ptr
from libc.stdint cimport uint32_t, uint64_t
from folly.iobuf cimport IOBuf, move, wrapBuffer
from libc.string cimport const_uchar


class Protocol(Enum):
    COMPACT = 0
    BINARY = 1
    JSON = 3


def serialize(tstruct, protocol=Protocol.COMPACT):
    assert isinstance(tstruct, Struct), "Must by a py3 thrift struct instance"
    assert isinstance(protocol, Protocol), "protocol must of type Protocol"
    cdef Struct cy_struct = <Struct> tstruct
    return cy_struct._serialize(protocol)


def deserialize(structKlass, bytes buf, protocol=Protocol.COMPACT):
    assert issubclass(structKlass, Struct), "Must by a py3 thrift struct class"
    assert isinstance(protocol, Protocol), "protocol must of type Protocol"
    cdef const_uchar* c_str = buf
    cdef uint64_t capacity = len(buf)
    cdef Struct cy_struct = <Struct> structKlass.__new__(structKlass)
    c_buf = move(wrapBuffer(c_str, capacity))
    cy_struct._deserialize(c_buf.get(), protocol)
    return cy_struct
