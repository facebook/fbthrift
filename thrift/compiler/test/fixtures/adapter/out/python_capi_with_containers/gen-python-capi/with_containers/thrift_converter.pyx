
#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from thrift.python.capi.cpp_converter cimport cpp_to_python, python_to_cpp
from libcpp.utility cimport move as cmove

cdef extern from "thrift/compiler/test/fixtures/adapter/gen-python-capi/with_containers/thrift_types_capi.h":
    cdef cppclass _fbthrift__NamespaceTag "with_containers::NamespaceTag"

cdef cAnnotationWithContainers AnnotationWithContainers_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cAnnotationWithContainers, _fbthrift__NamespaceTag](inst))

cdef object AnnotationWithContainers_from_cpp(const cAnnotationWithContainers& c_struct):
    return cpp_to_python[cAnnotationWithContainers, _fbthrift__NamespaceTag](c_struct)

cdef cMyStruct MyStruct_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cMyStruct, _fbthrift__NamespaceTag](inst))

cdef object MyStruct_from_cpp(const cMyStruct& c_struct):
    return cpp_to_python[cMyStruct, _fbthrift__NamespaceTag](c_struct)

