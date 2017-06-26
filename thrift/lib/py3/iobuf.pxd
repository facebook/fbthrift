from libcpp.string cimport string
from libc.stdint cimport uint64_t
from libcpp.memory cimport unique_ptr
from libc.string cimport const_uchar

cdef extern from "folly/io/IOBuf.h" namespace "folly":
    cdef cppclass IOBuf:
        uint64_t length()
        const_uchar* data()

cdef extern from '<utility>' namespace 'std':
    unique_ptr[IOBuf] move(unique_ptr[IOBuf])


cdef extern from "folly/io/IOBuf.h" namespace "folly::IOBuf":
    unique_ptr[IOBuf] wrapBuffer(const_uchar* buf, uint64_t capacity)
