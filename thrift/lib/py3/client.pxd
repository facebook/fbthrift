from cpython.ref cimport PyObject
from thrift.py3.folly cimport (
    cFollyEventBase,
    cFollyExceptionWrapper,
    cFollyTry
)
from libc.stdint cimport uint16_t, uint32_t
from libcpp.memory cimport shared_ptr, unique_ptr
from libcpp.string cimport string

cdef extern from "thrift/lib/cpp/EventHandlerBase.h" \
        namespace "apache::thrift":
    cdef cppclass cTClientBase "apache::thrift::TClientBase":
        pass

cdef extern from "thrift/lib/py3/client.h" namespace "apache::thrift":
    cdef void make_py3_client[T, U](
        shared_ptr[cFollyEventBase] event_base,
        const string& host,
        const uint16_t port,
        const uint32_t connect_timeout,
        void (*callback) (PyObject*, cFollyTry[unique_ptr[U]]),
        object py_future)

    cdef unique_ptr[T] py3_get_exception[T](
        const cFollyExceptionWrapper& excepton)

cdef class EventBase:
    cdef shared_ptr[cFollyEventBase] _folly_event_base
    cdef object asyncio_loop
    cdef object executor
