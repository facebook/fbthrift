
/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT
 *  @generated
 *
 */

#include <folly/python/import.h>
#include <thrift/lib/python/capi/iobuf.h>
#include <thrift/lib/python/types.h>

#include <thrift/compiler/test/fixtures/includes/gen-python-capi/includes/thrift_types_api.h>
#include <thrift/compiler/test/fixtures/includes/gen-python-capi/includes/thrift_types_capi.h>

#include "thrift/compiler/test/fixtures/includes/gen-python-capi/transitive/thrift_types_capi.h"

namespace apache::thrift::python::capi {
namespace {
bool ensure_module_imported() {
  static ::folly::python::import_cache_nocapture import((
      ::import_includes__thrift_types_capi));
  return import();
}
  static constexpr std::int16_t _fbthrift__Included__tuple_pos[2] = {
    1, 2
  };
} // namespace

ExtractorResult<::cpp2::Included>
Extractor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Included, ::includes::NamespaceTag>>::operator()(PyObject* obj) {
  int tCheckResult = typeCheck(obj);
  if (tCheckResult != 1) {
      if (tCheckResult == 0) {
        PyErr_SetString(PyExc_TypeError, "Not a Included");
      }
      return extractorError<::cpp2::Included>(
          "Marshal error: Included");
  }
  StrongRef fbThriftData(getThriftData(obj));
  return Extractor<::apache::thrift::python::capi::ComposedStruct<
      ::cpp2::Included, ::includes::NamespaceTag>>{}(*fbThriftData);
}

ExtractorResult<::cpp2::Included>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::cpp2::Included, ::includes::NamespaceTag>>::operator()(PyObject* fbThriftData) {
  ::cpp2::Included cpp;
  std::optional<std::string_view> error;
  Extractor<int64_t>{}.extractInto(
      cpp.MyIntField_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Included__tuple_pos[0]),
      error);
  Extractor<::apache::thrift::python::capi::ComposedStruct<::cpp2::Foo, ::transitive::NamespaceTag>>{}.extractInto(
      cpp.MyTransitiveField_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Included__tuple_pos[1]),
      error);
  if (error) {
    return folly::makeUnexpected(*error);
  }
  return cpp;
}


int Extractor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Included, ::includes::NamespaceTag>>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module includes import error");
  }
  int result =
      can_extract__includes__Included(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: Included");
  }
  return result;
}


PyObject* Constructor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Included, ::includes::NamespaceTag>>::operator()(
    const ::cpp2::Included& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Included, ::includes::NamespaceTag>> ctor;
  StrongRef fbthrift_data(ctor(val));
  if (!fbthrift_data) {
    return nullptr;
  }
  return init__includes__Included(*fbthrift_data);
}

PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Included, ::includes::NamespaceTag>>::operator()(
    [[maybe_unused]] const ::cpp2::Included& val) {
  StrongRef fbthrift_data(createStructTuple(2));
  StrongRef _fbthrift__MyIntField(
    Constructor<int64_t>{}
    .constructFrom(val.MyIntField_ref()));
  if (!_fbthrift__MyIntField ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Included__tuple_pos[0],
          *_fbthrift__MyIntField) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__MyTransitiveField(
    Constructor<::apache::thrift::python::capi::ComposedStruct<::cpp2::Foo, ::transitive::NamespaceTag>>{}
    .constructFrom(val.MyTransitiveField_ref()));
  if (!_fbthrift__MyTransitiveField ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Included__tuple_pos[1],
          *_fbthrift__MyTransitiveField) == -1) {
    return nullptr;
  }
  return std::move(fbthrift_data).release();
}


} // namespace apache::thrift::python::capi
