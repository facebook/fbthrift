#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/interactions/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from cpython.ref cimport PyObject
from libcpp.memory cimport shared_ptr
from thrift.python.server_impl.async_processor cimport cAsyncProcessorFactory
from folly cimport cFollyExecutor


cdef extern from "thrift/compiler/test/fixtures/interactions/gen-py3/module/services_wrapper.h" namespace "::cpp2":
    shared_ptr[cAsyncProcessorFactory] cMyServiceInterface "::cpp2::MyServiceInterface"(PyObject *if_object, cFollyExecutor* Q) except *
    shared_ptr[cAsyncProcessorFactory] cFactoriesInterface "::cpp2::FactoriesInterface"(PyObject *if_object, cFollyExecutor* Q) except *
    shared_ptr[cAsyncProcessorFactory] cPerformInterface "::cpp2::PerformInterface"(PyObject *if_object, cFollyExecutor* Q) except *
    shared_ptr[cAsyncProcessorFactory] cInteractWithSharedInterface "::cpp2::InteractWithSharedInterface"(PyObject *if_object, cFollyExecutor* Q) except *
    shared_ptr[cAsyncProcessorFactory] cBoxServiceInterface "::cpp2::BoxServiceInterface"(PyObject *if_object, cFollyExecutor* Q) except *
