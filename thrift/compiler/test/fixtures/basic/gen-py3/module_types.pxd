from libcpp.string cimport string
from libcpp cimport bool as cbool
from cpython cimport bool as pbool
from libc.stdint cimport int8_t, int16_t, int32_t, int64_t
from libcpp.memory cimport shared_ptr, unique_ptr

cdef extern from "src/gen-cpp2/module_types.h" namespace "cpp2":
    cdef cppclass cMyStruct "cpp2::MyStruct":
        cMyStruct() except +
        int64_t MyIntField
        shared_ptr[string] MyStringField


cdef extern from "<utility>" namespace "std" nogil:
    cdef shared_ptr[cMyStruct] move(unique_ptr[cMyStruct])
    cdef shared_ptr[cMyStruct] move(shared_ptr[cMyStruct])
