#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/types/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#
cimport cython as __cython
from cpython.object cimport PyTypeObject
from libcpp.memory cimport shared_ptr, make_shared, unique_ptr
from libcpp.optional cimport optional as __optional
from libcpp.string cimport string
from libcpp cimport bool as cbool
from libcpp.iterator cimport inserter as cinserter
from libcpp.utility cimport move as cmove
from cpython cimport bool as pbool
from cython.operator cimport dereference as deref, preincrement as inc, address as ptr_address
import thrift.py3.types
from thrift.py3.types import _IsSet as _fbthrift_IsSet
from thrift.py3.types cimport make_unique
cimport thrift.py3.types
cimport thrift.py3.exceptions
cimport thrift.python.exceptions
import thrift.python.converter
from thrift.python.types import EnumMeta as __EnumMeta
from thrift.python.std_libcpp cimport sv_to_str as __sv_to_str, string_view as __cstring_view
from thrift.python.types cimport BadEnum as __BadEnum
from thrift.py3.types cimport (
    richcmp as __richcmp,
    init_unicode_from_cpp as __init_unicode_from_cpp,
    set_iter as __set_iter,
    map_iter as __map_iter,
    reference_shared_ptr as __reference_shared_ptr,
    get_field_name_by_index as __get_field_name_by_index,
    reset_field as __reset_field,
    translate_cpp_enum_to_python,
    const_pointer_cast,
    make_const_shared,
    constant_shared_ptr,
)
cimport thrift.py3.serializer as serializer
from thrift.python.protocol cimport Protocol as __Protocol
import folly.iobuf as _fbthrift_iobuf
from folly.optional cimport cOptional
from folly.memory cimport to_shared_ptr as __to_shared_ptr
from folly.range cimport Range as __cRange

import sys
from collections.abc import Sequence, Set, Mapping, Iterable
import weakref as __weakref
import builtins as _builtins
import importlib
cimport apache.thrift.fixtures.types.included.types as _apache_thrift_fixtures_types_included_types
import apache.thrift.fixtures.types.included.types as _apache_thrift_fixtures_types_included_types

import apache.thrift.fixtures.types.module.types_inplace_FBTHRIFT_ONLY_DO_NOT_USE as _fbthrift_types_inplace
empty_struct = _fbthrift_types_inplace.empty_struct
decorated_struct = _fbthrift_types_inplace.decorated_struct
ContainerStruct = _fbthrift_types_inplace.ContainerStruct
CppTypeStruct = _fbthrift_types_inplace.CppTypeStruct
VirtualStruct = _fbthrift_types_inplace.VirtualStruct
MyStructWithForwardRefEnum = _fbthrift_types_inplace.MyStructWithForwardRefEnum
TrivialNumeric = _fbthrift_types_inplace.TrivialNumeric
TrivialNestedWithDefault = _fbthrift_types_inplace.TrivialNestedWithDefault
ComplexString = _fbthrift_types_inplace.ComplexString
ComplexNestedWithDefault = _fbthrift_types_inplace.ComplexNestedWithDefault
MinPadding = _fbthrift_types_inplace.MinPadding
MinPaddingWithCustomType = _fbthrift_types_inplace.MinPaddingWithCustomType
MyStruct = _fbthrift_types_inplace.MyStruct
MyDataItem = _fbthrift_types_inplace.MyDataItem
Renaming = _fbthrift_types_inplace.Renaming
AnnotatedTypes = _fbthrift_types_inplace.AnnotatedTypes
ForwardUsageRoot = _fbthrift_types_inplace.ForwardUsageRoot
ForwardUsageStruct = _fbthrift_types_inplace.ForwardUsageStruct
ForwardUsageByRef = _fbthrift_types_inplace.ForwardUsageByRef
IncompleteMap = _fbthrift_types_inplace.IncompleteMap
IncompleteMapDep = _fbthrift_types_inplace.IncompleteMapDep
CompleteMap = _fbthrift_types_inplace.CompleteMap
CompleteMapDep = _fbthrift_types_inplace.CompleteMapDep
IncompleteList = _fbthrift_types_inplace.IncompleteList
IncompleteListDep = _fbthrift_types_inplace.IncompleteListDep
CompleteList = _fbthrift_types_inplace.CompleteList
CompleteListDep = _fbthrift_types_inplace.CompleteListDep
AdaptedList = _fbthrift_types_inplace.AdaptedList
DependentAdaptedList = _fbthrift_types_inplace.DependentAdaptedList
AllocatorAware = _fbthrift_types_inplace.AllocatorAware
AllocatorAware2 = _fbthrift_types_inplace.AllocatorAware2
TypedefStruct = _fbthrift_types_inplace.TypedefStruct
StructWithDoubleUnderscores = _fbthrift_types_inplace.StructWithDoubleUnderscores
has_bitwise_ops = _fbthrift_types_inplace.has_bitwise_ops
is_unscoped = _fbthrift_types_inplace.is_unscoped
MyForwardRefEnum = _fbthrift_types_inplace.MyForwardRefEnum
std_unordered_map__Map__i32_string = _fbthrift_types_inplace.std_unordered_map__Map__i32_string
List__i64 = _fbthrift_types_inplace.List__i64
Map__binary_i64 = _fbthrift_types_inplace.Map__binary_i64
List__i32 = _fbthrift_types_inplace.List__i32
std_list__List__i32 = _fbthrift_types_inplace.std_list__List__i32
std_deque__List__i32 = _fbthrift_types_inplace.std_deque__List__i32
folly_fbvector__List__i32 = _fbthrift_types_inplace.folly_fbvector__List__i32
folly_small_vector__List__i32 = _fbthrift_types_inplace.folly_small_vector__List__i32
folly_sorted_vector_set__Set__i32 = _fbthrift_types_inplace.folly_sorted_vector_set__Set__i32
Map__i32_string = _fbthrift_types_inplace.Map__i32_string
std_list_int32_t__List__i32 = _fbthrift_types_inplace.std_list_int32_t__List__i32
Map__string_i32 = _fbthrift_types_inplace.Map__string_i32
List__std_unordered_map__Map__i32_string = _fbthrift_types_inplace.List__std_unordered_map__Map__i32_string
Map__i32_IncompleteMapDep = _fbthrift_types_inplace.Map__i32_IncompleteMapDep
std_unordered_map__Map__i32_CompleteMapDep = _fbthrift_types_inplace.std_unordered_map__Map__i32_CompleteMapDep
_std_list__List__IncompleteListDep = _fbthrift_types_inplace._std_list__List__IncompleteListDep
folly_small_vector__List__CompleteListDep = _fbthrift_types_inplace.folly_small_vector__List__CompleteListDep
List__AdaptedListDep = _fbthrift_types_inplace.List__AdaptedListDep
List__DependentAdaptedListDep = _fbthrift_types_inplace.List__DependentAdaptedListDep
Set__i32 = _fbthrift_types_inplace.Set__i32
Map__i32_i32 = _fbthrift_types_inplace.Map__i32_i32

TBinary = bytes
IntTypedef = int
UintTypedef = int
SomeListOfTypeMap_2468 = List__std_unordered_map__Map__i32_string
TBinary_8623 = bytes
i32_9314 = int
list_i32_9187 = List__i32
map_i32_i32_9565 = Map__i32_i32
map_i32_string_1261 = Map__i32_string
set_i32_7070 = Set__i32
set_i32_7194 = folly_sorted_vector_set__Set__i32
string_5252 = str
