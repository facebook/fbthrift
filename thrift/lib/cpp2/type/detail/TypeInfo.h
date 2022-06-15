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
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Type.h>

namespace apache {
namespace thrift {
namespace type {
namespace detail {

struct TypeInfo; // Forward declare.
struct Ref; // Forward declare.

// Runtime reference information for a Thrift reference.
//
// TODO(viz): Embed the qualifiers bits into a pointer in Ref instead of using
// RefInfo, which adds an extra layer of lazy singletons.
struct RefInfo {
  enum Qualifier {
    Const,
    Rvalue,

    QualifierSize,
  };
  using QualSet = std::bitset<QualifierSize>;

  const TypeInfo& type;
  // TODO(afuller): Use `bool isConst : 1;` instead.
  QualSet is;
};

// Runtime type information for a Thrift type.
struct TypeInfo {
  const Type thriftType;
  const std::type_info& cppType;

  // Type-erased ~v-table.
  // TODO(afuller): Consider merging some of these functions to reduce size.
  bool (*empty)(const void*);
  bool (*identical)(const void*, const Ref&);
  void (*clear)(void*);
  void (*append)(void*, const Ref&);
  bool (*add)(void*, const Ref&);
  bool (*put)(void*, FieldId, const Ref*, const Ref&);
  Ref (*get)(void*, const RefInfo::QualSet&, FieldId, const Ref*);

  // Type-safe, const-preserving casting functions.
  template <typename T>
  constexpr T* tryAs(void* ptr) const noexcept {
    return cppType == typeid(T) ? static_cast<T*>(ptr) : nullptr;
  }
  template <typename T>
  const T* tryAs(const void* ptr) const noexcept {
    return cppType == typeid(T) ? static_cast<const T*>(ptr) : nullptr;
  }
  template <typename T, typename V = void>
  decltype(auto) as(V* ptr) const {
    if (auto* tptr = tryAs<T>(ptr)) {
      return *tptr;
    }
    folly::throw_exception<std::bad_any_cast>();
  }
};

template <typename Tag, typename T>
FOLLY_EXPORT const TypeInfo& getTypeInfoImpl();

template <typename Tag = type::void_t, typename T = native_type<Tag>>
const TypeInfo& getTypeInfo() {
  return getTypeInfoImpl<Tag, std::decay_t<T>>();
}

template <typename Tag, typename T = native_type<Tag>>
FOLLY_EXPORT const RefInfo& getRefInfo() {
  static const RefInfo kValue{
      getTypeInfo<Tag, T>(),
      (std::is_rvalue_reference_v<T> << RefInfo::Rvalue) |
          (std::is_const_v<std::remove_reference_t<T>> << RefInfo::Const)};
  return kValue;
}

// A re-bindable, type-erased reference to a Thrift value.
// TODO(afuller): Rename to ~Ptr as it actually provides pointer semantics.
struct Ref {
  const RefInfo* info = &getRefInfo<type::void_t>();
  void* ptr = nullptr;

  template <typename Tag, typename T = native_type<Tag>>
  static Ref create(T&& val) {
    // Note: const safety is preserved in the RefInfo, and validated at runtime.
    return {&getRefInfo<Tag, T>(), const_cast<std::decay_t<T>*>(&val)};
  }

  // Throws if reference is const.
  constexpr void ensureMut() const {
    if (info->is[RefInfo::Const]) {
      folly::throw_exception<std::logic_error>("cannot modify a const ref");
    }
  }

  // Provides 'const' access to TypeInfo.
  constexpr const TypeInfo& type() const { return info->type; }

  // Provides 'mut' access to TypeInfo.
  constexpr const TypeInfo& mutType() const { return (ensureMut(), type()); }

