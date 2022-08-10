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

#include <memory>
#include <stdexcept>
#include <type_traits>

#include <folly/ConstexprMath.h>
#include <folly/lang/Exception.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/detail/TypeInfo.h>

namespace apache {
namespace thrift {
namespace type {
namespace detail {

const TypeInfo& voidTypeInfo();

// A pointer for a type that has sufficent alignment to store information
// in the lower bits.
//
// TODO(afuller): Moved to a shared locations, so this can be used to track
// 'ownership' as well.
template <typename T, size_t Bits = folly::constexpr_log2(alignof(T))>
class AlignedPtr {
 public:
  static_assert(
      Bits > 0 && Bits <= folly::constexpr_log2(alignof(T)),
      "insufficent alignment");
  constexpr static std::uintptr_t kMask = ~std::uintptr_t{} << Bits;

  constexpr AlignedPtr() noexcept = default;
  /* implicit */ constexpr AlignedPtr(T* ptr, std::uintptr_t bits = {}) noexcept
      : ptr_((folly::bit_cast<std::uintptr_t>(ptr) & kMask) | (bits & ~kMask)) {
    assert((bits & kMask) == 0); // Programming error.
    // Never happens because of T's alignment.
    assert((folly::bit_cast<std::uintptr_t>(ptr) & ~kMask) == 0);
  }

  T* get() const noexcept { return reinterpret_cast<T*>(ptr_ & kMask); }

  template <size_t Bit>
  constexpr bool get() const noexcept {
    return ptr_ & bitMask<Bit>();
  }

  template <size_t Bit>
  constexpr void set() noexcept {
    ptr_ |= bitMask<Bit>();
  }

  template <size_t Bit>
  constexpr void clear() noexcept {
    ptr_ &= ~bitMask<Bit>();
  }

 private:
  std::uintptr_t ptr_ = {};

  template <size_t Bit>
  constexpr static auto bitMask() noexcept {
    static_assert(Bit < Bits, "out of range");
    return std::uintptr_t{1} << Bit;
  }
};

// The type information associated with a runtime Thrift value.
//
// C++ only tracks const- and rvalue-ness at compiletime, so type-erased value
// wrappers must track it at runtime.
//
// This class only stores a single AlignedPtr, so should be passed by value.
class RuntimeType {
 public:
  RuntimeType() noexcept {}
  explicit RuntimeType(const TypeInfo& info) noexcept : info_(&info) {}
  RuntimeType(
      const TypeInfo& info, bool isConst, bool isRvalue = false) noexcept
      : info_(&info, (isConst << kConst) | (isRvalue << kRvalue)) {}

  bool isConst() const noexcept { return info_.template get<kConst>(); }
  bool isRvalue() const noexcept { return info_.template get<kRvalue>(); }
  const TypeInfo& info() const noexcept { return *info_.get(); }

  // Throws if const.
  const TypeInfo& mut() const { return (ensureMut(), info()); }

  // Returns the appropriate runtime type, based on the given context.
  //
  // The runtime type is const, if either the context or type is const.
  // The runtime type is r-value, if both the context and the type are r-value.
  RuntimeType mergeQualifiers(bool ctxConst, bool ctxRvalue = false) const {
    return {info(), isConst() || ctxConst, isRvalue() && ctxRvalue};
  }

  const TypeInfo* operator->() const noexcept { return &info(); }

 private:
  // Stash the runtime qualifer information in the TypeInfo pointer, as
  // we know it has sufficent alignment.
  enum Qualifier { kConst, kRvalue, kQualSize };
  AlignedPtr<const TypeInfo, kQualSize> info_ = &voidTypeInfo();

  void ensureMut() const {
    if (isConst()) {
      folly::throw_exception<std::logic_error>("cannot modify a const ref");
    }
  }
};

// A type-erased, qualifier-preserving pointer to a Thrift value.
struct Ptr {
  RuntimeType type;
  void* ptr = nullptr;

  // Bindings.
  template <typename T>
  constexpr const T& as() const {
    return type->as<T>(ptr);
  }
  template <typename T>
  constexpr const T* tryAs() const {
    return type->tryAs<T>(ptr);
  }
  template <typename T>
  constexpr T& mut() const {
    return type.mut().as<T>(ptr);
  }
  template <typename T>
  constexpr T* tryMut() const {
    return type.isConst() ? nullptr : type->tryAs<T>(ptr);
  }
  bool empty() const { return type->empty(ptr); }
  bool identical(const Ptr& rhs) const {
    return type->thriftType == rhs.type->thriftType &&
        type->identical(ptr, rhs);
  }
  void clear() const { type.mut().clear(ptr); }

  void append(const Ptr& val) const { type.mut().append(ptr, val); }
  bool add(const Ptr& val) const { return type.mut().add(ptr, val); }
  bool put(const Ptr& key, const Ptr& val) const {
    return type.mut().put(ptr, {}, &key, val);
  }
  bool put(FieldId id, const Ptr& val) const {
    return type.mut().put(ptr, id, nullptr, val);
  }

  Ptr mergeQuals(bool ctxConst, bool ctxRvalue = false) const {
    return {type.mergeQualifiers(ctxConst, ctxRvalue), ptr};
  }

  // Gets the given field or entry, taking into account the context in
  // which the value is being accessed.
  Ptr get(
      FieldId id,
      const Ptr* key,
      bool ctxConst = false,
      bool ctxRvalue = false) const {
    return type->get(mergeQuals(ctxConst, ctxRvalue), id, key);
  }
};

// A base impl that throws for every op.
struct BaseErasedOp {
  [[noreturn]] static void bad_op(const char* msg = "not supported") {
    folly::throw_exception<std::logic_error>(msg);
  }
  [[noreturn]] static void bad_type() {
    folly::throw_exception<std::bad_any_cast>();
  }
  [[noreturn]] static void unimplemented(const char* msg = "not implemented") {
    folly::throw_exception<std::runtime_error>(msg);
  }

  [[noreturn]] static bool empty(const void*) { bad_op(); }
  [[noreturn]] static void clear(void*) { bad_op(); }
  [[noreturn]] static void append(void*, const Ptr&) { bad_op(); }
  [[noreturn]] static bool add(void*, const Ptr&) { bad_op(); }
  [[noreturn]] static bool put(void*, FieldId, const Ptr*, const Ptr&) {
    bad_op();
  }
  [[noreturn]] static Ptr get(Ptr, FieldId, const Ptr*) { bad_op(); }
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
