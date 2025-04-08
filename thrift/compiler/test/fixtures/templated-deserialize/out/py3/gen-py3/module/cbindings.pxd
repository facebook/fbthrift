#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/templated-deserialize/src/module.thrift
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



cdef extern from "thrift/compiler/test/fixtures/templated-deserialize/gen-cpp2/module_metadata.h" namespace "apache::thrift::detail::md":
    cdef cppclass EnumMetadata[T]:
        @staticmethod
        void gen(__fbthrift_cThriftMetadata &metadata)
cdef extern from "thrift/compiler/test/fixtures/templated-deserialize/gen-cpp2/module_types.h" namespace "::cpp2":
    cdef cppclass cMyEnumA "::cpp2::MyEnumA":
        pass

cdef extern from "thrift/compiler/test/fixtures/templated-deserialize/gen-cpp2/module_metadata.h" namespace "apache::thrift::detail::md":
    cdef cppclass ExceptionMetadata[T]:
        @staticmethod
        void gen(__fbthrift_cThriftMetadata &metadata)
cdef extern from "thrift/compiler/test/fixtures/templated-deserialize/gen-cpp2/module_metadata.h" namespace "apache::thrift::detail::md":
    cdef cppclass StructMetadata[T]:
        @staticmethod
        void gen(__fbthrift_cThriftMetadata &metadata)
cdef extern from "thrift/compiler/test/fixtures/templated-deserialize/gen-cpp2/module_types_custom_protocol.h" namespace "::cpp2":

    cdef cppclass cSmallStruct "::cpp2::SmallStruct":
        cSmallStruct() except +
        cSmallStruct(const cSmallStruct&) except +
        bint operator==(cSmallStruct&)
        bint operator!=(cSmallStruct&)
        bint operator<(cSmallStruct&)
        bint operator>(cSmallStruct&)
        bint operator<=(cSmallStruct&)
        bint operator>=(cSmallStruct&)
        __field_ref[cbool] small_A_ref "small_A_ref" ()
        __field_ref[cint32_t] small_B_ref "small_B_ref" ()


    cdef cppclass ccontainerStruct "::cpp2::containerStruct":
        ccontainerStruct() except +
        ccontainerStruct(const ccontainerStruct&) except +
        bint operator==(ccontainerStruct&)
        bint operator!=(ccontainerStruct&)
        bint operator<(ccontainerStruct&)
        bint operator>(ccontainerStruct&)
        bint operator<=(ccontainerStruct&)
        bint operator>=(ccontainerStruct&)
        __field_ref[cbool] fieldA_ref "fieldA_ref" ()
        __field_ref[cmap[string,cbool]] fieldB_ref "fieldB_ref" ()
        __field_ref[cset[cint32_t]] fieldC_ref "fieldC_ref" ()
        __field_ref[string] fieldD_ref "fieldD_ref" ()
        __field_ref[string] fieldE_ref "fieldE_ref" ()
        __field_ref[vector[vector[vector[cint32_t]]]] fieldF_ref "fieldF_ref" ()
        __field_ref[cmap[string,cmap[string,cmap[string,cint32_t]]]] fieldG_ref "fieldG_ref" ()
        __field_ref[vector[cset[cint32_t]]] fieldH_ref "fieldH_ref" ()
        __field_ref[cbool] fieldI_ref "fieldI_ref" ()
        __field_ref[cmap[string,vector[cint32_t]]] fieldJ_ref "fieldJ_ref" ()
        __field_ref[vector[vector[vector[vector[cint32_t]]]]] fieldK_ref "fieldK_ref" ()
        __field_ref[cset[cset[cset[cbool]]]] fieldL_ref "fieldL_ref" ()
        __field_ref[cmap[cset[vector[cint32_t]],cmap[vector[cset[string]],string]]] fieldM_ref "fieldM_ref" ()
        __field_ref[cMyEnumA] fieldQ_ref "fieldQ_ref" ()
        unique_ptr[cmap[string,cbool]] fieldR_ref "fieldR_ref" ()
        unique_ptr[cSmallStruct] fieldS_ref "fieldS_ref" ()
        shared_ptr[cSmallStruct] fieldT_ref "fieldT_ref" ()
        shared_ptr[const cSmallStruct] fieldU_ref "fieldU_ref" ()
        unique_ptr[cSmallStruct] fieldX_ref "fieldX_ref" ()

