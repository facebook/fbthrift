
#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from thrift.python.capi.cpp_converter cimport cpp_to_python, python_to_cpp
from libcpp.utility cimport move as cmove

cdef extern from "thrift/compiler/test/fixtures/int_limits/gen-python-capi/module/thrift_types_capi.h":
    cdef cppclass _fbthrift__NamespaceTag "module::NamespaceTag"

cdef cLimits Limits_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cLimits, _fbthrift__NamespaceTag](inst))

cdef object Limits_from_cpp(const cLimits& c_struct):
    return cpp_to_python[cLimits, _fbthrift__NamespaceTag](c_struct)

