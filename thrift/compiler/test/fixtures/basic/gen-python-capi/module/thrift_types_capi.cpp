
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

#include <thrift/compiler/test/fixtures/basic/gen-python-capi/module/thrift_types_api.h>
#include <thrift/compiler/test/fixtures/basic/gen-python-capi/module/thrift_types_capi.h>


namespace apache {
namespace thrift {
namespace python {
namespace capi {
namespace {
bool ensure_module_imported() {
  static ::folly::python::import_cache_nocapture import((
      ::import_test__fixtures__basic__module__thrift_types_capi));
  return import();
}
  static constexpr std::int16_t _fbthrift__MyStruct__tuple_pos[9] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9
  };
  static constexpr std::int16_t _fbthrift__ReservedKeyword__tuple_pos[1] = {
    1
  };
} // namespace

ExtractorResult<::test::fixtures::basic::MyStruct>
Extractor<::test::fixtures::basic::MyStruct>::operator()(PyObject* obj) {
  int tCheckResult = typeCheck(obj);
  if (tCheckResult != 1) {
      if (tCheckResult == 0) {
        PyErr_SetString(PyExc_TypeError, "Not a MyStruct");
      }
      return extractorError<::test::fixtures::basic::MyStruct>(
          "Marshal error: MyStruct");
  }
  StrongRef fbThriftData(getThriftData(obj));
  return Extractor<::apache::thrift::python::capi::ComposedStruct<
      ::test::fixtures::basic::MyStruct>>{}(*fbThriftData);
}

ExtractorResult<::test::fixtures::basic::MyStruct>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::test::fixtures::basic::MyStruct>>::operator()(PyObject* fbThriftData) {
  ::test::fixtures::basic::MyStruct cpp;
  std::optional<std::string_view> error;
  Extractor<int64_t>{}.extractInto(
      cpp.MyIntField_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__MyStruct__tuple_pos[0]),
      error);
  Extractor<Bytes>{}.extractInto(
      cpp.MyStringField_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__MyStruct__tuple_pos[1]),
      error);
  Extractor<::apache::thrift::python::capi::ComposedStruct<::test::fixtures::basic::MyDataItem>>{}.extractInto(
      cpp.MyDataField_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__MyStruct__tuple_pos[2]),
      error);
  Extractor<::apache::thrift::python::capi::ComposedEnum<::test::fixtures::basic::MyEnum>>{}.extractInto(
      cpp.myEnum_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__MyStruct__tuple_pos[3]),
      error);
  Extractor<bool>{}.extractInto(
      cpp.oneway_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__MyStruct__tuple_pos[4]),
      error);
  Extractor<bool>{}.extractInto(
      cpp.readonly_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__MyStruct__tuple_pos[5]),
      error);
  Extractor<bool>{}.extractInto(
      cpp.idempotent_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__MyStruct__tuple_pos[6]),
      error);
  Extractor<set<float>>{}.extractInto(
      cpp.floatSet_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__MyStruct__tuple_pos[7]),
      error);
  Extractor<Bytes>{}.extractInto(
      cpp.no_hack_codegen_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__MyStruct__tuple_pos[8]),
      error);
  if (error) {
    return folly::makeUnexpected(*error);
  }
  return cpp;
}


int Extractor<::test::fixtures::basic::MyStruct>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.basic.module import error");
  }
  int result =
      can_extract__test__fixtures__basic__module__MyStruct(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: MyStruct");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::basic::MyStruct>::operator()(
    const ::test::fixtures::basic::MyStruct& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::MyStruct>> ctor;
  StrongRef fbthrift_data(ctor(val));
  if (!fbthrift_data) {
    return nullptr;
  }
  return init__test__fixtures__basic__module__MyStruct(*fbthrift_data);
}

PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::MyStruct>>::operator()(
    [[maybe_unused]] const ::test::fixtures::basic::MyStruct& val) {
  StrongRef fbthrift_data(createStructTuple(9));
  StrongRef _fbthrift__MyIntField(
    Constructor<int64_t>{}
    .constructFrom(val.MyIntField_ref()));
  if (!_fbthrift__MyIntField ||
      setStructField(
          *fbthrift_data,
          _fbthrift__MyStruct__tuple_pos[0],
          *_fbthrift__MyIntField) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__MyStringField(
    Constructor<Bytes>{}
    .constructFrom(val.MyStringField_ref()));
  if (!_fbthrift__MyStringField ||
      setStructField(
          *fbthrift_data,
          _fbthrift__MyStruct__tuple_pos[1],
          *_fbthrift__MyStringField) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__MyDataField(
    Constructor<::apache::thrift::python::capi::ComposedStruct<::test::fixtures::basic::MyDataItem>>{}
    .constructFrom(val.MyDataField_ref()));
  if (!_fbthrift__MyDataField ||
      setStructField(
          *fbthrift_data,
          _fbthrift__MyStruct__tuple_pos[2],
          *_fbthrift__MyDataField) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__myEnum(
    Constructor<::apache::thrift::python::capi::ComposedEnum<::test::fixtures::basic::MyEnum>>{}
    .constructFrom(val.myEnum_ref()));
  if (!_fbthrift__myEnum ||
      setStructField(
          *fbthrift_data,
          _fbthrift__MyStruct__tuple_pos[3],
          *_fbthrift__myEnum) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__oneway(
    Constructor<bool>{}
    .constructFrom(val.oneway_ref()));
  if (!_fbthrift__oneway ||
      setStructField(
          *fbthrift_data,
          _fbthrift__MyStruct__tuple_pos[4],
          *_fbthrift__oneway) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__readonly(
    Constructor<bool>{}
    .constructFrom(val.readonly_ref()));
  if (!_fbthrift__readonly ||
      setStructField(
          *fbthrift_data,
          _fbthrift__MyStruct__tuple_pos[5],
          *_fbthrift__readonly) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__idempotent(
    Constructor<bool>{}
    .constructFrom(val.idempotent_ref()));
  if (!_fbthrift__idempotent ||
      setStructField(
          *fbthrift_data,
          _fbthrift__MyStruct__tuple_pos[6],
          *_fbthrift__idempotent) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__floatSet(
    Constructor<set<float>>{}
    .constructFrom(val.floatSet_ref()));
  if (!_fbthrift__floatSet ||
      setStructField(
          *fbthrift_data,
          _fbthrift__MyStruct__tuple_pos[7],
          *_fbthrift__floatSet) == -1) {
    return nullptr;
  }
  StrongRef _fbthrift__no_hack_codegen_field(
    Constructor<Bytes>{}
    .constructFrom(val.no_hack_codegen_field_ref()));
  if (!_fbthrift__no_hack_codegen_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__MyStruct__tuple_pos[8],
          *_fbthrift__no_hack_codegen_field) == -1) {
    return nullptr;
  }
  return std::move(fbthrift_data).release();
}


ExtractorResult<::test::fixtures::basic::MyDataItem>
Extractor<::test::fixtures::basic::MyDataItem>::operator()(PyObject* obj) {
  int tCheckResult = typeCheck(obj);
  if (tCheckResult != 1) {
      if (tCheckResult == 0) {
        PyErr_SetString(PyExc_TypeError, "Not a MyDataItem");
      }
      return extractorError<::test::fixtures::basic::MyDataItem>(
          "Marshal error: MyDataItem");
  }
  return ::test::fixtures::basic::MyDataItem{};
}

ExtractorResult<::test::fixtures::basic::MyDataItem>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::test::fixtures::basic::MyDataItem>>::operator()(PyObject* fbThriftData) {
  ::test::fixtures::basic::MyDataItem cpp;
  (void)fbThriftData;
  return cpp;
}


int Extractor<::test::fixtures::basic::MyDataItem>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.basic.module import error");
  }
  int result =
      can_extract__test__fixtures__basic__module__MyDataItem(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: MyDataItem");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::basic::MyDataItem>::operator()(
    const ::test::fixtures::basic::MyDataItem& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::MyDataItem>> ctor;
  StrongRef fbthrift_data(ctor(val));
  if (!fbthrift_data) {
    return nullptr;
  }
  return init__test__fixtures__basic__module__MyDataItem(*fbthrift_data);
}

PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::MyDataItem>>::operator()(
    [[maybe_unused]] const ::test::fixtures::basic::MyDataItem& val) {
  StrongRef fbthrift_data(createStructTuple(0));
  return std::move(fbthrift_data).release();
}


ExtractorResult<::test::fixtures::basic::MyUnion>
Extractor<::test::fixtures::basic::MyUnion>::operator()(PyObject* obj) {
  int tCheckResult = typeCheck(obj);
  if (tCheckResult != 1) {
      if (tCheckResult == 0) {
        PyErr_SetString(PyExc_TypeError, "Not a MyUnion");
      }
      return extractorError<::test::fixtures::basic::MyUnion>(
          "Marshal error: MyUnion");
  }
  StrongRef fbThriftData(getThriftData(obj));
  return Extractor<::apache::thrift::python::capi::ComposedStruct<
      ::test::fixtures::basic::MyUnion>>{}(*fbThriftData);
}

ExtractorResult<::test::fixtures::basic::MyUnion>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::test::fixtures::basic::MyUnion>>::operator()(PyObject* fbThriftData) {
  ::test::fixtures::basic::MyUnion cpp;
  std::optional<std::string_view> error;
  auto type_tag = Extractor<int64_t>{}(PyTuple_GET_ITEM(fbThriftData, 0));
  if (type_tag.hasError()) {
    return folly::makeUnexpected(type_tag.error());
  }
  switch (*type_tag) {
    case 0:
      break; // union is unset
    case 1:
      Extractor<::apache::thrift::python::capi::ComposedEnum<::test::fixtures::basic::MyEnum>>{}.extractInto(
          cpp.myEnum_ref(), PyTuple_GET_ITEM(fbThriftData, 1), error);
      break;
    case 2:
      Extractor<::apache::thrift::python::capi::ComposedStruct<::test::fixtures::basic::MyStruct>>{}.extractInto(
          cpp.myStruct_ref(), PyTuple_GET_ITEM(fbThriftData, 1), error);
      break;
    case 3:
      Extractor<::apache::thrift::python::capi::ComposedStruct<::test::fixtures::basic::MyDataItem>>{}.extractInto(
          cpp.myDataItem_ref(), PyTuple_GET_ITEM(fbThriftData, 1), error);
      break;
    case 4:
      Extractor<set<float>>{}.extractInto(
          cpp.floatSet_ref(), PyTuple_GET_ITEM(fbThriftData, 1), error);
      break;
  }
  if (error) {
    return folly::makeUnexpected(*error);
  }
  return cpp;
}


int Extractor<::test::fixtures::basic::MyUnion>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.basic.module import error");
  }
  int result =
      can_extract__test__fixtures__basic__module__MyUnion(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: MyUnion");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::basic::MyUnion>::operator()(
    const ::test::fixtures::basic::MyUnion& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::MyUnion>> ctor;
  StrongRef fbthrift_data(ctor(val));
  if (!fbthrift_data) {
    return nullptr;
  }
  return init__test__fixtures__basic__module__MyUnion(*fbthrift_data);
}

PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::MyUnion>>::operator()(
    [[maybe_unused]] const ::test::fixtures::basic::MyUnion& val) {
  int64_t type_key = static_cast<int64_t>(val.getType());
  StrongRef py_val;
  switch (type_key) {
    case 0:
      Py_INCREF(Py_None);
      py_val = StrongRef(Py_None);
      break;
    case 1:
      py_val = StrongRef(
          Constructor<::apache::thrift::python::capi::ComposedEnum<::test::fixtures::basic::MyEnum>>{}
          .constructFrom(val.myEnum_ref()));
      break;
    case 2:
      py_val = StrongRef(
          Constructor<::apache::thrift::python::capi::ComposedStruct<::test::fixtures::basic::MyStruct>>{}
          .constructFrom(val.myStruct_ref()));
      break;
    case 3:
      py_val = StrongRef(
          Constructor<::apache::thrift::python::capi::ComposedStruct<::test::fixtures::basic::MyDataItem>>{}
          .constructFrom(val.myDataItem_ref()));
      break;
    case 4:
      py_val = StrongRef(
          Constructor<set<float>>{}
          .constructFrom(val.floatSet_ref()));
      break;
  }
  if (!py_val) {
    return nullptr;
  }
  return unionTupleFromValue(type_key, *py_val);
}


ExtractorResult<::test::fixtures::basic::ReservedKeyword>
Extractor<::test::fixtures::basic::ReservedKeyword>::operator()(PyObject* obj) {
  int tCheckResult = typeCheck(obj);
  if (tCheckResult != 1) {
      if (tCheckResult == 0) {
        PyErr_SetString(PyExc_TypeError, "Not a ReservedKeyword");
      }
      return extractorError<::test::fixtures::basic::ReservedKeyword>(
          "Marshal error: ReservedKeyword");
  }
  StrongRef fbThriftData(getThriftData(obj));
  return Extractor<::apache::thrift::python::capi::ComposedStruct<
      ::test::fixtures::basic::ReservedKeyword>>{}(*fbThriftData);
}

ExtractorResult<::test::fixtures::basic::ReservedKeyword>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::test::fixtures::basic::ReservedKeyword>>::operator()(PyObject* fbThriftData) {
  ::test::fixtures::basic::ReservedKeyword cpp;
  std::optional<std::string_view> error;
  Extractor<int32_t>{}.extractInto(
      cpp.reserved_field_ref(),
      PyTuple_GET_ITEM(fbThriftData, _fbthrift__ReservedKeyword__tuple_pos[0]),
      error);
  if (error) {
    return folly::makeUnexpected(*error);
  }
  return cpp;
}


int Extractor<::test::fixtures::basic::ReservedKeyword>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.basic.module import error");
  }
  int result =
      can_extract__test__fixtures__basic__module__ReservedKeyword(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: ReservedKeyword");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::basic::ReservedKeyword>::operator()(
    const ::test::fixtures::basic::ReservedKeyword& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::ReservedKeyword>> ctor;
  StrongRef fbthrift_data(ctor(val));
  if (!fbthrift_data) {
    return nullptr;
  }
  return init__test__fixtures__basic__module__ReservedKeyword(*fbthrift_data);
}

PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::ReservedKeyword>>::operator()(
    [[maybe_unused]] const ::test::fixtures::basic::ReservedKeyword& val) {
  StrongRef fbthrift_data(createStructTuple(1));
  StrongRef _fbthrift__reserved_field(
    Constructor<int32_t>{}
    .constructFrom(val.reserved_field_ref()));
  if (!_fbthrift__reserved_field ||
      setStructField(
          *fbthrift_data,
          _fbthrift__ReservedKeyword__tuple_pos[0],
          *_fbthrift__reserved_field) == -1) {
    return nullptr;
  }
  return std::move(fbthrift_data).release();
}


ExtractorResult<::test::fixtures::basic::UnionToBeRenamed>
Extractor<::test::fixtures::basic::UnionToBeRenamed>::operator()(PyObject* obj) {
  int tCheckResult = typeCheck(obj);
  if (tCheckResult != 1) {
      if (tCheckResult == 0) {
        PyErr_SetString(PyExc_TypeError, "Not a UnionToBeRenamed");
      }
      return extractorError<::test::fixtures::basic::UnionToBeRenamed>(
          "Marshal error: UnionToBeRenamed");
  }
  StrongRef fbThriftData(getThriftData(obj));
  return Extractor<::apache::thrift::python::capi::ComposedStruct<
      ::test::fixtures::basic::UnionToBeRenamed>>{}(*fbThriftData);
}

ExtractorResult<::test::fixtures::basic::UnionToBeRenamed>
Extractor<::apache::thrift::python::capi::ComposedStruct<
    ::test::fixtures::basic::UnionToBeRenamed>>::operator()(PyObject* fbThriftData) {
  ::test::fixtures::basic::UnionToBeRenamed cpp;
  std::optional<std::string_view> error;
  auto type_tag = Extractor<int64_t>{}(PyTuple_GET_ITEM(fbThriftData, 0));
  if (type_tag.hasError()) {
    return folly::makeUnexpected(type_tag.error());
  }
  switch (*type_tag) {
    case 0:
      break; // union is unset
    case 1:
      Extractor<int32_t>{}.extractInto(
          cpp.reserved_field_ref(), PyTuple_GET_ITEM(fbThriftData, 1), error);
      break;
  }
  if (error) {
    return folly::makeUnexpected(*error);
  }
  return cpp;
}


int Extractor<::test::fixtures::basic::UnionToBeRenamed>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.basic.module import error");
  }
  int result =
      can_extract__test__fixtures__basic__module__UnionToBeRenamed(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: UnionToBeRenamed");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::basic::UnionToBeRenamed>::operator()(
    const ::test::fixtures::basic::UnionToBeRenamed& val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::UnionToBeRenamed>> ctor;
  StrongRef fbthrift_data(ctor(val));
  if (!fbthrift_data) {
    return nullptr;
  }
  return init__test__fixtures__basic__module__UnionToBeRenamed(*fbthrift_data);
}

PyObject* Constructor<::apache::thrift::python::capi::ComposedStruct<
        ::test::fixtures::basic::UnionToBeRenamed>>::operator()(
    [[maybe_unused]] const ::test::fixtures::basic::UnionToBeRenamed& val) {
  int64_t type_key = static_cast<int64_t>(val.getType());
  StrongRef py_val;
  switch (type_key) {
    case 0:
      Py_INCREF(Py_None);
      py_val = StrongRef(Py_None);
      break;
    case 1:
      py_val = StrongRef(
          Constructor<int32_t>{}
          .constructFrom(val.reserved_field_ref()));
      break;
  }
  if (!py_val) {
    return nullptr;
  }
  return unionTupleFromValue(type_key, *py_val);
}


ExtractorResult<::test::fixtures::basic::MyEnum>
Extractor<::test::fixtures::basic::MyEnum>::operator()(PyObject* obj) {
  long val = PyLong_AsLong(obj);
  if (val == -1 && PyErr_Occurred()) {
    return extractorError<::test::fixtures::basic::MyEnum>(
        "Error getting python int value: MyEnum");
  }
  return static_cast<::test::fixtures::basic::MyEnum>(val);
}

int Extractor<::test::fixtures::basic::MyEnum>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.basic.module import error");
  }
  int result =
      can_extract__test__fixtures__basic__module__MyEnum(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: MyEnum");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::basic::MyEnum>::operator()(
    ::test::fixtures::basic::MyEnum val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  auto ptr = construct__test__fixtures__basic__module__MyEnum(
      static_cast<int64_t>(val));
  if (!ptr) {
    CHECK(PyErr_Occurred());
  }
  return ptr;
}

ExtractorResult<::test::fixtures::basic::HackEnum>
Extractor<::test::fixtures::basic::HackEnum>::operator()(PyObject* obj) {
  long val = PyLong_AsLong(obj);
  if (val == -1 && PyErr_Occurred()) {
    return extractorError<::test::fixtures::basic::HackEnum>(
        "Error getting python int value: HackEnum");
  }
  return static_cast<::test::fixtures::basic::HackEnum>(val);
}

int Extractor<::test::fixtures::basic::HackEnum>::typeCheck(PyObject* obj) {
  if (!ensure_module_imported()) {
    ::folly::python::handlePythonError(
      "Module test.fixtures.basic.module import error");
  }
  int result =
      can_extract__test__fixtures__basic__module__HackEnum(obj);
  if (result < 0) {
    ::folly::python::handlePythonError(
      "Unexpected type check error: HackEnum");
  }
  return result;
}


PyObject* Constructor<::test::fixtures::basic::HackEnum>::operator()(
    ::test::fixtures::basic::HackEnum val) {
  if (!ensure_module_imported()) {
    DCHECK(PyErr_Occurred() != nullptr);
    return nullptr;
  }
  auto ptr = construct__test__fixtures__basic__module__HackEnum(
      static_cast<int64_t>(val));
  if (!ptr) {
    CHECK(PyErr_Occurred());
  }
  return ptr;
}

} // namespace capi
} // namespace python
} // namespace thrift
} // namespace apache
