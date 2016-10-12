from cpython.ref cimport PyObject
from libcpp.memory cimport shared_ptr
from thrift.lib.py3.thrift_server cimport cServerInterface


cdef extern from "src/gen-py3/module_services_wrapper.h" namespace "cpp2":
    pass
