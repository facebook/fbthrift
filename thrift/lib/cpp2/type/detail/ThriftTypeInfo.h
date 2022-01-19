/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <any>
#include <typeinfo>

#include <folly/CPortability.h>
#include <folly/lang/Exception.h>
#include <thrift/lib/cpp2/op/Clear.h>
#include <thrift/lib/cpp2/op/Compare.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache {
namespace thrift {
namespace type {
namespace detail {

// Runtime type information for a Thrift type.
struct ThriftTypeInfo {
  const AnyType thriftType;
  const std::type_info& cppType;

  // ThriftTypeInfo type-erased ~v-table.
  bool (*empty)(const void*);
  bool (*identical)(const void*, const ThriftTypeInfo&, const void*);
  void (*clear)(void*);

  // Helpers.
  template <typename T>
  constexpr T* try_as(void* ptr) const noexcept {
    return cppType == typeid(T) ? static_cast<T*>(ptr) : nullptr;
  }
  template <typename T>
  const T* try_as(const void* ptr) const noexcept {
    return cppType == typeid(T) ? static_cast<const T*>(ptr) : nullptr;
  }
  template <typename T, typename V = void>
  decltype(auto) as(V* ptr) const {
    if (auto* tptr = try_as<T>(ptr)) {
      return *tptr;
    }
    folly::throw_exception<std::bad_any_cast>();
  }
};

// Type erased Thrift operator implementations.
template <typename Tag, typename T>
struct TypeErasedOp {
  static T& ref(void* ptr) { return *static_cast<T*>(ptr); }
  static const T& ref(const void* ptr) { return *static_cast<const T*>(ptr); }
  static bool empty(const void* ptr) { return op::isEmpty<Tag>(ref(ptr)); }
  static void clear(void* ptr) { op::clear<Tag>(ref(ptr)); }

  static bool identical(
      const void* lhs, const ThriftTypeInfo& rhsInfo, const void* rhs) {
    // Caller should have already checked the types match.
    assert(rhsInfo.thriftType == Tag{});
    if (rhsInfo.cppType == typeid(T)) {
      return op::identical<Tag>(ref(lhs), ref(rhs));
    }
    // TODO(afuller): Convert to the standard type and perform the comparsion
    // using that.
    folly::throw_exception<std::bad_any_cast>();
  }
};

// Specialization for the singleton type void_t.
template <typename T>
struct TypeErasedOp<void_t, T> {
  static bool empty(const void*) { return true; }
  static bool identical(const void*, const ThriftTypeInfo&, const void*) {
    return true;
  }
  static void clear(void*) {}
};

template <typename Tag, typename T>
FOLLY_EXPORT const ThriftTypeInfo& getTypeInfoImpl() {
  using Op = TypeErasedOp<Tag, T>;
  static const auto* kValue = new ThriftTypeInfo{
      Tag{},
      typeid(T),
      &Op::empty,
      &Op::identical,
      &Op::clear,
  };
  return *kValue;
}

template <typename Tag, typename T = native_type<Tag>>
const ThriftTypeInfo& getTypeInfo() {
  return getTypeInfoImpl<Tag, std::decay_t<T>>();
}

} // namespace detail
} // namespace type
} // namespace thrift
} // namespace apache
