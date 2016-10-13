from libcpp.string cimport string
from libcpp cimport bool as cbool
from cpython cimport bool as pbool
from libc.stdint cimport int8_t, int16_t, int32_t, int64_t
from libcpp.memory cimport shared_ptr, unique_ptr

cdef extern from "src/gen-cpp2/module_types.h" namespace "py3::simple":
    cdef cppclass cSimpleStruct "py3::simple::SimpleStruct":
        cSimpleStruct() except +
        int32_t key
        int32_t value


cdef extern from "<utility>" namespace "std" nogil:
    cdef shared_ptr[cSimpleStruct] move(unique_ptr[cSimpleStruct])

cdef class SimpleStruct:
    cdef shared_ptr[cSimpleStruct] c_SimpleStruct

    @staticmethod
    cdef create(shared_ptr[cSimpleStruct] c_SimpleStruct)

