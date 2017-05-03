from libc.stdint cimport uint16_t
from libcpp.string cimport string
from libcpp.memory cimport shared_ptr, unique_ptr
from libcpp.list cimport list as clist

cdef extern from "thrift/lib/cpp2/async/AsyncProcessor.h" \
        namespace "apache::thrift":
    cdef cppclass cAsyncProcessorFactory \
            "apache::thrift::AsyncProcessorFactory":
        pass

    cdef cppclass cServerInterface \
            "apache::thrift::ServerInterface"(cAsyncProcessorFactory):
        pass

cdef extern from "thrift/lib/cpp2/server/ThriftServer.h" \
        namespace "apache::thrift":

    cdef cppclass cSSLPolicy "apache::thrift::SSLPolicy":
        bint operator==(cSSLPolicy&)
    cSSLPolicy SSLPolicy__DISABLED "apache::thrift::SSLPolicy::DISABLED"
    cSSLPolicy SSLPolicy__PERMITTED "apache::thrift::SSLPolicy::PERMITTED"
    cSSLPolicy SSLPolicy__REQUIRED "apache::thrift::SSLPolicy::REQUIRED"

    cdef cppclass cThriftServer "apache::thrift::ThriftServer":
        ThriftServer() nogil except +
        void setPort(uint16_t port) nogil
        void setInterface(shared_ptr[cServerInterface]) nogil
        void serve() nogil except +
        void stop() nogil except +
        void setSSLPolicy(cSSLPolicy policy) nogil


cdef class ServiceInterface:
    cdef shared_ptr[cServerInterface] interface_wrapper


cdef class ThriftServer:
    cdef unique_ptr[cThriftServer] server
    cdef ServiceInterface handler
    cdef object loop
