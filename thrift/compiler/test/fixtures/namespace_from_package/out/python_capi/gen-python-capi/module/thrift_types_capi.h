
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

#include <thrift/compiler/test/fixtures/namespace_from_package/gen-cpp2/module_types.h>

namespace apache {
namespace thrift {
namespace python {
namespace capi {
template <>
struct Extractor<::test::namespace_from_package::module::Foo>
    : public BaseExtractor<::test::namespace_from_package::module::Foo> {
  static const bool kUsingMarshal = true;
  ExtractorResult<::test::namespace_from_package::module::Foo> operator()(PyObject* obj);
  int typeCheck(PyObject* obj);
};

template <>
struct Extractor<::apache::thrift::python::capi::ComposedStruct<
        ::test::namespace_from_package::module::Foo>>
    : public BaseExtractor<::apache::thrift::python::capi::ComposedStruct<
        ::test::namespace_from_package::module::Foo>> {
  ExtractorResult<::test::namespace_from_package::module::Foo> operator()(PyObject* obj);
};

template <>
struct Constructor<::test::namespace_from_package::module::Foo>
    : public BaseConstructor<::test::namespace_from_package::module::Foo> {
  static const bool kUsingMarshal = true;
  PyObject* operator()(const ::test::namespace_from_package::module::Foo& val);
};

template <>
struct Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::namespace_from_package::module::Foo>>
    : public BaseConstructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::namespace_from_package::module::Foo>> {
  PyObject* operator()(const ::test::namespace_from_package::module::Foo& val);
};

} // namespace capi
} // namespace python
} // namespace thrift
} // namespace apache
