#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/sink/src/module.thrift
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



cdef extern from "thrift/compiler/test/fixtures/sink/gen-cpp2/module_metadata.h" namespace "apache::thrift::detail::md":
    cdef cppclass ExceptionMetadata[T]:
        @staticmethod
        void gen(__fbthrift_cThriftMetadata &metadata)
cdef extern from "thrift/compiler/test/fixtures/sink/gen-cpp2/module_metadata.h" namespace "apache::thrift::detail::md":
    cdef cppclass StructMetadata[T]:
        @staticmethod
        void gen(__fbthrift_cThriftMetadata &metadata)
cdef extern from "thrift/compiler/test/fixtures/sink/gen-cpp2/module_types_custom_protocol.h" namespace "::cpp2":

    cdef cppclass cInitialResponse "::cpp2::InitialResponse":
        cInitialResponse() except +
        cInitialResponse(const cInitialResponse&) except +
        bint operator==(cInitialResponse&)
        bint operator!=(cInitialResponse&)
        bint operator<(cInitialResponse&)
        bint operator>(cInitialResponse&)
        bint operator<=(cInitialResponse&)
        bint operator>=(cInitialResponse&)
        __field_ref[string] content_ref "content_ref" ()


    cdef cppclass cFinalResponse "::cpp2::FinalResponse":
        cFinalResponse() except +
        cFinalResponse(const cFinalResponse&) except +
        bint operator==(cFinalResponse&)
        bint operator!=(cFinalResponse&)
        bint operator<(cFinalResponse&)
        bint operator>(cFinalResponse&)
        bint operator<=(cFinalResponse&)
        bint operator>=(cFinalResponse&)
        __field_ref[string] content_ref "content_ref" ()


    cdef cppclass cSinkPayload "::cpp2::SinkPayload":
        cSinkPayload() except +
        cSinkPayload(const cSinkPayload&) except +
        bint operator==(cSinkPayload&)
        bint operator!=(cSinkPayload&)
        bint operator<(cSinkPayload&)
        bint operator>(cSinkPayload&)
        bint operator<=(cSinkPayload&)
        bint operator>=(cSinkPayload&)
        __field_ref[string] content_ref "content_ref" ()


    cdef cppclass cCompatibleWithKeywordSink "::cpp2::CompatibleWithKeywordSink":
        cCompatibleWithKeywordSink() except +
        cCompatibleWithKeywordSink(const cCompatibleWithKeywordSink&) except +
        bint operator==(cCompatibleWithKeywordSink&)
        bint operator!=(cCompatibleWithKeywordSink&)
        bint operator<(cCompatibleWithKeywordSink&)
        bint operator>(cCompatibleWithKeywordSink&)
        bint operator<=(cCompatibleWithKeywordSink&)
        bint operator>=(cCompatibleWithKeywordSink&)
        __field_ref[string] sink_ref "sink_ref" ()


    cdef cppclass cInitialException "::cpp2::InitialException"(cTException):
        cInitialException() except +
        cInitialException(const cInitialException&) except +
        bint operator==(cInitialException&)
        bint operator!=(cInitialException&)
        bint operator<(cInitialException&)
        bint operator>(cInitialException&)
        bint operator<=(cInitialException&)
        bint operator>=(cInitialException&)
        __field_ref[string] reason_ref "reason_ref" ()


    cdef cppclass cSinkException1 "::cpp2::SinkException1"(cTException):
        cSinkException1() except +
        cSinkException1(const cSinkException1&) except +
        bint operator==(cSinkException1&)
        bint operator!=(cSinkException1&)
        bint operator<(cSinkException1&)
        bint operator>(cSinkException1&)
        bint operator<=(cSinkException1&)
        bint operator>=(cSinkException1&)
        __field_ref[string] reason_ref "reason_ref" ()


    cdef cppclass cSinkException2 "::cpp2::SinkException2"(cTException):
        cSinkException2() except +
        cSinkException2(const cSinkException2&) except +
        bint operator==(cSinkException2&)
        bint operator!=(cSinkException2&)
        bint operator<(cSinkException2&)
        bint operator>(cSinkException2&)
        bint operator<=(cSinkException2&)
        bint operator>=(cSinkException2&)
        __field_ref[cint64_t] reason_ref "reason_ref" ()

