
#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/includes/src/includes.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from libcpp.memory cimport make_shared
from cython.operator cimport dereference as deref
from thrift.py3.types cimport const_pointer_cast
cimport includes.thrift_converter as _includes_thrift_converter


cdef shared_ptr[_fbthrift_cbindings.cIncluded] Included_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cIncluded](
        _includes_thrift_converter.Included_convert_to_cpp(inst)
    )
cdef object Included_from_cpp(const shared_ptr[_fbthrift_cbindings.cIncluded]& c_struct):
    _py_struct = _includes_thrift_converter.Included_from_cpp(deref(const_pointer_cast(c_struct)))
    return _py_struct


