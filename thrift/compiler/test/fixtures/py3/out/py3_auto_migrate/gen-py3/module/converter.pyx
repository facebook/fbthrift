
#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/py3/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from libcpp.memory cimport make_shared
from cython.operator cimport dereference as deref
from thrift.py3.types cimport const_pointer_cast
cimport module.thrift_converter as _module_thrift_converter


cdef shared_ptr[_fbthrift_cbindings.cSimpleException] SimpleException_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cSimpleException](
        _module_thrift_converter.SimpleException_convert_to_cpp(inst)
    )
cdef object SimpleException_from_cpp(const shared_ptr[_fbthrift_cbindings.cSimpleException]& c_struct):
    _py_struct = _module_thrift_converter.SimpleException_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cOptionalRefStruct] OptionalRefStruct_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cOptionalRefStruct](
        _module_thrift_converter.OptionalRefStruct_convert_to_cpp(inst)
    )
cdef object OptionalRefStruct_from_cpp(const shared_ptr[_fbthrift_cbindings.cOptionalRefStruct]& c_struct):
    _py_struct = _module_thrift_converter.OptionalRefStruct_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cSimpleStruct] SimpleStruct_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cSimpleStruct](
        _module_thrift_converter.SimpleStruct_convert_to_cpp(inst)
    )
cdef object SimpleStruct_from_cpp(const shared_ptr[_fbthrift_cbindings.cSimpleStruct]& c_struct):
    _py_struct = _module_thrift_converter.SimpleStruct_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cHiddenTypeFieldsStruct] HiddenTypeFieldsStruct_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cHiddenTypeFieldsStruct](
        _module_thrift_converter.HiddenTypeFieldsStruct_convert_to_cpp(inst)
    )
cdef object HiddenTypeFieldsStruct_from_cpp(const shared_ptr[_fbthrift_cbindings.cHiddenTypeFieldsStruct]& c_struct):
    _py_struct = _module_thrift_converter.HiddenTypeFieldsStruct_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cComplexStruct] ComplexStruct_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cComplexStruct](
        _module_thrift_converter.ComplexStruct_convert_to_cpp(inst)
    )
cdef object ComplexStruct_from_cpp(const shared_ptr[_fbthrift_cbindings.cComplexStruct]& c_struct):
    _py_struct = _module_thrift_converter.ComplexStruct_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cBinaryUnion] BinaryUnion_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cBinaryUnion](
        _module_thrift_converter.BinaryUnion_convert_to_cpp(inst)
    )
cdef object BinaryUnion_from_cpp(const shared_ptr[_fbthrift_cbindings.cBinaryUnion]& c_struct):
    _py_struct = _module_thrift_converter.BinaryUnion_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cBinaryUnionStruct] BinaryUnionStruct_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cBinaryUnionStruct](
        _module_thrift_converter.BinaryUnionStruct_convert_to_cpp(inst)
    )
cdef object BinaryUnionStruct_from_cpp(const shared_ptr[_fbthrift_cbindings.cBinaryUnionStruct]& c_struct):
    _py_struct = _module_thrift_converter.BinaryUnionStruct_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cCustomFields] CustomFields_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cCustomFields](
        _module_thrift_converter.CustomFields_convert_to_cpp(inst)
    )
cdef object CustomFields_from_cpp(const shared_ptr[_fbthrift_cbindings.cCustomFields]& c_struct):
    _py_struct = _module_thrift_converter.CustomFields_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cCustomTypedefFields] CustomTypedefFields_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cCustomTypedefFields](
        _module_thrift_converter.CustomTypedefFields_convert_to_cpp(inst)
    )
cdef object CustomTypedefFields_from_cpp(const shared_ptr[_fbthrift_cbindings.cCustomTypedefFields]& c_struct):
    _py_struct = _module_thrift_converter.CustomTypedefFields_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cAdaptedTypedefFields] AdaptedTypedefFields_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cAdaptedTypedefFields](
        _module_thrift_converter.AdaptedTypedefFields_convert_to_cpp(inst)
    )
cdef object AdaptedTypedefFields_from_cpp(const shared_ptr[_fbthrift_cbindings.cAdaptedTypedefFields]& c_struct):
    _py_struct = _module_thrift_converter.AdaptedTypedefFields_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct


