from libcpp.string cimport string


cdef extern from * namespace "std":
    cdef cppclass cException "std:Exception":
        pass

cdef extern from "thrift/lib/cpp/Thrift.h" namespace "apache::thrift":
    cdef cppclass cTException "apache::thrift::TException"(cException):
        pass

cdef extern from "thrift/lib/cpp/TApplicationException.h" \
        namespace "apache::thrift":
    cdef cppclass cTApplicationException \
            "apache::thrift::TApplicationException"(cTException):
        cTApplicationException(string& message) nogil except +

cdef extern from "Python.h":
    ctypedef extern class builtins.Exception[object PyBaseExceptionObject]:
        pass

cdef class TException(Exception):
    pass
