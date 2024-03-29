/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/basic-structured-annotations/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */

#include "thrift/compiler/test/fixtures/basic-structured-annotations/gen-cpp2/module_data.h"

#include <thrift/lib/cpp2/gen/module_data_cpp.h>

FOLLY_CLANG_DISABLE_WARNING("-Wunused-macros")

#if defined(__GNUC__) && defined(__linux__) && !FOLLY_MOBILE
// These attributes are applied to the static data members to ensure that they
// are not stripped from the compiled binary, in order to keep them available
// for use by debuggers at runtime.
//
// The "used" attribute is required to ensure the compiler always emits unused
// data.
//
// The "section" attribute is required to stop the linker from stripping used
// data. It works by forcing all of the data members (both used and unused ones)
// into the same section. As the linker strips data on a per-section basis, it
// is then unable to remove unused data without also removing used data.
// This has a similar effect to the "retain" attribute, but works with older
// toolchains.
#define THRIFT_DATA_MEMBER [[gnu::used]] [[gnu::section(".rodata.thrift.data")]]
#else
#define THRIFT_DATA_MEMBER
#endif

namespace apache {
namespace thrift {

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::test::fixtures::basic-structured-annotations::runtime_annotation>::name = "runtime_annotation";
THRIFT_DATA_MEMBER const std::array<std::string_view, 0> TStructDataStorage<::test::fixtures::basic-structured-annotations::runtime_annotation>::fields_names = { {
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 0> TStructDataStorage<::test::fixtures::basic-structured-annotations::runtime_annotation>::fields_ids = { {
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 0> TStructDataStorage<::test::fixtures::basic-structured-annotations::runtime_annotation>::fields_types = { {
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 0> TStructDataStorage<::test::fixtures::basic-structured-annotations::runtime_annotation>::storage_names = { {
}};
THRIFT_DATA_MEMBER const std::array<int, 0> TStructDataStorage<::test::fixtures::basic-structured-annotations::runtime_annotation>::isset_indexes = { {
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_inline>::name = "structured_annotation_inline";
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_inline>::fields_names = { {
  "count"sv,
  "name"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_inline>::fields_ids = { {
  1,
  2,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_inline>::fields_types = { {
  TType::T_I64,
  TType::T_STRING,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_inline>::storage_names = { {
  "__fbthrift_field_count"sv,
  "__fbthrift_field_name"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_inline>::isset_indexes = { {
  0,
  1,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_with_default>::name = "structured_annotation_with_default";
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_with_default>::fields_names = { {
  "name"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_with_default>::fields_ids = { {
  1,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_with_default>::fields_types = { {
  TType::T_STRING,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_with_default>::storage_names = { {
  "__fbthrift_field_name"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_with_default>::isset_indexes = { {
  0,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_recursive>::name = "structured_annotation_recursive";
THRIFT_DATA_MEMBER const std::array<std::string_view, 3> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_recursive>::fields_names = { {
  "name"sv,
  "recurse"sv,
  "forward"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 3> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_recursive>::fields_ids = { {
  1,
  2,
  3,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 3> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_recursive>::fields_types = { {
  TType::T_STRING,
  TType::T_STRUCT,
  TType::T_STRUCT,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 3> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_recursive>::storage_names = { {
  "__fbthrift_field_name"sv,
  "__fbthrift_field_recurse"sv,
  "__fbthrift_field_forward"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 3> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_recursive>::isset_indexes = { {
  0,
  -1,
  1,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_forward>::name = "structured_annotation_forward";
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_forward>::fields_names = { {
  "count"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_forward>::fields_ids = { {
  1,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_forward>::fields_types = { {
  TType::T_I64,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_forward>::storage_names = { {
  "__fbthrift_field_count"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_forward>::isset_indexes = { {
  0,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_nested>::name = "structured_annotation_nested";
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_nested>::fields_names = { {
  "name"sv,
  "nest"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_nested>::fields_ids = { {
  1,
  2,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_nested>::fields_types = { {
  TType::T_STRING,
  TType::T_STRUCT,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_nested>::storage_names = { {
  "__fbthrift_field_name"sv,
  "__fbthrift_field_nest"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::structured_annotation_nested>::isset_indexes = { {
  0,
  1,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::test::fixtures::basic-structured-annotations::MyStruct>::name = "MyStruct";
THRIFT_DATA_MEMBER const std::array<std::string_view, 4> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyStruct>::fields_names = { {
  "annotated_field"sv,
  "annotated_type"sv,
  "annotated_recursive"sv,
  "annotated_nested"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 4> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyStruct>::fields_ids = { {
  1,
  2,
  3,
  4,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 4> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyStruct>::fields_types = { {
  TType::T_I64,
  TType::T_STRING,
  TType::T_STRING,
  TType::T_I64,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 4> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyStruct>::storage_names = { {
  "__fbthrift_field_annotated_field"sv,
  "__fbthrift_field_annotated_type"sv,
  "__fbthrift_field_annotated_recursive"sv,
  "__fbthrift_field_annotated_nested"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 4> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyStruct>::isset_indexes = { {
  0,
  1,
  2,
  3,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::test::fixtures::basic-structured-annotations::MyException>::name = "MyException";
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyException>::fields_names = { {
  "context"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyException>::fields_ids = { {
  1,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyException>::fields_types = { {
  TType::T_STRING,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyException>::storage_names = { {
  "__fbthrift_field_context"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 1> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyException>::isset_indexes = { {
  0,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::test::fixtures::basic-structured-annotations::MyUnion>::name = "MyUnion";
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyUnion>::fields_names = { {
  "first"sv,
  "second"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyUnion>::fields_ids = { {
  1,
  2,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyUnion>::fields_types = { {
  TType::T_STRING,
  TType::T_I64,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyUnion>::storage_names = { {
  "first"sv,
  "second"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 2> TStructDataStorage<::test::fixtures::basic-structured-annotations::MyUnion>::isset_indexes = { {
  0,
  1,
}};

} // namespace thrift
} // namespace apache
