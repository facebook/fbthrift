
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

#include <thrift/compiler/test/fixtures/int_limits/gen-python-capi/module/thrift_types_api.h>
#include <thrift/compiler/test/fixtures/int_limits/gen-python-capi/module/thrift_types_capi.h>


namespace apache::thrift::python::capi {
namespace {
bool ensure_module_imported() {
  static ::folly::python::import_cache_nocapture import((
      ::import_module__thrift_types_capi));
  return import();
}
  static constexpr std::int16_t _fbthrift__Limits__tuple_pos[8] = {
    1, 2, 3, 4, 5, 6, 7, 8
  };
} // namespace

ExtractorResult<::cpp2::Limits>
Extractor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Limits, ::module::NamespaceTag>>::operator()(PyObject* obj) {
  int tCheckResult = typeCheck(obj);
  if (tCheckResult != 1) {
      if (tCheckResult == 0) {
        PyErr_SetString(PyExc_TypeError, "Not a Limits");
      }
      return extractorError<::cpp2::Limits>(
          "Marshal error: Limits");
  }
  StrongRef fbThriftData(getThriftData(obj));
  return Extractor<::apache::thrift::python::capi::ComposedStruct<
      ::cpp2::Limits, ::module::NamespaceTag>>{}(*fbThriftData);
}

ExtractorResult<::cpp2::Limits>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::cpp2::Limits, ::module::NamespaceTag>>::operator()(PyObject* fbThriftData) {
  ::cpp2::Limits cpp;
  std::optional<std::string_view> error;
  Extractor<int64_t>{}.extractInto(
      cpp.max_i64_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Limits__tuple_pos[0]),
      error);
  Extractor<int64_t>{}.extractInto(
      cpp.min_i64_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Limits__tuple_pos[1]),
      error);
  Extractor<int32_t>{}.extractInto(
      cpp.max_i32_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Limits__tuple_pos[2]),
      error);
  Extractor<int32_t>{}.extractInto(
      cpp.min_i32_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Limits__tuple_pos[3]),
      error);
  Extractor<int16_t>{}.extractInto(
      cpp.max_i16_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Limits__tuple_pos[4]),
      error);
  Extractor<int16_t>{}.extractInto(
      cpp.min_i16_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Limits__tuple_pos[5]),
      error);
  Extractor<int8_t>{}.extractInto(
      cpp.max_byte_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Limits__tuple_pos[6]),
      error);
  Extractor<int8_t>{}.extractInto(
      cpp.min_byte_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Limits__tuple_pos[7]),
      error);
  if (error) {
    return folly::makeUnexpected(*error);
  }
  return cpp;
}


int Extractor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Limits, ::module::NamespaceTag>>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module module import error");
  }
  int result =
      can_extract__module__Limits(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: Limits");
  }
  return result;
}


PyObject* Constructor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Limits, ::module::NamespaceTag>>::operator()(
    const ::cpp2::Limits& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Limits, ::module::NamespaceTag>> ctor;
  StrongRef fbthrift_data(ctor(val));
  if (!fbthrift_data) {
    return nullptr;
  }
  return init__module__Limits(*fbthrift_data);
}

PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Limits, ::module::NamespaceTag>>::operator()(
    [[maybe_unused]] const ::cpp2::Limits& val) {
  StrongRef fbthrift_data(createStructTuple(8));
  StrongRef _fbthrift__max_i64_field(
    Constructor<int64_t>{}
    .constructFrom(val.max_i64_field_ref()));
  if (!_fbthrift__max_i64_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Limits__tuple_pos[0],
          *_fbthrift__max_i64_field) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__min_i64_field(
    Constructor<int64_t>{}
    .constructFrom(val.min_i64_field_ref()));
  if (!_fbthrift__min_i64_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Limits__tuple_pos[1],
          *_fbthrift__min_i64_field) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__max_i32_field(
    Constructor<int32_t>{}
    .constructFrom(val.max_i32_field_ref()));
  if (!_fbthrift__max_i32_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Limits__tuple_pos[2],
          *_fbthrift__max_i32_field) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__min_i32_field(
    Constructor<int32_t>{}
    .constructFrom(val.min_i32_field_ref()));
  if (!_fbthrift__min_i32_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Limits__tuple_pos[3],
          *_fbthrift__min_i32_field) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__max_i16_field(
    Constructor<int16_t>{}
    .constructFrom(val.max_i16_field_ref()));
  if (!_fbthrift__max_i16_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Limits__tuple_pos[4],
          *_fbthrift__max_i16_field) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__min_i16_field(
    Constructor<int16_t>{}
    .constructFrom(val.min_i16_field_ref()));
  if (!_fbthrift__min_i16_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Limits__tuple_pos[5],
          *_fbthrift__min_i16_field) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__max_byte_field(
    Constructor<int8_t>{}
    .constructFrom(val.max_byte_field_ref()));
  if (!_fbthrift__max_byte_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Limits__tuple_pos[6],
          *_fbthrift__max_byte_field) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__min_byte_field(
    Constructor<int8_t>{}
    .constructFrom(val.min_byte_field_ref()));
  if (!_fbthrift__min_byte_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Limits__tuple_pos[7],
          *_fbthrift__min_byte_field) == -1) {
    return nullptr;
  }
  return std::move(fbthrift_data).release();
}


} // namespace apache::thrift::python::capi
