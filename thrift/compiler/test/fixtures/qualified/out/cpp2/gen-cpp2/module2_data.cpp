/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/qualified/src/module2.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */

#include "thrift/compiler/test/fixtures/qualified/gen-cpp2/module2_data.h"

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

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::module2::Struct>::name = "Struct";
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::module2::Struct>::fields_names = { {
  "first"sv,
  "second"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 2> TStructDataStorage<::module2::Struct>::fields_ids = { {
  1,
  2,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 2> TStructDataStorage<::module2::Struct>::fields_types = { {
  TType::T_STRUCT,
  TType::T_STRUCT,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::module2::Struct>::storage_names = { {
  "__fbthrift_field_first"sv,
  "__fbthrift_field_second"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 2> TStructDataStorage<::module2::Struct>::isset_indexes = { {
  0,
  1,
}};

THRIFT_DATA_MEMBER const std::string_view TStructDataStorage<::module2::BigStruct>::name = "BigStruct";
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::module2::BigStruct>::fields_names = { {
  "s"sv,
  "id"sv,
}};
THRIFT_DATA_MEMBER const std::array<int16_t, 2> TStructDataStorage<::module2::BigStruct>::fields_ids = { {
  1,
  2,
}};
THRIFT_DATA_MEMBER const std::array<protocol::TType, 2> TStructDataStorage<::module2::BigStruct>::fields_types = { {
  TType::T_STRUCT,
  TType::T_I32,
}};
THRIFT_DATA_MEMBER const std::array<std::string_view, 2> TStructDataStorage<::module2::BigStruct>::storage_names = { {
  "__fbthrift_field_s"sv,
  "__fbthrift_field_id"sv,
}};
THRIFT_DATA_MEMBER const std::array<int, 2> TStructDataStorage<::module2::BigStruct>::isset_indexes = { {
  0,
  1,
}};

} // namespace thrift
} // namespace apache
