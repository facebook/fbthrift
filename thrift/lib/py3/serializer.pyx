from thrift.py3.types cimport Struct
from thrift.py3.types import Struct
from libcpp.memory cimport unique_ptr
from folly.iobuf import IOBuf
from folly.iobuf cimport IOBuf

from thrift.py3.exceptions import Error
from thrift.py3.common import Protocol

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
