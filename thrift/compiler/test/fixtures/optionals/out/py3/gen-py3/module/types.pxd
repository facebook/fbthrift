#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/optionals/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from libc.stdint cimport (
    int8_t as cint8_t,
    int16_t as cint16_t,
    int32_t as cint32_t,
    int64_t as cint64_t,
    uint16_t as cuint16_t,
    uint32_t as cuint32_t,
)
from libcpp.string cimport string
from libcpp cimport bool as cbool, nullptr, nullptr_t
from cpython cimport bool as pbool
from libcpp.memory cimport shared_ptr, unique_ptr
from libcpp.vector cimport vector
from libcpp.set cimport set as cset
from libcpp.map cimport map as cmap, pair as cpair
from libcpp.unordered_map cimport unordered_map as cumap
cimport folly.iobuf as _fbthrift_iobuf
from thrift.python.exceptions cimport cTException
from thrift.py3.types cimport (
    bstring,
    field_ref as __field_ref,
    optional_field_ref as __optional_field_ref,
    required_field_ref as __required_field_ref,
    terse_field_ref as __terse_field_ref,
    union_field_ref as __union_field_ref,
    get_union_field_value as __get_union_field_value,
)
from thrift.python.common cimport cThriftMetadata as __fbthrift_cThriftMetadata
cimport thrift.py3.exceptions
cimport thrift.py3.types
from thrift.python.common cimport (
    RpcOptions as __RpcOptions,
    MetadataBox as __MetadataBox,
)
from folly.optional cimport cOptional as __cOptional


cimport module.types as _fbthrift_types
cimport module.types_fields as _fbthrift_types_fields
cimport module.cbindings as _module_cbindings

cdef extern from "thrift/compiler/test/fixtures/optionals/gen-py3/module/types.h":
  pass



cdef class Color(thrift.py3.types.Struct):
    cdef shared_ptr[_module_cbindings.cColor] _cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE
    cdef _fbthrift_types_fields.__Color_FieldsSetter _fields_setter
    cdef inline object red_impl(self)
    cdef inline object green_impl(self)
    cdef inline object blue_impl(self)
    cdef inline object alpha_impl(self)

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cColor])



cdef class Vehicle(thrift.py3.types.Struct):
    cdef shared_ptr[_module_cbindings.cVehicle] _cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE
    cdef _fbthrift_types_fields.__Vehicle_FieldsSetter _fields_setter
    cdef inline object color_impl(self)
    cdef inline object licensePlate_impl(self)
    cdef inline object description_impl(self)
    cdef inline object name_impl(self)
    cdef inline object hasAC_impl(self)
    cdef Color __fbthrift_cached_color

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cVehicle])



cdef class Person(thrift.py3.types.Struct):
    cdef shared_ptr[_module_cbindings.cPerson] _cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE
    cdef _fbthrift_types_fields.__Person_FieldsSetter _fields_setter
    cdef inline object id_impl(self)
    cdef inline object name_impl(self)
    cdef inline object age_impl(self)
    cdef inline object address_impl(self)
    cdef inline object favoriteColor_impl(self)
    cdef inline object friends_impl(self)
    cdef inline object bestFriend_impl(self)
    cdef inline object petNames_impl(self)
    cdef inline object afraidOfAnimal_impl(self)
    cdef inline object vehicles_impl(self)
    cdef Color __fbthrift_cached_favoriteColor
    cdef object __fbthrift_cached_friends
    cdef Map__Animal_string __fbthrift_cached_petNames
    cdef object __fbthrift_cached_afraidOfAnimal
    cdef object __fbthrift_cached_vehicles

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cPerson])


cdef cset[cint64_t] Set__i64__make_instance(object items) except *
cdef object Set__i64__from_cpp(const cset[cint64_t]&) except *

cdef class Map__Animal_string(thrift.py3.types.Map):
    cdef shared_ptr[cmap[_module_cbindings.cAnimal,string]] _cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE
    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[cmap[_module_cbindings.cAnimal,string]])
    cdef _check_key_type(self, key)

cdef shared_ptr[cmap[_module_cbindings.cAnimal,string]] Map__Animal_string__make_instance(object items) except *

cdef vector[_module_cbindings.cVehicle] List__Vehicle__make_instance(object items) except *
cdef object List__Vehicle__from_cpp(const vector[_module_cbindings.cVehicle]&) except *