  // Bindings.
  template <typename T>
  constexpr const T& as() const {
    return type().as<T>(ptr);
  }
  template <typename T>
  constexpr const T* tryAs() const {
    return type().tryAs<T>(ptr);
  }
  template <typename T>
  constexpr T& mut() const {
    return mutType().as<T>(ptr);
  }
  template <typename T>
  constexpr T* tryMut() const {
    return info->is[RefInfo::Const] ? nullptr : type().tryAs<T>(ptr);
  }
  bool empty() const { return type().empty(ptr); }
  bool identical(const Ref& rhs) const {
    return type().thriftType == rhs.type().thriftType &&
        type().identical(ptr, rhs);
  }
  void clear() const { mutType().clear(ptr); }
  void append(const Ref& val) const { mutType().append(ptr, val); }
  bool add(const Ref& val) const { return mutType().add(ptr, val); }
  bool put(const Ref& key, const Ref& val) const {
    return mutType().put(ptr, {}, &key, val);
  }
  bool put(FieldId id, const Ref& val) const {
    return mutType().put(ptr, id, nullptr, val);
  }

  Ref get(FieldId id, const Ref* key) const {
    return type().get(ptr, info->is, id, key);
  }
};

// Type erased Thrift operator implementations.
template <typename Tag, typename T>
struct TypeErasedOp {
  static T& ref(void* ptr) { return *static_cast<T*>(ptr); }
  static const T& ref(const void* ptr) { return *static_cast<const T*>(ptr); }
  static bool empty(const void* ptr) { return op::isEmpty<Tag>(ref(ptr)); }
  static void clear(void* ptr) { op::clear<Tag>(ref(ptr)); }

  static bool identical(const void* lhs, const Ref& rhs) {
    // Caller should have already checked the types match.
    assert(rhs.type().thriftType == Tag{});
    if (rhs.type().cppType == typeid(T)) {
      return op::identical<Tag>(ref(lhs), ref(rhs.ptr));
    }
    // TODO(afuller): Convert to the standard type and perform the comparsion
    // using that.
    folly::throw_exception<std::bad_any_cast>();
  }

  static void append(void*, const Ref&) {
    // TODO(afuller): Implement.
    folly::throw_exception<std::runtime_error>("not implemented");
  }
  static bool add(void*, const Ref&) {
    // TODO(afuller): Implement.
    folly::throw_exception<std::runtime_error>("not implemented");
  }
  static bool put(void*, FieldId, const Ref*, const Ref&) {
    // TODO(afuller): Implement.
    folly::throw_exception<std::runtime_error>("not implemented");
  }
  static Ref get(void*, const RefInfo::QualSet&, FieldId, const Ref*) {
    // TODO(afuller): Implement type-erased 'get' access.
    folly::throw_exception<std::runtime_error>("not implemented");
  }
};

// Specialization for the singleton type void_t.
template <typename T>
struct TypeErasedOp<void_t, T> {
  static bool empty(const void*) { return true; }
  static bool identical(const void*, const Ref&) { return true; }
  static void clear(void*) {}
  static void append(void*, const Ref&) {
    folly::throw_exception<std::out_of_range>("void does not support 'append'");
  }
  static bool add(void*, const Ref&) {
    folly::throw_exception<std::out_of_range>("void does not support 'add'");
  }
  static bool put(void*, FieldId, const Ref*, const Ref&) {
    folly::throw_exception<std::out_of_range>("void does not support 'put'");
  }
  static Ref get(void*, const RefInfo::QualSet&, FieldId, const Ref*) {
    folly::throw_exception<std::out_of_range>("void does not support 'get'");
  }
};

template <typename Tag, typename T>
FOLLY_EXPORT const TypeInfo& getTypeInfoImpl() {
  using Op = TypeErasedOp<Tag, T>;
  static const auto* kValue = new TypeInfo{
      Tag{},
      typeid(T),
      &Op::empty,
      &Op::identical,
      &Op::clear,
      &Op::append,
      &Op::add,
      &Op::put,
      &Op::get,
  };
  return *kValue;
}

} // namespace detail
} // namespace type
} // namespace thrift
} // namespace apache
