from libcpp.string cimport string
from libcpp cimport bool as cbool
from cpython cimport bool as pbool
from libc.stdint cimport int8_t, int16_t, int32_t, int64_t
from libcpp.memory cimport shared_ptr, unique_ptr

cdef extern from "src/gen-cpp2/module_types.h" namespace "cpp2":
    cdef cppclass cEmpty "cpp2::Empty":
        cEmpty() except +


cdef extern from "<utility>" namespace "std" nogil:
    cdef shared_ptr[cEmpty] move(unique_ptr[cEmpty])

cdef class Empty:
    cdef shared_ptr[cEmpty] c_Empty

    @staticmethod
    cdef create(shared_ptr[cEmpty] c_Empty)

