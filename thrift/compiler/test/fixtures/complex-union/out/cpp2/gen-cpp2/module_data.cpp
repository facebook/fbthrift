/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/complex-union/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */

#include "thrift/compiler/test/fixtures/complex-union/gen-cpp2/module_data.h"

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

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::cpp2::ComplexUnion>::name = "ComplexUnion";
THRIFT_DATA_MEMBER const std::array<std::string_view, 6> TStructDataStorage<::cpp2::ComplexUnion>::fields_names = { {
  "intValue"sv,
  "stringValue"sv,
  "intListValue"sv,
  "stringListValue"sv,
  "typedefValue"sv,
  "stringRef"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 6> TStructDataStorage<::cpp2::ComplexUnion>::fields_ids = { {
  1,
  5,
  2,
  3,
  9,
  14,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 6> TStructDataStorage<::cpp2::ComplexUnion>::fields_types = { {
  TType::T_I64,
  TType::T_STRING,
  TType::T_LIST,
  TType::T_LIST,
  TType::T_MAP,
  TType::T_STRING,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 6> TStructDataStorage<::cpp2::ComplexUnion>::storage_names = { {
  "intValue"sv,
  "stringValue"sv,
  "intListValue"sv,
  "stringListValue"sv,
  "typedefValue"sv,
  "stringRef"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 6> TStructDataStorage<::cpp2::ComplexUnion>::isset_indexes = { {
  0,
  1,
  2,
  3,
  4,
  -1,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::cpp2::ListUnion>::name = "ListUnion";
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::cpp2::ListUnion>::fields_names = { {
  "intListValue"sv,
  "stringListValue"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 2> TStructDataStorage<::cpp2::ListUnion>::fields_ids = { {
  2,
  3,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 2> TStructDataStorage<::cpp2::ListUnion>::fields_types = { {
  TType::T_LIST,
  TType::T_LIST,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::cpp2::ListUnion>::storage_names = { {
  "intListValue"sv,
  "stringListValue"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 2> TStructDataStorage<::cpp2::ListUnion>::isset_indexes = { {
  0,
  1,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::cpp2::DataUnion>::name = "DataUnion";
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::cpp2::DataUnion>::fields_names = { {
  "binaryData"sv,
  "stringData"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 2> TStructDataStorage<::cpp2::DataUnion>::fields_ids = { {
  1,
  2,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 2> TStructDataStorage<::cpp2::DataUnion>::fields_types = { {
  TType::T_STRING,
  TType::T_STRING,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::cpp2::DataUnion>::storage_names = { {
  "binaryData"sv,
  "stringData"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 2> TStructDataStorage<::cpp2::DataUnion>::isset_indexes = { {
  0,
  1,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::cpp2::Val>::name = "Val";
THRIFT_DATA_MEMBER const std::array<std::string_view, 3> TStructDataStorage<::cpp2::Val>::fields_names = { {
  "strVal"sv,
  "intVal"sv,
  "typedefValue"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 3> TStructDataStorage<::cpp2::Val>::fields_ids = { {
  1,
  2,
  9,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 3> TStructDataStorage<::cpp2::Val>::fields_types = { {
  TType::T_STRING,
  TType::T_I32,
  TType::T_MAP,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 3> TStructDataStorage<::cpp2::Val>::storage_names = { {
  "__fbthrift_field_strVal"sv,
  "__fbthrift_field_intVal"sv,
  "__fbthrift_field_typedefValue"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 3> TStructDataStorage<::cpp2::Val>::isset_indexes = { {
  0,
  1,
  2,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::cpp2::ValUnion>::name = "ValUnion";
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::cpp2::ValUnion>::fields_names = { {
  "v1"sv,
  "v2"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 2> TStructDataStorage<::cpp2::ValUnion>::fields_ids = { {
  1,
  2,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 2> TStructDataStorage<::cpp2::ValUnion>::fields_types = { {
  TType::T_STRUCT,
  TType::T_STRUCT,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::cpp2::ValUnion>::storage_names = { {
  "v1"sv,
  "v2"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 2> TStructDataStorage<::cpp2::ValUnion>::isset_indexes = { {
  0,
  1,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::cpp2::VirtualComplexUnion>::name = "VirtualComplexUnion";
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::cpp2::VirtualComplexUnion>::fields_names = { {
  "thingOne"sv,
  "thingTwo"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 2> TStructDataStorage<::cpp2::VirtualComplexUnion>::fields_ids = { {
  1,
  2,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 2> TStructDataStorage<::cpp2::VirtualComplexUnion>::fields_types = { {
  TType::T_STRING,
  TType::T_STRING,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::cpp2::VirtualComplexUnion>::storage_names = { {
  "thingOne"sv,
  "thingTwo"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 2> TStructDataStorage<::cpp2::VirtualComplexUnion>::isset_indexes = { {
  0,
  1,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::cpp2::NonCopyableStruct>::name = "NonCopyableStruct";
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::cpp2::NonCopyableStruct>::fields_names = { {
  "num"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 1> TStructDataStorage<::cpp2::NonCopyableStruct>::fields_ids = { {
  1,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 1> TStructDataStorage<::cpp2::NonCopyableStruct>::fields_types = { {
  TType::T_I64,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::cpp2::NonCopyableStruct>::storage_names = { {
  "__fbthrift_field_num"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 1> TStructDataStorage<::cpp2::NonCopyableStruct>::isset_indexes = { {
  0,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::cpp2::NonCopyableUnion>::name = "NonCopyableUnion";
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::cpp2::NonCopyableUnion>::fields_names = { {
  "s"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 1> TStructDataStorage<::cpp2::NonCopyableUnion>::fields_ids = { {
  1,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 1> TStructDataStorage<::cpp2::NonCopyableUnion>::fields_types = { {
  TType::T_STRUCT,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 1> TStructDataStorage<::cpp2::NonCopyableUnion>::storage_names = { {
  "s"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 1> TStructDataStorage<::cpp2::NonCopyableUnion>::isset_indexes = { {
  0,
}};

} // namespace thrift
} // namespace apache
