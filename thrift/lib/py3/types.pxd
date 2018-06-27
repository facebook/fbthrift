from folly.iobuf cimport cIOBuf, IOBuf
from libc.stdint cimport uint32_t


cdef class Struct:
    cdef IOBuf _serialize(self, proto)
    cdef uint32_t _deserialize(self, const cIOBuf* buf, proto) except? 0


cdef class Union(Struct):
    pass


cdef class BadEnum:
    cdef object _enum
    cdef readonly int value
    cdef readonly str name


cdef translate_cpp_enum_to_python(object EnumClass, int value)
