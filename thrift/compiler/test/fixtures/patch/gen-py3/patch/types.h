/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#pragma once

#include <functional>
#include <folly/Range.h>

#include <thrift/lib/py3/enums.h>
#include "thrift/compiler/test/fixtures/patch/gen-cpp2/patch_data.h"
#include "thrift/compiler/test/fixtures/patch/gen-cpp2/patch_types.h"
#include "thrift/compiler/test/fixtures/patch/gen-cpp2/patch_metadata.h"
namespace thrift {
namespace py3 {



template<>
inline void reset_field<::apache::thrift::op::GeneratePatch>(
    ::apache::thrift::op::GeneratePatch& obj, uint16_t index) {
  switch (index) {
  }
}

template<>
inline void reset_field<::apache::thrift::op::AssignOnlyPatch>(
    ::apache::thrift::op::AssignOnlyPatch& obj, uint16_t index) {
  switch (index) {
  }
}

template<>
inline const std::unordered_map<std::string_view, std::string_view>& PyStructTraits<
    ::apache::thrift::op::GeneratePatch>::namesmap() {
  static const folly::Indestructible<NamesMap> map {
    {
    }
  };
  return *map;
}

template<>
inline const std::unordered_map<std::string_view, std::string_view>& PyStructTraits<
    ::apache::thrift::op::AssignOnlyPatch>::namesmap() {
  static const folly::Indestructible<NamesMap> map {
    {
    }
  };
  return *map;
}
} // namespace py3
} // namespace thrift
