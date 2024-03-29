/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/field_interceptor/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include "thrift/compiler/test/fixtures/field_interceptor/gen-cpp2/module_metadata.h"
#include <thrift/lib/cpp2/visitation/for_each.h>

namespace apache {
namespace thrift {
namespace detail {

template <>
struct ForEachField<::facebook::thrift::test::InterceptedFields> {
  template <typename F, typename... T>
  void operator()([[maybe_unused]] F&& f, [[maybe_unused]] T&&... t) const {
    f(0, static_cast<T&&>(t).access_field_ref()...);
    f(1, static_cast<T&&>(t).access_shared_field_ref()...);
    f(2, static_cast<T&&>(t).access_optional_shared_field_ref()...);
    f(3, static_cast<T&&>(t).access_shared_const_field_ref()...);
    f(4, static_cast<T&&>(t).access_optional_shared_const_field_ref()...);
    f(5, static_cast<T&&>(t).access_optional_boxed_field_ref()...);
  }
};
} // namespace detail
} // namespace thrift
} // namespace apache
