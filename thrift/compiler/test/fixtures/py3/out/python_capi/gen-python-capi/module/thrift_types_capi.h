
/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT
 *  @generated
 *
 */

#pragma once

#include <thrift/lib/python/capi/constructor.h>
#include <thrift/lib/python/capi/extractor.h>

#include <thrift/compiler/test/fixtures/py3/gen-cpp2/module_types.h>

namespace apache {
namespace thrift {
namespace python {
namespace capi {
template <>
struct Extractor<::py3::simple::SimpleException>
    : public BaseExtractor<::py3::simple::SimpleException> {
  static const bool kUsingMarshal = true;
  ExtractorResult<::py3::simple::SimpleException> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::SimpleException>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::SimpleException>> {
  ExtractorResult<::py3::simple::SimpleException> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::SimpleException>
    : public BaseConstructor<::py3::simple::SimpleException> {
  static const bool kUsingMarshal = true;
  PyObject* operator()(const ::py3::simple::SimpleException& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::SimpleException>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::SimpleException>> {
  PyObject* operator()(const ::py3::simple::SimpleException& val);
};

template <>
struct Extractor<::py3::simple::OptionalRefStruct>
    : public BaseExtractor<::py3::simple::OptionalRefStruct> {
  static const bool kUsingMarshal = true;
  ExtractorResult<::py3::simple::OptionalRefStruct> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::OptionalRefStruct>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::OptionalRefStruct>> {
  ExtractorResult<::py3::simple::OptionalRefStruct> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::OptionalRefStruct>
    : public BaseConstructor<::py3::simple::OptionalRefStruct> {
  static const bool kUsingMarshal = true;
  PyObject* operator()(const ::py3::simple::OptionalRefStruct& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::OptionalRefStruct>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::OptionalRefStruct>> {
  PyObject* operator()(const ::py3::simple::OptionalRefStruct& val);
};

template <>
struct Extractor<::py3::simple::SimpleStruct>
    : public BaseExtractor<::py3::simple::SimpleStruct> {
  static const bool kUsingMarshal = true;
  ExtractorResult<::py3::simple::SimpleStruct> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::SimpleStruct>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::SimpleStruct>> {
  ExtractorResult<::py3::simple::SimpleStruct> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::SimpleStruct>
    : public BaseConstructor<::py3::simple::SimpleStruct> {
  static const bool kUsingMarshal = true;
  PyObject* operator()(const ::py3::simple::SimpleStruct& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::SimpleStruct>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::SimpleStruct>> {
  PyObject* operator()(const ::py3::simple::SimpleStruct& val);
};

template <>
struct Extractor<::py3::simple::HiddenTypeFieldsStruct>
    : public BaseExtractor<::py3::simple::HiddenTypeFieldsStruct> {
  static const bool kUsingMarshal = false;
  ExtractorResult<::py3::simple::HiddenTypeFieldsStruct> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::HiddenTypeFieldsStruct>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::HiddenTypeFieldsStruct>> {
  ExtractorResult<::py3::simple::HiddenTypeFieldsStruct> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::HiddenTypeFieldsStruct>
    : public BaseConstructor<::py3::simple::HiddenTypeFieldsStruct> {
  static const bool kUsingMarshal = false;
  PyObject* operator()(const ::py3::simple::HiddenTypeFieldsStruct& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::HiddenTypeFieldsStruct>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::HiddenTypeFieldsStruct>> {
  PyObject* operator()(const ::py3::simple::HiddenTypeFieldsStruct& val);
};

template <>
struct Extractor<::py3::simple::detail::AdaptedUnion>
    : public BaseExtractor<::py3::simple::detail::AdaptedUnion> {
  static const bool kUsingMarshal = true;
  ExtractorResult<::py3::simple::detail::AdaptedUnion> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::detail::AdaptedUnion>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::detail::AdaptedUnion>> {
  ExtractorResult<::py3::simple::detail::AdaptedUnion> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::detail::AdaptedUnion>
    : public BaseConstructor<::py3::simple::detail::AdaptedUnion> {
  static const bool kUsingMarshal = true;
  PyObject* operator()(const ::py3::simple::detail::AdaptedUnion& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::detail::AdaptedUnion>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::detail::AdaptedUnion>> {
  PyObject* operator()(const ::py3::simple::detail::AdaptedUnion& val);
};

template <>
struct Extractor<::py3::simple::HiddenException>
    : public BaseExtractor<::py3::simple::HiddenException> {
  static const bool kUsingMarshal = true;
  ExtractorResult<::py3::simple::HiddenException> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::HiddenException>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::HiddenException>> {
  ExtractorResult<::py3::simple::HiddenException> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::HiddenException>
    : public BaseConstructor<::py3::simple::HiddenException> {
  static const bool kUsingMarshal = true;
  PyObject* operator()(const ::py3::simple::HiddenException& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::HiddenException>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::HiddenException>> {
  PyObject* operator()(const ::py3::simple::HiddenException& val);
};

template <>
struct Extractor<::py3::simple::ComplexStruct>
    : public BaseExtractor<::py3::simple::ComplexStruct> {
  static const bool kUsingMarshal = false;
  ExtractorResult<::py3::simple::ComplexStruct> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::ComplexStruct>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::ComplexStruct>> {
  ExtractorResult<::py3::simple::ComplexStruct> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::ComplexStruct>
    : public BaseConstructor<::py3::simple::ComplexStruct> {
  static const bool kUsingMarshal = false;
  PyObject* operator()(const ::py3::simple::ComplexStruct& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::ComplexStruct>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::ComplexStruct>> {
  PyObject* operator()(const ::py3::simple::ComplexStruct& val);
};

template <>
struct Extractor<::py3::simple::BinaryUnion>
    : public BaseExtractor<::py3::simple::BinaryUnion> {
  static const bool kUsingMarshal = true;
  ExtractorResult<::py3::simple::BinaryUnion> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::BinaryUnion>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::BinaryUnion>> {
  ExtractorResult<::py3::simple::BinaryUnion> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::BinaryUnion>
    : public BaseConstructor<::py3::simple::BinaryUnion> {
  static const bool kUsingMarshal = true;
  PyObject* operator()(const ::py3::simple::BinaryUnion& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::BinaryUnion>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::BinaryUnion>> {
  PyObject* operator()(const ::py3::simple::BinaryUnion& val);
};

template <>
struct Extractor<::py3::simple::BinaryUnionStruct>
    : public BaseExtractor<::py3::simple::BinaryUnionStruct> {
  static const bool kUsingMarshal = true;
  ExtractorResult<::py3::simple::BinaryUnionStruct> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::BinaryUnionStruct>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::BinaryUnionStruct>> {
  ExtractorResult<::py3::simple::BinaryUnionStruct> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::BinaryUnionStruct>
    : public BaseConstructor<::py3::simple::BinaryUnionStruct> {
  static const bool kUsingMarshal = true;
  PyObject* operator()(const ::py3::simple::BinaryUnionStruct& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::BinaryUnionStruct>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::BinaryUnionStruct>> {
  PyObject* operator()(const ::py3::simple::BinaryUnionStruct& val);
};

template <>
struct Extractor<::py3::simple::CustomFields>
    : public BaseExtractor<::py3::simple::CustomFields> {
  static const bool kUsingMarshal = false;
  ExtractorResult<::py3::simple::CustomFields> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::CustomFields>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::CustomFields>> {
  ExtractorResult<::py3::simple::CustomFields> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::CustomFields>
    : public BaseConstructor<::py3::simple::CustomFields> {
  static const bool kUsingMarshal = false;
  PyObject* operator()(const ::py3::simple::CustomFields& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::CustomFields>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::CustomFields>> {
  PyObject* operator()(const ::py3::simple::CustomFields& val);
};

template <>
struct Extractor<::py3::simple::CustomTypedefFields>
    : public BaseExtractor<::py3::simple::CustomTypedefFields> {
  static const bool kUsingMarshal = false;
  ExtractorResult<::py3::simple::CustomTypedefFields> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::CustomTypedefFields>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::CustomTypedefFields>> {
  ExtractorResult<::py3::simple::CustomTypedefFields> operator()(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::CustomTypedefFields>
    : public BaseConstructor<::py3::simple::CustomTypedefFields> {
  static const bool kUsingMarshal = false;
  PyObject* operator()(const ::py3::simple::CustomTypedefFields& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::CustomTypedefFields>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::py3::simple::CustomTypedefFields>> {
  PyObject* operator()(const ::py3::simple::CustomTypedefFields& val);
};

template <>
struct Extractor<::py3::simple::AnEnum>
    : public BaseExtractor<::py3::simple::AnEnum> {
  ExtractorResult<::py3::simple::AnEnum> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::AnEnum> {
  PyObject* operator()(::py3::simple::AnEnum val);
};

template <>
struct Extractor<::py3::simple::AnEnumRenamed>
    : public BaseExtractor<::py3::simple::AnEnumRenamed> {
  ExtractorResult<::py3::simple::AnEnumRenamed> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::AnEnumRenamed> {
  PyObject* operator()(::py3::simple::AnEnumRenamed val);
};

template <>
struct Extractor<::py3::simple::Flags>
    : public BaseExtractor<::py3::simple::Flags> {
  ExtractorResult<::py3::simple::Flags> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Constructor<::py3::simple::Flags> {
  PyObject* operator()(::py3::simple::Flags val);
};

} // namespace capi
} // namespace python
} // namespace thrift
} // namespace apache
