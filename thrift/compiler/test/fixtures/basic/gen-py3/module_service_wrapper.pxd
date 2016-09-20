from cpython.ref cimport PyObject
from libcpp.memory cimport shared_ptr
from cy_thrift_server cimport cServerInterface

from cy_module_services cimport cMyServiceSvIf
from cy_module_services cimport cMyServiceFastSvIf

cdef extern from "src/gen_py3/module_service_wrapper.h" namespace "cpp2":
    cdef cppclass cMyServiceWrapper "cpp2::MyService"(cMyServiceSvIf):
        pass

    shared_ptr[cServerInterface] cMyServiceInterface "cpp2::MyServiceInterface"(PyObject *if_object)
    cdef cppclass cMyServiceFastWrapper "cpp2::MyServiceFast"(cMyServiceFastSvIf):
        pass

    shared_ptr[cServerInterface] cMyServiceFastInterface "cpp2::MyServiceFastInterface"(PyObject *if_object)
