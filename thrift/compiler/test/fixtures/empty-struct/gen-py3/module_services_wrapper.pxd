from cpython.ref cimport PyObject
from libcpp.memory cimport shared_ptr
from py3_thrift_server cimport cServerInterface


cdef extern from "src/gen-py3/module_services_wrapper.h" namespace "cpp2":
    pass
