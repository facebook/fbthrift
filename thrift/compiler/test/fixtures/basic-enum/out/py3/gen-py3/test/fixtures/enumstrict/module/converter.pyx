
#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/basic-enum/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from libcpp.memory cimport make_shared, unique_ptr
from cython.operator cimport dereference as deref, address
from libcpp.utility cimport move as cmove
cimport test.fixtures.enumstrict.module.types as _fbthrift_ctypes
from thrift.py3.serializer cimport (
    cserialize as __cserialize,
    cdeserialize as __cdeserialize,
)
from thrift.python.protocol cimport Protocol
cimport folly.iobuf as _folly__iobuf


cdef shared_ptr[_fbthrift_cbindings.cMyStruct] MyStruct_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cMyStruct](deref(
        (<_fbthrift_ctypes.MyStruct?>inst)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE
    ))

cdef object MyStruct_from_cpp(const shared_ptr[_fbthrift_cbindings.cMyStruct]& c_struct):
    return _fbthrift_ctypes.MyStruct._create_FBTHRIFT_ONLY_DO_NOT_USE(c_struct)


