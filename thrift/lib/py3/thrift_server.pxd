from libc.stdint cimport uint16_t
from libcpp.string cimport string
from libcpp.memory cimport shared_ptr

cdef extern from "thrift/lib/cpp2/async/AsyncProcessor.h" namespace "apache::thrift":
  cdef cppclass cAsyncProcessorFactory "apache::thrift::AsyncProcessorFactory":
    pass

  cdef cppclass cServerInterface "apache::thrift::ServerInterface"(cAsyncProcessorFactory):
    pass

cdef extern from "thrift/lib/cpp2/server/ThriftServer.h" namespace "apache::thrift":
  cdef cppclass cThriftServer "apache::thrift::ThriftServer":
    ThriftServer() nogil except +
    void setPort(uint16_t port) nogil
    void setInterface(shared_ptr[cServerInterface]) nogil
    void serve() nogil except +
    void stop() nogil except +

cdef extern from * namespace "std":
  cdef cppclass cException "std:Exception":
      pass

cdef extern from "thrift/lib/cpp/Thrift.h" namespace "apache::thrift":
  cdef cppclass cTException "apache::thrift::TException"(cException):
    pass

cdef extern from "thrift/lib/cpp/TApplicationException.h" namespace "apache::thrift":
  cdef cppclass cTApplicationException "apache::thrift::TApplicationException"(cTException):
    cTApplicationException(string& message) nogil except +

cdef extern from "Python.h":
  ctypedef extern class builtins.Exception[object PyBaseExceptionObject]:
    pass

cdef class ServiceInterface:
    cdef shared_ptr[cServerInterface] interface_wrapper

cdef class TException(Exception):
  pass
