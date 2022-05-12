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

#include <bitset>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include <folly/CPortability.h>
#include <folly/lang/Exception.h>
#include <thrift/lib/cpp2/type/NativeType.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/detail/ThriftTypeInfo.h>

namespace apache {
namespace thrift {
namespace type {

// A light weight (pass-by-value), non-owning reference to a Thrift value.
//
// Should typically be passed by value as it only holds two
// ponters; a pointer to the value being reference and a pointer to the static
// runtime metadata associated with the type of the value.
class AnyRef {
 public:
  constexpr AnyRef() noexcept = default;

  template <typename Tag, typename T>
  constexpr static AnyRef create(T&& value) {
    return {Tag{}, std::forward<T>(value)};
  }

  // Rebinds the AnyRef to another value (or void).
  template <typename Tag, typename T = native_type<Tag>>
  void reset(T&& value);
  void reset() noexcept { info_ = &getAnyRefInfo<void_t>(); }

  // Returns true iff the referenced value is 'empty', and not serialized in a
  // 'terse' context.
  bool empty() const { return info_->type.empty(ptr_); }
  // Returns true iff the referenced value is identical to the given value.
  bool identical(const AnyRef& rhs) const {
    return info_->type.thriftType == rhs.info_->type.thriftType &&
        info_->type.identical(ptr_, rhs.info_->type, rhs.ptr_);
  }
  // Sets the referenced value to it's intrinsic default (e.g. ignoring custom
  // field defaults).
  void clear() { ensureMutable().clear(ptr_); }

  // Type accessors.
  constexpr const Type& type() const { return info_->type.thriftType; }
  constexpr const std::type_info& typeId() const { return info_->type.cppType; }

  // Type-safe value accessors.
  template <typename Tag>
  constexpr const native_type<Tag>& as() const {
    return info_->type.as<native_type<Tag>>(ptr_);
  }
  template <typename Tag>
  constexpr native_type<Tag>& as_mut() {
    ensureMutable();
    return info_->type.as<native_type<Tag>>(ptr_);
  }
  template <typename Tag>
  constexpr const native_type<Tag>* try_as() const noexcept {
    return info_->type.try_as<native_type<Tag>>(ptr_);
  }

 private:
  enum Qualifier {
    Const,
    Rvalue,

    QualifierSize,
  };

  // TODO(viz): Embed the qualiferes bits in the pointers instead of using
  // AnyRefInfo, which adds an extra layer of lazy singletons.
  struct AnyRefInfo {
    const detail::ThriftTypeInfo& type;
    std::bitset<QualifierSize> is;
  };

  const AnyRefInfo* info_ = &getAnyRefInfo<void_t>();
  void* ptr_ = nullptr;

  template <typename Tag, typename T = native_type<Tag>>
  FOLLY_EXPORT static const AnyRefInfo& getAnyRefInfo();

  template <typename Tag, typename T>
  constexpr AnyRef(Tag, T&& value)
      : info_(&getAnyRefInfo<Tag, T>()),
        ptr_(const_cast<std::decay_t<T>*>(&value)) {}

  const detail::ThriftTypeInfo& ensureMutable();
};

template <typename Tag, typename T>
void AnyRef::reset(T&& value) {
  info_ = getAnyRefInfo<Tag, T>();
  ptr_ = &value;
}

template <typename Tag, typename T>
FOLLY_EXPORT auto AnyRef::getAnyRefInfo() -> const AnyRefInfo& {
  static const AnyRefInfo kValue{
      detail::getTypeInfo<Tag, T>(),
      (std::is_rvalue_reference_v<T> << Rvalue) |
          (std::is_const_v<std::remove_reference_t<T>> << Const)};
  return kValue;
}

inline const detail::ThriftTypeInfo& AnyRef::ensureMutable() {
  if (info_->is[Const]) {
    folly::throw_exception<std::logic_error>("cannot modify a const ref");
  }
  return info_->type;
}

} // namespace type
} // namespace thrift
} // namespace apache
