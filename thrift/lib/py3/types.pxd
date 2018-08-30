from cpython.object cimport PyTypeObject, Py_LT, Py_EQ
from folly.iobuf cimport cIOBuf, IOBuf
from libc.stdint cimport uint32_t
from libcpp.string cimport string
from libcpp.memory cimport shared_ptr
from collections import Iterable

cdef extern from "":
    """
        static CYTHON_INLINE void SetMetaClass(PyTypeObject* t, PyTypeObject* m)
        {
            Py_TYPE(t) = m;
            PyType_Modified(t);
        }
    """
    void SetMetaClass(PyTypeObject* t, PyTypeObject* m)

cdef extern from "thrift/lib/py3/types.h" namespace "thrift::py3" nogil:
    shared_ptr[T] constant_shared_ptr[T](T)

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


# For cpp.type'd binary values we need a "string" that cython doesn't think
# is a string (so it doesn't generate all the string stuff)
cdef extern from "<string>" namespace "std" nogil:
    cdef cppclass bstring "std::basic_string<char>":
        bstring(string&) except +
        const char* data()
        size_t size()
        size_t length()


cdef extern from "<utility>" namespace "std" nogil:
    cdef string move(string)


cdef inline object list_compare(object first, object second, int op):
    """ Take either Py_EQ or Py_LT, everything else is derived """
    if not (isinstance(first, Iterable) and isinstance(second, Iterable)):
        if op == Py_EQ:
            return False
        else:
            return NotImplemented

    if op == Py_EQ:
        if len(first) != len(second):
            return False

    for x, y in zip(first, second):
        if x != y:
            if op == Py_LT:
                return x < y
            else:
                return False

    if op == Py_LT:
        return len(first) < len(second)
    return True
