
#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/interactions/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from libcpp.memory cimport make_shared, unique_ptr
from cython.operator cimport dereference as deref, address
from libcpp.utility cimport move as cmove
from thrift.py3.types cimport const_pointer_cast
cimport test.fixtures.interactions.module.thrift_converter as _test_fixtures_interactions_module_thrift_converter
import test.fixtures.interactions.module.types as _test_fixtures_interactions_module_types


cdef shared_ptr[_fbthrift_cbindings.cCustomException] CustomException_convert_to_cpp(object inst) except*:
    try:
        inst = inst._fbthrift__inner
    except AttributeError:
        pass

    return make_shared[_fbthrift_cbindings.cCustomException](
        _test_fixtures_interactions_module_thrift_converter.CustomException_convert_to_cpp(inst)
    )
cdef object CustomException_from_cpp(const shared_ptr[_fbthrift_cbindings.cCustomException]& c_struct):
    _py_struct = _test_fixtures_interactions_module_thrift_converter.CustomException_from_cpp(deref(const_pointer_cast(c_struct)))
    _py_struct = _test_fixtures_interactions_module_types.CustomException.from_python(_py_struct)
    return _py_struct

cdef shared_ptr[_fbthrift_cbindings.cShouldBeBoxed] ShouldBeBoxed_convert_to_cpp(object inst) except*:
    try:
        inst = inst._fbthrift__inner
    except AttributeError:
        pass

    return make_shared[_fbthrift_cbindings.cShouldBeBoxed](
        _test_fixtures_interactions_module_thrift_converter.ShouldBeBoxed_convert_to_cpp(inst)
    )
cdef object ShouldBeBoxed_from_cpp(const shared_ptr[_fbthrift_cbindings.cShouldBeBoxed]& c_struct):
    _py_struct = _test_fixtures_interactions_module_thrift_converter.ShouldBeBoxed_from_cpp(deref(const_pointer_cast(c_struct)))
    _py_struct = _test_fixtures_interactions_module_types.ShouldBeBoxed.from_python(_py_struct)
    return _py_struct


