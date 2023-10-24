
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

#include <thrift/compiler/test/fixtures/namespace_from_package/src/gen-python-capi/module/thrift_types_api.h>
#include <thrift/compiler/test/fixtures/namespace_from_package/src/gen-python-capi/module/thrift_types_capi.h>


namespace apache {
namespace thrift {
namespace python {
namespace capi {
namespace {
bool ensure_module_imported() {
  static ::folly::python::import_cache_nocapture import((
      ::import_test__namespace_from_package__module__thrift_types_capi));
  return import();
}
} // namespace

ExtractorResult<::test::namespace_from_package::module::Foo>
Extractor<::test::namespace_from_package::module::Foo>::operator()(PyObject* obj) {
  int tCheckResult = typeCheck(obj);
  if (tCheckResult != 1) {
      if (tCheckResult == 0) {
        PyErr_SetString(PyExc_TypeError, "Not a Foo");
      }
      return extractorError<::test::namespace_from_package::module::Foo>(
          "Marshal error: Foo");
  }
  StrongRef fbThriftData(getThriftData(obj));
  return Extractor<::apache::thrift::python::capi::ComposedStruct<
      ::test::namespace_from_package::module::Foo>>{}(*fbThriftData);
}

ExtractorResult<::test::namespace_from_package::module::Foo>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::test::namespace_from_package::module::Foo>>::operator()(PyObject* fbThriftData) {
  ::test::namespace_from_package::module::Foo cpp;
  std::optional<std::string_view> error;
  const int _fbthrift__tuple_pos[1] = {
    1
  };
  Extractor<int64_t>{}.extractInto(
      cpp.MyInt_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__tuple_pos[0]),
      error);
  if (error) {
    return folly::makeUnexpected(*error);
  }
  return cpp;
}


int Extractor<::test::namespace_from_package::module::Foo>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.namespace_from_package.module import error");
  }
  int result =
      can_extract__test__namespace_from_package__module__Foo(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: Foo");
  }
  return result;
}


PyObject* Constructor<::test::namespace_from_package::module::Foo>::operator()(
    const ::test::namespace_from_package::module::Foo& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::namespace_from_package::module::Foo>> ctor;
  StrongRef fbthrift_data(ctor(val));
  if (!fbthrift_data) {
    return nullptr;
  }
  return init__test__namespace_from_package__module__Foo(*fbthrift_data);
}

PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::namespace_from_package::module::Foo>>::operator()(
    FOLLY_MAYBE_UNUSED const ::test::namespace_from_package::module::Foo& val) {
  const int _fbthrift__tuple_pos[1] = {
    1
  };
  StrongRef fbthrift_data(createStructTuple(1));
  StrongRef _fbthrift__MyInt(
    Constructor<int64_t>{}
    .constructFrom(val.MyInt_ref()));
  if (!_fbthrift__MyInt ||
      setStructField(*fbthrift_data, _fbthrift__tuple_pos[0], *_fbthrift__MyInt) == -1) {
    return nullptr;
  }
  return std::move(fbthrift_data).release();
}


} // namespace capi
} // namespace python
} // namespace thrift
} // namespace apache