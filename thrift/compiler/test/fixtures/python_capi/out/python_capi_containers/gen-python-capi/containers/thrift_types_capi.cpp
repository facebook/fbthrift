
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

#include <thrift/compiler/test/fixtures/python_capi/gen-python-capi/containers/thrift_types_api.h>
#include <thrift/compiler/test/fixtures/python_capi/gen-python-capi/containers/thrift_types_capi.h>


namespace apache {
namespace thrift {
namespace python {
namespace capi {
namespace {
bool ensure_module_imported() {
  static ::folly::python::import_cache_nocapture import((
      ::import_test__fixtures__python_capi__containers__thrift_types_capi));
  return import();
}
} // namespace

ExtractorResult<::test::fixtures::python_capi::TemplateLists>
Extractor<::test::fixtures::python_capi::TemplateLists>::operator()(PyObject* obj) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return extractorError<::test::fixtures::python_capi::TemplateLists>(
      "Module test.fixtures.python_capi.containers import error");
  }
  std::unique_ptr<folly::IOBuf> val(
      extract__test__fixtures__python_capi__containers__TemplateLists(obj));
  if (!val) {
    CHECK(PyErr_Occurred());
    return extractorError<::test::fixtures::python_capi::TemplateLists>(
        "Thrift serialize error: TemplateLists");
  }
  return detail::deserialize_iobuf<::test::fixtures::python_capi::TemplateLists>(std::move(val));
}


ExtractorResult<::test::fixtures::python_capi::TemplateLists>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::test::fixtures::python_capi::TemplateLists>>::operator()(PyObject* fbthrift_data) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return extractorError<::test::fixtures::python_capi::TemplateLists>(
      "Module test.fixtures.python_capi.containers import error");
  }
  auto obj = StrongRef(init__test__fixtures__python_capi__containers__TemplateLists(fbthrift_data));
  if (!obj) {
      return extractorError<::test::fixtures::python_capi::TemplateLists>(
          "Init from fbthrift error: TemplateLists");
  }
  return Extractor<::test::fixtures::python_capi::TemplateLists>{}(*obj);
}

int Extractor<::test::fixtures::python_capi::TemplateLists>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.python_capi.containers import error");
  }
  int result =
      can_extract__test__fixtures__python_capi__containers__TemplateLists(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: TemplateLists");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::python_capi::TemplateLists>::operator()(
    const ::test::fixtures::python_capi::TemplateLists& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  ::std::unique_ptr<::folly::IOBuf> serialized;
  try {
    serialized = detail::serialize_to_iobuf(val);
  } catch (const apache::thrift::TProtocolException& e) {
    detail::handle_protocol_error(e);
    return nullptr;
  }
  DCHECK(serialized);
  auto ptr = construct__test__fixtures__python_capi__containers__TemplateLists(std::move(serialized));
  if (!ptr) {
    CHECK(PyErr_Occurred());
  }
  return ptr;
}


PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::python_capi::TemplateLists>>::operator()(
    const ::test::fixtures::python_capi::TemplateLists& val) {
  auto obj = StrongRef(Constructor<::test::fixtures::python_capi::TemplateLists>{}(val));
  if (!obj) {
    return nullptr;
  }
  return getThriftData(*obj);
}

ExtractorResult<::test::fixtures::python_capi::TemplateSets>
Extractor<::test::fixtures::python_capi::TemplateSets>::operator()(PyObject* obj) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return extractorError<::test::fixtures::python_capi::TemplateSets>(
      "Module test.fixtures.python_capi.containers import error");
  }
  std::unique_ptr<folly::IOBuf> val(
      extract__test__fixtures__python_capi__containers__TemplateSets(obj));
  if (!val) {
    CHECK(PyErr_Occurred());
    return extractorError<::test::fixtures::python_capi::TemplateSets>(
        "Thrift serialize error: TemplateSets");
  }
  return detail::deserialize_iobuf<::test::fixtures::python_capi::TemplateSets>(std::move(val));
}


