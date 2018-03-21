from folly cimport (
    cFollyExceptionWrapper,
    cFollyFuture,
    cFollyExecutor,
    cFollyTry
)
from folly.range cimport StringPiece
from libc.stdint cimport uint16_t, uint32_t
from libcpp.memory cimport shared_ptr
from libcpp.string cimport string
from libcpp.typeinfo cimport type_info
from cpython.ref cimport PyObject
from libcpp cimport bool

cdef extern from "thrift/lib/py3/client.h" namespace "thrift::py3":
    # The custome deleter is hard, so instead make cython treat it as class
    cdef cppclass cRequestChannel_ptr "thrift::py3::RequestChannel_ptr":
        bool operator bool()

    cdef cFollyFuture[cRequestChannel_ptr] createThriftChannelTCP(
        cFollyFuture[string] fut,
        const uint16_t port,
        const uint32_t connect_timeout,
    )

    cdef cFollyFuture[cRequestChannel_ptr] createThriftChannelUnix(
        StringPiece path,
        const uint32_t connect_timeout,
    )
    cdef void destroyInEventBaseThread(cRequestChannel_ptr)
    cdef shared_ptr[U] makeClientWrapper[T, U](cRequestChannel_ptr channel)

cdef extern from "<utility>" namespace "std" nogil:
    cdef cRequestChannel_ptr move(cRequestChannel_ptr)

cdef class Client:
    cdef object __weakref__
    cdef object _connect_future
    cdef object _deferred_headers
    cdef cFollyExecutor* _executor
    cdef cRequestChannel_ptr _cRequestChannel
    cdef inline _check_connect_future(self):
        ex = self._connect_future.exception()
        if ex:
            raise ex

    cdef const type_info* _typeid(self)

cdef void requestchannel_callback(
        cFollyTry[cRequestChannel_ptr]&& result,
        PyObject* userData)

cdef object get_proxy_factory()
