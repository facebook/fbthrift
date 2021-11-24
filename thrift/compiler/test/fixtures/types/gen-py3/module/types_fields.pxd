#
# Autogenerated by Thrift
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
from libcpp.utility cimport move as cmove
from libcpp.vector cimport vector
from libcpp.set cimport set as cset
from libcpp.map cimport map as cmap
from libcpp.unordered_map cimport unordered_map as cumap
from thrift.py3.exceptions cimport cTException
cimport folly.iobuf as _fbthrift_iobuf
cimport thrift.py3.exceptions
cimport thrift.py3.types
from thrift.py3.common cimport Protocol as __Protocol
from thrift.py3.std_libcpp cimport string_view as __cstring_view
from thrift.py3.types cimport (
    bstring,
    bytes_to_string,
    field_ref as __field_ref,
    optional_field_ref as __optional_field_ref,
    required_field_ref as __required_field_ref,
    StructFieldsSetter as __StructFieldsSetter
)
from folly.optional cimport cOptional as __cOptional

cimport include.types as _include_types

cimport module.types as _module_types



ctypedef void (*__decorated_struct_FieldsSetterFunc)(__decorated_struct_FieldsSetter, object) except *

cdef class __decorated_struct_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cdecorated_struct* _struct_cpp_obj
    cdef cumap[__cstring_view, __decorated_struct_FieldsSetterFunc] _setters

    @staticmethod
    cdef __decorated_struct_FieldsSetter _fbthrift_create(_module_types.cdecorated_struct* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *


ctypedef void (*__ContainerStruct_FieldsSetterFunc)(__ContainerStruct_FieldsSetter, object) except *

cdef class __ContainerStruct_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cContainerStruct* _struct_cpp_obj
    cdef cumap[__cstring_view, __ContainerStruct_FieldsSetterFunc] _setters

    @staticmethod
    cdef __ContainerStruct_FieldsSetter _fbthrift_create(_module_types.cContainerStruct* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *
    cdef void _set_field_2(self, _fbthrift_value) except *
    cdef void _set_field_3(self, _fbthrift_value) except *
    cdef void _set_field_4(self, _fbthrift_value) except *
    cdef void _set_field_5(self, _fbthrift_value) except *
    cdef void _set_field_6(self, _fbthrift_value) except *
    cdef void _set_field_7(self, _fbthrift_value) except *


ctypedef void (*__CppTypeStruct_FieldsSetterFunc)(__CppTypeStruct_FieldsSetter, object) except *

cdef class __CppTypeStruct_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cCppTypeStruct* _struct_cpp_obj
    cdef cumap[__cstring_view, __CppTypeStruct_FieldsSetterFunc] _setters

    @staticmethod
    cdef __CppTypeStruct_FieldsSetter _fbthrift_create(_module_types.cCppTypeStruct* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *


ctypedef void (*__VirtualStruct_FieldsSetterFunc)(__VirtualStruct_FieldsSetter, object) except *

cdef class __VirtualStruct_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cVirtualStruct* _struct_cpp_obj
    cdef cumap[__cstring_view, __VirtualStruct_FieldsSetterFunc] _setters

    @staticmethod
    cdef __VirtualStruct_FieldsSetter _fbthrift_create(_module_types.cVirtualStruct* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *


ctypedef void (*__MyStructWithForwardRefEnum_FieldsSetterFunc)(__MyStructWithForwardRefEnum_FieldsSetter, object) except *

cdef class __MyStructWithForwardRefEnum_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cMyStructWithForwardRefEnum* _struct_cpp_obj
    cdef cumap[__cstring_view, __MyStructWithForwardRefEnum_FieldsSetterFunc] _setters

    @staticmethod
    cdef __MyStructWithForwardRefEnum_FieldsSetter _fbthrift_create(_module_types.cMyStructWithForwardRefEnum* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *


ctypedef void (*__TrivialNumeric_FieldsSetterFunc)(__TrivialNumeric_FieldsSetter, object) except *

cdef class __TrivialNumeric_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cTrivialNumeric* _struct_cpp_obj
    cdef cumap[__cstring_view, __TrivialNumeric_FieldsSetterFunc] _setters

    @staticmethod
    cdef __TrivialNumeric_FieldsSetter _fbthrift_create(_module_types.cTrivialNumeric* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *


ctypedef void (*__TrivialNestedWithDefault_FieldsSetterFunc)(__TrivialNestedWithDefault_FieldsSetter, object) except *

cdef class __TrivialNestedWithDefault_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cTrivialNestedWithDefault* _struct_cpp_obj
    cdef cumap[__cstring_view, __TrivialNestedWithDefault_FieldsSetterFunc] _setters

    @staticmethod
    cdef __TrivialNestedWithDefault_FieldsSetter _fbthrift_create(_module_types.cTrivialNestedWithDefault* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *


ctypedef void (*__ComplexString_FieldsSetterFunc)(__ComplexString_FieldsSetter, object) except *

cdef class __ComplexString_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cComplexString* _struct_cpp_obj
    cdef cumap[__cstring_view, __ComplexString_FieldsSetterFunc] _setters

    @staticmethod
    cdef __ComplexString_FieldsSetter _fbthrift_create(_module_types.cComplexString* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *


ctypedef void (*__ComplexNestedWithDefault_FieldsSetterFunc)(__ComplexNestedWithDefault_FieldsSetter, object) except *

cdef class __ComplexNestedWithDefault_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cComplexNestedWithDefault* _struct_cpp_obj
    cdef cumap[__cstring_view, __ComplexNestedWithDefault_FieldsSetterFunc] _setters

    @staticmethod
    cdef __ComplexNestedWithDefault_FieldsSetter _fbthrift_create(_module_types.cComplexNestedWithDefault* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *


ctypedef void (*__MinPadding_FieldsSetterFunc)(__MinPadding_FieldsSetter, object) except *

cdef class __MinPadding_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cMinPadding* _struct_cpp_obj
    cdef cumap[__cstring_view, __MinPadding_FieldsSetterFunc] _setters

    @staticmethod
    cdef __MinPadding_FieldsSetter _fbthrift_create(_module_types.cMinPadding* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *
    cdef void _set_field_2(self, _fbthrift_value) except *
    cdef void _set_field_3(self, _fbthrift_value) except *
    cdef void _set_field_4(self, _fbthrift_value) except *


ctypedef void (*__MyStruct_FieldsSetterFunc)(__MyStruct_FieldsSetter, object) except *

cdef class __MyStruct_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cMyStruct* _struct_cpp_obj
    cdef cumap[__cstring_view, __MyStruct_FieldsSetterFunc] _setters

    @staticmethod
    cdef __MyStruct_FieldsSetter _fbthrift_create(_module_types.cMyStruct* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *
    cdef void _set_field_2(self, _fbthrift_value) except *
    cdef void _set_field_3(self, _fbthrift_value) except *


ctypedef void (*__MyDataItem_FieldsSetterFunc)(__MyDataItem_FieldsSetter, object) except *

cdef class __MyDataItem_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cMyDataItem* _struct_cpp_obj
    cdef cumap[__cstring_view, __MyDataItem_FieldsSetterFunc] _setters

    @staticmethod
    cdef __MyDataItem_FieldsSetter _fbthrift_create(_module_types.cMyDataItem* struct_cpp_obj)


ctypedef void (*__Renaming_FieldsSetterFunc)(__Renaming_FieldsSetter, object) except *

cdef class __Renaming_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cRenaming* _struct_cpp_obj
    cdef cumap[__cstring_view, __Renaming_FieldsSetterFunc] _setters

    @staticmethod
    cdef __Renaming_FieldsSetter _fbthrift_create(_module_types.cRenaming* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *


ctypedef void (*__AnnotatedTypes_FieldsSetterFunc)(__AnnotatedTypes_FieldsSetter, object) except *

cdef class __AnnotatedTypes_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cAnnotatedTypes* _struct_cpp_obj
    cdef cumap[__cstring_view, __AnnotatedTypes_FieldsSetterFunc] _setters

    @staticmethod
    cdef __AnnotatedTypes_FieldsSetter _fbthrift_create(_module_types.cAnnotatedTypes* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *


ctypedef void (*__ForwardUsageRoot_FieldsSetterFunc)(__ForwardUsageRoot_FieldsSetter, object) except *

cdef class __ForwardUsageRoot_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cForwardUsageRoot* _struct_cpp_obj
    cdef cumap[__cstring_view, __ForwardUsageRoot_FieldsSetterFunc] _setters

    @staticmethod
    cdef __ForwardUsageRoot_FieldsSetter _fbthrift_create(_module_types.cForwardUsageRoot* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *


ctypedef void (*__ForwardUsageStruct_FieldsSetterFunc)(__ForwardUsageStruct_FieldsSetter, object) except *

cdef class __ForwardUsageStruct_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cForwardUsageStruct* _struct_cpp_obj
    cdef cumap[__cstring_view, __ForwardUsageStruct_FieldsSetterFunc] _setters

    @staticmethod
    cdef __ForwardUsageStruct_FieldsSetter _fbthrift_create(_module_types.cForwardUsageStruct* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *


ctypedef void (*__ForwardUsageByRef_FieldsSetterFunc)(__ForwardUsageByRef_FieldsSetter, object) except *

cdef class __ForwardUsageByRef_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cForwardUsageByRef* _struct_cpp_obj
    cdef cumap[__cstring_view, __ForwardUsageByRef_FieldsSetterFunc] _setters

    @staticmethod
    cdef __ForwardUsageByRef_FieldsSetter _fbthrift_create(_module_types.cForwardUsageByRef* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *


ctypedef void (*__NoexceptMoveEmpty_FieldsSetterFunc)(__NoexceptMoveEmpty_FieldsSetter, object) except *

cdef class __NoexceptMoveEmpty_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cNoexceptMoveEmpty* _struct_cpp_obj
    cdef cumap[__cstring_view, __NoexceptMoveEmpty_FieldsSetterFunc] _setters

    @staticmethod
    cdef __NoexceptMoveEmpty_FieldsSetter _fbthrift_create(_module_types.cNoexceptMoveEmpty* struct_cpp_obj)


ctypedef void (*__NoexceptMoveSimpleStruct_FieldsSetterFunc)(__NoexceptMoveSimpleStruct_FieldsSetter, object) except *

cdef class __NoexceptMoveSimpleStruct_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cNoexceptMoveSimpleStruct* _struct_cpp_obj
    cdef cumap[__cstring_view, __NoexceptMoveSimpleStruct_FieldsSetterFunc] _setters

    @staticmethod
    cdef __NoexceptMoveSimpleStruct_FieldsSetter _fbthrift_create(_module_types.cNoexceptMoveSimpleStruct* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *


ctypedef void (*__NoexceptMoveComplexStruct_FieldsSetterFunc)(__NoexceptMoveComplexStruct_FieldsSetter, object) except *

cdef class __NoexceptMoveComplexStruct_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cNoexceptMoveComplexStruct* _struct_cpp_obj
    cdef cumap[__cstring_view, __NoexceptMoveComplexStruct_FieldsSetterFunc] _setters

    @staticmethod
    cdef __NoexceptMoveComplexStruct_FieldsSetter _fbthrift_create(_module_types.cNoexceptMoveComplexStruct* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *
    cdef void _set_field_2(self, _fbthrift_value) except *
    cdef void _set_field_3(self, _fbthrift_value) except *
    cdef void _set_field_4(self, _fbthrift_value) except *
    cdef void _set_field_5(self, _fbthrift_value) except *
    cdef void _set_field_6(self, _fbthrift_value) except *
    cdef void _set_field_7(self, _fbthrift_value) except *
    cdef void _set_field_8(self, _fbthrift_value) except *


ctypedef void (*__AllocatorAware_FieldsSetterFunc)(__AllocatorAware_FieldsSetter, object) except *

cdef class __AllocatorAware_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cAllocatorAware* _struct_cpp_obj
    cdef cumap[__cstring_view, __AllocatorAware_FieldsSetterFunc] _setters

    @staticmethod
    cdef __AllocatorAware_FieldsSetter _fbthrift_create(_module_types.cAllocatorAware* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *
    cdef void _set_field_2(self, _fbthrift_value) except *
    cdef void _set_field_3(self, _fbthrift_value) except *
    cdef void _set_field_4(self, _fbthrift_value) except *


ctypedef void (*__AllocatorAware2_FieldsSetterFunc)(__AllocatorAware2_FieldsSetter, object) except *

cdef class __AllocatorAware2_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cAllocatorAware2* _struct_cpp_obj
    cdef cumap[__cstring_view, __AllocatorAware2_FieldsSetterFunc] _setters

    @staticmethod
    cdef __AllocatorAware2_FieldsSetter _fbthrift_create(_module_types.cAllocatorAware2* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *


ctypedef void (*__TypedefStruct_FieldsSetterFunc)(__TypedefStruct_FieldsSetter, object) except *

cdef class __TypedefStruct_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cTypedefStruct* _struct_cpp_obj
    cdef cumap[__cstring_view, __TypedefStruct_FieldsSetterFunc] _setters

    @staticmethod
    cdef __TypedefStruct_FieldsSetter _fbthrift_create(_module_types.cTypedefStruct* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *
    cdef void _set_field_1(self, _fbthrift_value) except *
    cdef void _set_field_2(self, _fbthrift_value) except *


ctypedef void (*__StructWithDoubleUnderscores_FieldsSetterFunc)(__StructWithDoubleUnderscores_FieldsSetter, object) except *

cdef class __StructWithDoubleUnderscores_FieldsSetter(__StructFieldsSetter):
    cdef _module_types.cStructWithDoubleUnderscores* _struct_cpp_obj
    cdef cumap[__cstring_view, __StructWithDoubleUnderscores_FieldsSetterFunc] _setters

    @staticmethod
    cdef __StructWithDoubleUnderscores_FieldsSetter _fbthrift_create(_module_types.cStructWithDoubleUnderscores* struct_cpp_obj)
    cdef void _set_field_0(self, _fbthrift_value) except *

