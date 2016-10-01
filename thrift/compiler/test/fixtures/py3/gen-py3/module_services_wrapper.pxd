from cpython.ref cimport PyObject
from libcpp.memory cimport shared_ptr
from py3_thrift_server cimport cServerInterface

from module_services cimport cSimpleServiceSvIf

cdef extern from "src/gen-py3/module_services_wrapper.h" namespace "py3::simple":
    cdef cppclass cSimpleServiceWrapper "py3::simple::SimpleService"(cSimpleServiceSvIf):
        pass

    shared_ptr[cServerInterface] cSimpleServiceInterface "py3::simple::SimpleServiceInterface"(PyObject *if_object)
