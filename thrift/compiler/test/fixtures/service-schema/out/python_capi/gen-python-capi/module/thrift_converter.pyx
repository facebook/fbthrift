
#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from thrift.python.capi.cpp_converter cimport cpp_to_python, python_to_cpp
from libcpp.utility cimport move as cmove

cdef extern from "thrift/compiler/test/fixtures/service-schema/gen-python-capi/module/thrift_types_capi.h":
    cdef cppclass _fbthrift__NamespaceTag "module::NamespaceTag"

cdef cCustomException CustomException_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cCustomException, _fbthrift__NamespaceTag](inst))

cdef object CustomException_from_cpp(const cCustomException& c_struct):
    return cpp_to_python[cCustomException, _fbthrift__NamespaceTag](c_struct)

