/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <folly/Poly.h>
#include <thrift/lib/cpp2/type/AnyType.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/Traits.h>
#include <thrift/lib/cpp2/type/detail/AnyValue.h>

namespace apache::thrift::type {

// A type-erased Thrift value.
class AnyValue {
 public:
  constexpr AnyValue() noexcept = default;

  template <typename Tag, typename... Args>
  static AnyValue create(Args&&... args);

  constexpr const AnyType& type() const { return type_; }

  // Type safe access to the internal storage.
  // Throws folly::BadPolyCast if the types are incompatible.
  template <typename Tag>
  native_type<Tag>& as() &;
  template <typename Tag>
  native_type<Tag>&& as() &&;
  template <typename Tag>
  const native_type<Tag>& as() const&;
  template <typename Tag>
  const native_type<Tag>&& as() const&&;

  bool empty() const noexcept { return value_.empty(); }
  void clear() noexcept { value_.clear(); }

  // Throws folly::BadPolyCast if the underlying types are incompatible.
  bool identical(const AnyValue& other) const {
    return type_ == other.type_ && value_.identical(other.value_);
  }

 private:
  AnyValue(AnyType type, detail::AnyValueHolder value)
      : type_(std::move(type)), value_(std::move(value)) {}

  AnyType type_;
  detail::AnyValueHolder value_ = detail::VoidValueData{};
};

///////
// Implemnation Details

template <typename Tag, typename... Args>
AnyValue AnyValue::create(Args&&... args) {
  return {
      AnyType::create<Tag>(),
      detail::AnyValueData<Tag>{{std::forward<Args>(args)...}}};
}

template <typename Tag>
native_type<Tag>& AnyValue::as() & {
  return folly::poly_cast<detail::AnyValueData<Tag>&>(value_).data;
}
template <typename Tag>
native_type<Tag>&& AnyValue::as() && {
  return std::move(folly::poly_cast<detail::AnyValueData<Tag>&>(value_).data);
}
template <typename Tag>
const native_type<Tag>& AnyValue::as() const& {
  return folly::poly_cast<const detail::AnyValueData<Tag>&>(value_).data;
}
template <typename Tag>
const native_type<Tag>&& AnyValue::as() const&& {
  return std::move(
      folly::poly_cast<const detail::AnyValueData<Tag>&>(value_).data);
}

} // namespace apache::thrift::type
