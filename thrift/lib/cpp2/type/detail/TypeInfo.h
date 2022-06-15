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

struct Ptr;

// Runtime type information for a Thrift type.
struct TypeInfo {
  const Type thriftType;
  const std::type_info& cppType;

  // Type-erased ~v-table.
  // TODO(afuller): Consider merging some of these functions to reduce size.
  bool (*empty)(const void*);
  bool (*identical)(const void*, const Ptr&);
  void (*clear)(void*);
  void (*append)(void*, const Ptr&);
  bool (*add)(void*, const Ptr&);
  bool (*put)(void*, FieldId, const Ptr*, const Ptr&);
  Ptr (*get)(Ptr, FieldId, const Ptr*);

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

// A type-erased, qualifier-preserving pointer to a Thrift value.
struct Ptr {
  template <typename Tag, typename T = native_type<Tag>>
  static Ptr create(T&& val) {
    // Note: const safety is validated at runtime.
    return {
        &getTypeInfo<Tag, T>(),
        const_cast<std::decay_t<T>*>(&val),
        std::is_const_v<std::remove_reference_t<T>>,
        std::is_rvalue_reference_v<T>};
  }

  const TypeInfo* info = &getTypeInfo<type::void_t>();
  void* ptr = nullptr;
  // TODO(afuller): Embed the qualifiers bits into the pointer's high bits.
  bool isConst : 1;
  bool isRvalue : 1;

  // Throws if reference is const.
  constexpr void ensureMut() const {
    if (isConst) {
      folly::throw_exception<std::logic_error>("cannot modify a const ref");
    }
  }

  // Provides 'const' access to TypeInfo.
  constexpr const TypeInfo& type() const { return *info; }

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
    return isConst ? nullptr : type().tryAs<T>(ptr);
  }
  bool empty() const { return type().empty(ptr); }
  bool identical(const Ptr& rhs) const {
    return type().thriftType == rhs.type().thriftType &&
        type().identical(ptr, rhs);
  }
  void clear() const { mutType().clear(ptr); }

  void append(const Ptr& val) const { mutType().append(ptr, val); }
  bool add(const Ptr& val) const { return mutType().add(ptr, val); }
  bool put(const Ptr& key, const Ptr& val) const {
    return mutType().put(ptr, {}, &key, val);
  }
  bool put(FieldId id, const Ptr& val) const {
    return mutType().put(ptr, id, nullptr, val);
  }

  Ptr mergeQuals(bool ctxConst, bool ctxRvalue) const {
    return {info, ptr, isConst || ctxConst, isRvalue && ctxRvalue};
  }

  // Gets the given field or entry, taking into account the context in
  // which the value is being accessed.
  Ptr get(
      FieldId id,
      const Ptr* key,
      bool ctxConst = false,
      bool ctxRvalue = false) const {
    return type().get(mergeQuals(ctxConst, ctxRvalue), id, key);
  }
};

// Type erased Thrift operator implementations.
template <typename Tag, typename T>
struct TypeErasedOp {
  static T& ref(void* ptr) { return *static_cast<T*>(ptr); }
  static const T& ref(const void* ptr) { return *static_cast<const T*>(ptr); }
  static bool empty(const void* ptr) { return op::isEmpty<Tag>(ref(ptr)); }
  static void clear(void* ptr) { op::clear<Tag>(ref(ptr)); }

  static bool identical(const void* lhs, const Ptr& rhs) {
    // Caller should have already checked the types match.
    assert(rhs.type().thriftType == Tag{});
    if (rhs.type().cppType == typeid(T)) {
      return op::identical<Tag>(ref(lhs), ref(rhs.ptr));
    }
    // TODO(afuller): Convert to the standard type and perform the comparsion
    // using that.
    folly::throw_exception<std::bad_any_cast>();
  }

  static void append(void*, const Ptr&) {
    // TODO(afuller): Implement.
    folly::throw_exception<std::runtime_error>("not implemented");
  }
  static bool add(void*, const Ptr&) {
    // TODO(afuller): Implement.
    folly::throw_exception<std::runtime_error>("not implemented");
  }
  static bool put(void*, FieldId, const Ptr*, const Ptr&) {
    // TODO(afuller): Implement.
    folly::throw_exception<std::runtime_error>("not implemented");
  }
  static Ptr get(Ptr, FieldId, const Ptr*) {
    // TODO(afuller): Implement type-erased 'get' access.
    folly::throw_exception<std::runtime_error>("not implemented");
  }
};

// Specialization for the singleton type void_t.
template <typename T>
struct TypeErasedOp<void_t, T> {
  static bool empty(const void*) { return true; }
  static bool identical(const void*, const Ptr&) { return true; }
  static void clear(void*) {}
  static void append(void*, const Ptr&) {
    folly::throw_exception<std::out_of_range>("void does not support 'append'");
  }
  static bool add(void*, const Ptr&) {
    folly::throw_exception<std::out_of_range>("void does not support 'add'");
  }
  static bool put(void*, FieldId, const Ptr*, const Ptr&) {
    folly::throw_exception<std::out_of_range>("void does not support 'put'");
  }
  static Ptr get(Ptr, FieldId, const Ptr*) {
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
