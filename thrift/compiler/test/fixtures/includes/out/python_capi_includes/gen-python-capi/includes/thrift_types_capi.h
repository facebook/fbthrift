
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

#include <thrift/compiler/test/fixtures/includes/gen-cpp2/includes_types.h>

namespace includes {

struct NamespaceTag {};

} // namespace includes

namespace apache::thrift::python::capi {
template <>
struct Extractor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Included, ::includes::NamespaceTag>>
    : public BaseExtractor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Included, ::includes::NamespaceTag>> {
  static const bool kUsingMarshal = true;
  ExtractorResult<::cpp2::Included> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Included, ::includes::NamespaceTag >>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Included, ::includes::NamespaceTag>> {
  ExtractorResult<::cpp2::Included> operator()(PyObject* obj);
};

template <>
struct Constructor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Included, ::includes::NamespaceTag>>
    : public BaseConstructor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Included, ::includes::NamespaceTag>> {
  static const bool kUsingMarshal = true;
  PyObject* operator()(const ::cpp2::Included& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Included, ::includes::NamespaceTag>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Included, ::includes::NamespaceTag>> {
  PyObject* operator()(const ::cpp2::Included& val);
};

} // namespace apache::thrift::python::capi
