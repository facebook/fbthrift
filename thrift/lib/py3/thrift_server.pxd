from libc.stdint cimport uint16_t
from libcpp.string cimport string
from libcpp.memory cimport shared_ptr

cdef extern from "thrift/lib/cpp2/async/AsyncProcessor.h" namespace 'apache::thrift':
  cdef cppclass cAsyncProcessorFactory "apache::thrift::AsyncProcessorFactory":
    pass

  cdef cppclass cServerInterface "apache::thrift::ServerInterface"(cAsyncProcessorFactory):
    pass

cdef extern from "thrift/lib/cpp2/server/ThriftServer.h" namespace 'apache::thrift':
  cdef cppclass cThriftServer "apache::thrift::ThriftServer":
    ThriftServer() nogil except +
    void setPort(uint16_t port) nogil
    void setInterface(shared_ptr[cServerInterface]) nogil
    void serve() nogil
    void stop() nogil

cdef extern from "thrift/lib/cpp/TApplicationException.h" namespace 'apache::thrift':
  cdef cppclass cTApplicationException "apache::thrift::TApplicationException":
    cTApplicationException(string& message) nogil except +

cdef class ServiceInterface:
    cdef shared_ptr[cServerInterface] interface_wrapper
