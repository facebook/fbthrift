
#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/namespace/src/emptyns.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from libcpp.memory cimport make_shared, unique_ptr
from cython.operator cimport dereference as deref, address
from libcpp.utility cimport move as cmove
cimport emptyns.types as _fbthrift_ctypes
from thrift.py3.serializer cimport (
    cserialize as __cserialize,
    cdeserialize as __cdeserialize,
)
from thrift.python.protocol cimport Protocol
cimport folly.iobuf as _folly__iobuf


cdef shared_ptr[_fbthrift_cbindings.cFoo] Foo_convert_to_cpp(object inst) except*:
    return make_shared[_fbthrift_cbindings.cFoo](deref(
        (<_fbthrift_ctypes.Foo?>inst)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE
    ))

cdef object Foo_from_cpp(const shared_ptr[_fbthrift_cbindings.cFoo]& c_struct):
    return _fbthrift_ctypes.Foo._create_FBTHRIFT_ONLY_DO_NOT_USE(c_struct)


