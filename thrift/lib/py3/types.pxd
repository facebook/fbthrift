from folly.iobuf cimport IOBuf
from libc.stdint cimport uint32_t


cdef class Struct:
    cdef bytes _serialize(self, proto)
    cdef uint32_t _deserialize(self, const IOBuf* buf, proto)


cdef class BadEnum:
    cdef readonly object the_enum
    cdef readonly int value
    cdef readonly str name


cdef translate_cpp_enum_to_python(object EnumClass, int value)
