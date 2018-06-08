from libcpp.string cimport string
from libc.stdint cimport uint64_t
from libcpp.memory cimport unique_ptr
from libc.string cimport const_uchar

cdef extern from "folly/io/IOBuf.h" namespace "folly":
    cdef cppclass cIOBuf "folly::IOBuf":
        uint64_t length()
        const_uchar* data()

cdef extern from '<utility>' namespace 'std':
    unique_ptr[cIOBuf] move(unique_ptr[cIOBuf])


cdef extern from "folly/io/IOBuf.h" namespace "folly::IOBuf":
    unique_ptr[cIOBuf] wrapBuffer(const_uchar* buf, uint64_t capacity)
