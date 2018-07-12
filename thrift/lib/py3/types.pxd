from cpython.object cimport PyTypeObject
from folly.iobuf cimport cIOBuf, IOBuf
from libc.stdint cimport uint32_t

cdef extern from "":
    """
        static CYTHON_INLINE void SetMetaClass(PyTypeObject* t, PyTypeObject* m)
        {
            Py_TYPE(t) = m;
            PyType_Modified(t);
        }
    """
    void SetMetaClass(PyTypeObject* t, PyTypeObject* m)

cdef class Struct:
    cdef IOBuf _serialize(self, proto)
    cdef uint32_t _deserialize(self, const cIOBuf* buf, proto) except? 0


cdef class Union(Struct):
    pass


cdef class CompiledEnum:
    cdef readonly int value
    cdef readonly str name
    cdef object __hash
    cdef object __str
    cdef object __repr


cdef class Flag(CompiledEnum):
    pass


cdef class BadEnum:
    cdef object _enum
    cdef readonly int value
    cdef readonly str name


cdef translate_cpp_enum_to_python(object EnumClass, int value)
