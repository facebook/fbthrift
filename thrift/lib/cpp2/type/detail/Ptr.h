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

#include <stdexcept>

#include <folly/lang/Exception.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/detail/TypeInfo.h>

namespace apache {
namespace thrift {
namespace type {
namespace detail {

const TypeInfo& voidTypeInfo();

// A type-erased, qualifier-preserving pointer to a Thrift value.
struct Ptr {
  const TypeInfo* info = &voidTypeInfo();
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

// A base impl that throws for every op.
struct BaseErasedOp {
  [[noreturn]] static void bad_op(const char* msg) {
    folly::throw_exception<std::logic_error>(msg);
  }
  [[noreturn]] static void bad_type() {
    folly::throw_exception<std::bad_any_cast>();
  }
  [[noreturn]] static void unimplemented(const char* msg = "not implemented") {
    folly::throw_exception<std::runtime_error>(msg);
  }

  [[noreturn]] static bool empty(const void*) {
    bad_op("'empty' is not supported");
  }
  [[noreturn]] static void clear(void*) { bad_op("'clear' is not supported"); }
  [[noreturn]] static void append(void*, const Ptr&) {
    bad_op("'append' is not supported");
  }
  [[noreturn]] static bool add(void*, const Ptr&) {
    bad_op("'add' is not supported");
  }
  [[noreturn]] static bool put(void*, FieldId, const Ptr*, const Ptr&) {
    bad_op("'clear' is not supported");
  }
  [[noreturn]] static Ptr get(Ptr, FieldId, const Ptr*) {
    bad_op("'get' is not supported");
  }
};

// The ops for the empty type 'void'.
struct VoidErasedOp : BaseErasedOp {
  static bool empty(const void*) { return true; }
  static bool identical(const void*, const Ptr&) { return true; }
  static void clear(void*) {}
};

inline const TypeInfo& voidTypeInfo() {
  return getTypeInfo<VoidErasedOp, void_t>();
}

} // namespace detail
} // namespace type
} // namespace thrift
} // namespace apache
