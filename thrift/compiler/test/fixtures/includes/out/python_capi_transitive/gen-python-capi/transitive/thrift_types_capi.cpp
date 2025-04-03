
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

#include <thrift/compiler/test/fixtures/includes/gen-python-capi/transitive/thrift_types_api.h>
#include <thrift/compiler/test/fixtures/includes/gen-python-capi/transitive/thrift_types_capi.h>


namespace apache::thrift::python::capi {
namespace {
bool ensure_module_imported() {
  static ::folly::python::import_cache_nocapture import((
      ::import_transitive__thrift_types_capi));
  return import();
}
  static constexpr std::int16_t _fbthrift__Foo__tuple_pos[1] = {
    1
  };
} // namespace

ExtractorResult<::cpp2::Foo>
Extractor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Foo, ::transitive::NamespaceTag>>::operator()(PyObject* obj) {
  int tCheckResult = typeCheck(obj);
  if (tCheckResult != 1) {
      if (tCheckResult == 0) {
        PyErr_SetString(PyExc_TypeError, "Not a Foo");
      }
      return extractorError<::cpp2::Foo>(
          "Marshal error: Foo");
  }
  StrongRef fbThriftData(getThriftData(obj));
  return Extractor<::apache::thrift::python::capi::ComposedStruct<
      ::cpp2::Foo, ::transitive::NamespaceTag>>{}(*fbThriftData);
}

ExtractorResult<::cpp2::Foo>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::cpp2::Foo, ::transitive::NamespaceTag>>::operator()(PyObject* fbThriftData) {
  ::cpp2::Foo cpp;
  std::optional<std::string_view> error;
  Extractor<int64_t>{}.extractInto(
      cpp.a_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__Foo__tuple_pos[0]),
      error);
  if (error) {
    return folly::makeUnexpected(*error);
  }
  return cpp;
}


int Extractor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Foo, ::transitive::NamespaceTag>>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module transitive import error");
  }
  int result =
      can_extract__transitive__Foo(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: Foo");
  }
  return result;
}


PyObject* Constructor<::apache::thrift::python::capi::PythonNamespaced<::cpp2::Foo, ::transitive::NamespaceTag>>::operator()(
    const ::cpp2::Foo& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Foo, ::transitive::NamespaceTag>> ctor;
  StrongRef fbthrift_data(ctor(val));
  if (!fbthrift_data) {
    return nullptr;
  }
  return init__transitive__Foo(*fbthrift_data);
}

PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::cpp2::Foo, ::transitive::NamespaceTag>>::operator()(
    [[maybe_unused]] const ::cpp2::Foo& val) {
  StrongRef fbthrift_data(createStructTuple(1));
  StrongRef _fbthrift__a(
    Constructor<int64_t>{}
    .constructFrom(val.a_ref()));
  if (!_fbthrift__a ||
      setStructField(
          *fbthrift_data,
          _fbthrift__Foo__tuple_pos[0],
          *_fbthrift__a) == -1) {
    return nullptr;
  }
  return std::move(fbthrift_data).release();
}


} // namespace apache::thrift::python::capi
