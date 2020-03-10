# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# distutils: language=c++
from cpython.bytes cimport PyBytes_AsStringAndSize
from cpython.object cimport PyTypeObject, Py_LT, Py_EQ
from folly.iobuf cimport cIOBuf, IOBuf
from libc.stdint cimport uint32_t
from libcpp.string cimport string
from libcpp.memory cimport shared_ptr
from collections.abc import Iterable

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
    const T& default_inst[T]()

cdef class Struct:
    cdef IOBuf _serialize(self, proto)
    cdef uint32_t _deserialize(self, const cIOBuf* buf, proto) except? 0


cdef class Union(Struct):
    pass


cdef class Container:
    cdef object __hash
    cdef object __weakref__


cdef class CompiledEnum:
    cdef object __weakref__
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


cdef inline string bytes_to_string(bytes b) except*:
    cdef Py_ssize_t length
    cdef char* data
    PyBytes_AsStringAndSize(b, &data, &length)
    return move(string(data, length))  # there is a temp because string can raise


cdef inline uint32_t largest_flag(uint32_t v):
    """
    Given a 32bit flag field, this identifies the largest bit flag that v is
    composed of
    """
    v |= (v >> 1)
    v |= (v >> 2)
    v |= (v >> 4)
    v |= (v >> 8)
    v |= (v >> 16)
    return v ^ (v >> 1)


cdef extern from "thrift/lib/cpp2/FieldRef.h" namespace "apache::thrift" nogil:
    cdef cppclass optional_field_ref[T]:
        void assign "operator="(T)
        T value_unchecked()
        # Cython doesn't handle references very well, so use a different name
        # for value_unchecked in the contexts where references actually work.
        T& ref_unchecked "value_unchecked" ()

cdef extern from "thrift/lib/cpp2/OptionalField.h" namespace "apache::thrift" nogil:
    cdef cppclass DeprecatedOptionalField[T]:
        void assign "operator="(T)
        bint has_value()
        T value()