ExtractorResult<::test::fixtures::python_capi::TemplateSets>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::test::fixtures::python_capi::TemplateSets>>::operator()(PyObject* fbthrift_data) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return extractorError<::test::fixtures::python_capi::TemplateSets>(
      "Module test.fixtures.python_capi.containers import error");
  }
  auto obj = StrongRef(init__test__fixtures__python_capi__containers__TemplateSets(fbthrift_data));
  if (!obj) {
      return extractorError<::test::fixtures::python_capi::TemplateSets>(
          "Init from fbthrift error: TemplateSets");
  }
  return Extractor<::test::fixtures::python_capi::TemplateSets>{}(*obj);
}

int Extractor<::test::fixtures::python_capi::TemplateSets>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.python_capi.containers import error");
  }
  int result =
      can_extract__test__fixtures__python_capi__containers__TemplateSets(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: TemplateSets");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::python_capi::TemplateSets>::operator()(
    const ::test::fixtures::python_capi::TemplateSets& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  ::std::unique_ptr<::folly::IOBuf> serialized;
  try {
    serialized = detail::serialize_to_iobuf(val);
  } catch (const apache::thrift::TProtocolException& e) {
    detail::handle_protocol_error(e);
    return nullptr;
  }
  DCHECK(serialized);
  auto ptr = construct__test__fixtures__python_capi__containers__TemplateSets(std::move(serialized));
  if (!ptr) {
    CHECK(PyErr_Occurred());
  }
  return ptr;
}


PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::python_capi::TemplateSets>>::operator()(
    const ::test::fixtures::python_capi::TemplateSets& val) {
  auto obj = StrongRef(Constructor<::test::fixtures::python_capi::TemplateSets>{}(val));
  if (!obj) {
    return nullptr;
  }
  return getThriftData(*obj);
}

ExtractorResult<::test::fixtures::python_capi::TemplateMaps>
Extractor<::test::fixtures::python_capi::TemplateMaps>::operator()(PyObject* obj) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return extractorError<::test::fixtures::python_capi::TemplateMaps>(
      "Module test.fixtures.python_capi.containers import error");
  }
  std::unique_ptr<folly::IOBuf> val(
      extract__test__fixtures__python_capi__containers__TemplateMaps(obj));
  if (!val) {
    CHECK(PyErr_Occurred());
    return extractorError<::test::fixtures::python_capi::TemplateMaps>(
        "Thrift serialize error: TemplateMaps");
  }
  return detail::deserialize_iobuf<::test::fixtures::python_capi::TemplateMaps>(std::move(val));
}


ExtractorResult<::test::fixtures::python_capi::TemplateMaps>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::test::fixtures::python_capi::TemplateMaps>>::operator()(PyObject* fbthrift_data) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return extractorError<::test::fixtures::python_capi::TemplateMaps>(
      "Module test.fixtures.python_capi.containers import error");
  }
  auto obj = StrongRef(init__test__fixtures__python_capi__containers__TemplateMaps(fbthrift_data));
  if (!obj) {
      return extractorError<::test::fixtures::python_capi::TemplateMaps>(
          "Init from fbthrift error: TemplateMaps");
  }
  return Extractor<::test::fixtures::python_capi::TemplateMaps>{}(*obj);
}

int Extractor<::test::fixtures::python_capi::TemplateMaps>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.python_capi.containers import error");
  }
  int result =
      can_extract__test__fixtures__python_capi__containers__TemplateMaps(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: TemplateMaps");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::python_capi::TemplateMaps>::operator()(
    const ::test::fixtures::python_capi::TemplateMaps& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  ::std::unique_ptr<::folly::IOBuf> serialized;
  try {
    serialized = detail::serialize_to_iobuf(val);
  } catch (const apache::thrift::TProtocolException& e) {
    detail::handle_protocol_error(e);
    return nullptr;
  }
  DCHECK(serialized);
  auto ptr = construct__test__fixtures__python_capi__containers__TemplateMaps(std::move(serialized));
  if (!ptr) {
    CHECK(PyErr_Occurred());
  }
  return ptr;
}


PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::python_capi::TemplateMaps>>::operator()(
    const ::test::fixtures::python_capi::TemplateMaps& val) {
  auto obj = StrongRef(Constructor<::test::fixtures::python_capi::TemplateMaps>{}(val));
  if (!obj) {
    return nullptr;
  }
  return getThriftData(*obj);
}

} // namespace capi
} // namespace python
} // namespace thrift
} // namespace apache