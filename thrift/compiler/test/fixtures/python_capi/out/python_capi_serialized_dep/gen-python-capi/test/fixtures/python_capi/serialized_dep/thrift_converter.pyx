
#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from thrift.python.capi.cpp_converter cimport cpp_to_python, python_to_cpp
from libcpp.utility cimport move as cmove

cdef extern from "thrift/compiler/test/fixtures/python_capi/gen-python-capi/serialized_dep/thrift_types_capi.h":
    cdef cppclass _fbthrift__NamespaceTag "test__fixtures__python_capi__serialized_dep::NamespaceTag"

cdef cSerializedStruct SerializedStruct_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cSerializedStruct, _fbthrift__NamespaceTag](inst))

cdef object SerializedStruct_from_cpp(const cSerializedStruct& c_struct):
    return cpp_to_python[cSerializedStruct, _fbthrift__NamespaceTag](c_struct)

cdef cSerializedUnion SerializedUnion_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cSerializedUnion, _fbthrift__NamespaceTag](inst))

cdef object SerializedUnion_from_cpp(const cSerializedUnion& c_struct):
    return cpp_to_python[cSerializedUnion, _fbthrift__NamespaceTag](c_struct)

cdef cSerializedError SerializedError_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cSerializedError, _fbthrift__NamespaceTag](inst))

cdef object SerializedError_from_cpp(const cSerializedError& c_struct):
    return cpp_to_python[cSerializedError, _fbthrift__NamespaceTag](c_struct)

cdef cMarshalStruct MarshalStruct_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cMarshalStruct, _fbthrift__NamespaceTag](inst))

cdef object MarshalStruct_from_cpp(const cMarshalStruct& c_struct):
    return cpp_to_python[cMarshalStruct, _fbthrift__NamespaceTag](c_struct)

cdef cMarshalUnion MarshalUnion_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cMarshalUnion, _fbthrift__NamespaceTag](inst))

cdef object MarshalUnion_from_cpp(const cMarshalUnion& c_struct):
    return cpp_to_python[cMarshalUnion, _fbthrift__NamespaceTag](c_struct)

cdef cMarshalError MarshalError_convert_to_cpp(object inst) except *:
    return cmove(python_to_cpp[cMarshalError, _fbthrift__NamespaceTag](inst))

cdef object MarshalError_from_cpp(const cMarshalError& c_struct):
    return cpp_to_python[cMarshalError, _fbthrift__NamespaceTag](c_struct)

