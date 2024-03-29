/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/lazy_deserialization/src/deprecated_terse_writes.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include "thrift/compiler/test/fixtures/lazy_deserialization/gen-cpp2/deprecated_terse_writes_metadata.h"
#include <thrift/lib/cpp2/visitation/for_each.h>

namespace apache {
namespace thrift {
namespace detail {

template <>
struct ForEachField<::apache::thrift::test::TerseFoo> {
  template <typename F, typename... T>
  void operator()([[maybe_unused]] F&& f, [[maybe_unused]] T&&... t) const {
    f(0, static_cast<T&&>(t).field1_ref()...);
    f(1, static_cast<T&&>(t).field2_ref()...);
    f(2, static_cast<T&&>(t).field3_ref()...);
    f(3, static_cast<T&&>(t).field4_ref()...);
  }
};

template <>
struct ForEachField<::apache::thrift::test::TerseLazyFoo> {
  template <typename F, typename... T>
  void operator()([[maybe_unused]] F&& f, [[maybe_unused]] T&&... t) const {
    f(0, static_cast<T&&>(t).field1_ref()...);
    f(1, static_cast<T&&>(t).field2_ref()...);
    f(2, static_cast<T&&>(t).field3_ref()...);
    f(3, static_cast<T&&>(t).field4_ref()...);
  }
};

template <>
struct ForEachField<::apache::thrift::test::TerseOptionalFoo> {
  template <typename F, typename... T>
  void operator()([[maybe_unused]] F&& f, [[maybe_unused]] T&&... t) const {
    f(0, static_cast<T&&>(t).field1_ref()...);
    f(1, static_cast<T&&>(t).field2_ref()...);
    f(2, static_cast<T&&>(t).field3_ref()...);
    f(3, static_cast<T&&>(t).field4_ref()...);
  }
};

template <>
struct ForEachField<::apache::thrift::test::TerseOptionalLazyFoo> {
  template <typename F, typename... T>
  void operator()([[maybe_unused]] F&& f, [[maybe_unused]] T&&... t) const {
    f(0, static_cast<T&&>(t).field1_ref()...);
    f(1, static_cast<T&&>(t).field2_ref()...);
    f(2, static_cast<T&&>(t).field3_ref()...);
    f(3, static_cast<T&&>(t).field4_ref()...);
  }
};
} // namespace detail
} // namespace thrift
} // namespace apache
