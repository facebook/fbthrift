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

#include <folly/Optional.h>
#include <folly/Portability.h>
#include <type_traits>
#include <utility>

namespace apache {
namespace thrift {

/**
 * DeprecatedOptionalField is used for thrift optional field with "optionals"
 * turned on. It will be eventually replaced by thrift::optional_field_ref.
 */
template <typename T>
class DeprecatedOptionalField : public folly::Optional<T> {
 private:
  using Base = folly::Optional<T>;
  const Base& toFolly() const {
    return *this;
  }

 public:
  using Base::Base;

  // Use perfect forwarding to hide Base::operator=(Base&&),
  // Otherwise `s.foo = {};` will be ambiguous between
  // default copy ctor and Base::operator=(Base&&).
  template <class U>
  auto& operator=(U&& u) noexcept(
      noexcept(std::declval<DeprecatedOptionalField>().Base::operator=(
          std::forward<U>(u)))) {
    Base::operator=(std::forward<U>(u));
    return *this;
  }

#define THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(OP)              \
  template <class U>                                                       \
  bool operator OP(const DeprecatedOptionalField<U>& rhs) const {          \
    return toFolly() OP rhs.toFolly();                                     \
  }                                                                        \
  template <class U>                                                       \
  friend bool operator OP(                                                 \
      const folly::Optional<U>& lhs, const DeprecatedOptionalField& rhs) { \
    return lhs OP rhs.toFolly();                                           \
  }                                                                        \
  template <class U>                                                       \
  bool operator OP(const folly::Optional<U>& rhs) const {                  \
    return toFolly() OP rhs;                                               \
  }
  THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(==)
  THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(!=)
  THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(<)
  THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(<=)
  THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(>)
  THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(>=)
#undef THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP
};

template <class T>
folly::Optional<T> castToFolly(const DeprecatedOptionalField<T>& t) {
  return t;
}

template <class T>
folly::Optional<T> castToFolly(DeprecatedOptionalField<T>&& t) {
  return std::move(t);
}

template <class T>
folly::Optional<T> castToFollyOrForward(const DeprecatedOptionalField<T>& t) {
  return t;
}

template <class T>
folly::Optional<T> castToFollyOrForward(DeprecatedOptionalField<T>&& t) {
  return std::move(t);
}

template <class T>
T&& castToFollyOrForward(T&& t) {
  return std::forward<T>(t);
}

template <class T>
auto&& fromFollyOptional(
    DeprecatedOptionalField<T>& lhs,
    const folly::Optional<T>& rhs) {
  if (rhs) {
    lhs = *rhs;
  } else {
    lhs.reset();
  }
  return lhs;
}

template <class T>
auto&& fromFollyOptional(
    DeprecatedOptionalField<T>& lhs,
    folly::Optional<T>&& rhs) {
  if (rhs) {
    lhs = std::move(*rhs);
  } else {
    lhs.reset();
  }

  return lhs;
}

// TODO: remove rvalue overloads if no one use them
template <class T>
auto&& fromFollyOptional(
    DeprecatedOptionalField<T>&& lhs,
    const folly::Optional<T>& rhs) {
  if (rhs) {
    lhs = *rhs;
  } else {
    lhs.reset();
  }
  return std::move(lhs);
}

template <class T>
auto&& fromFollyOptional(
    DeprecatedOptionalField<T>&& lhs,
    folly::Optional<T>&& rhs) {
  if (rhs) {
    lhs = std::move(*rhs);
  } else {
    lhs.reset();
  }
  return std::move(lhs);
}

} // namespace thrift
} // namespace apache

FOLLY_NAMESPACE_STD_BEGIN
template <class T>
struct hash<apache::thrift::DeprecatedOptionalField<T>>
    : hash<folly::Optional<T>> {};
FOLLY_NAMESPACE_STD_END
