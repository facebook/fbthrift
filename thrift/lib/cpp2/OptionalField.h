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

namespace apache {
namespace thrift {

/**
 * DeprecatedOptionalField is used for thrift optional field with "optionals"
 * turned on. It will be eventually replaced by thrift::optional_field_ref.
 */
template <typename T>
class DeprecatedOptionalField : public folly::Optional<T> {
 private:
  const folly::Optional<T>& toFolly() const {
    return *this;
  }

 public:
  using folly::Optional<T>::Optional;
  using folly::Optional<T>::operator=;

  template <typename L, typename R>
  friend bool operator==(
      const DeprecatedOptionalField<L>& lhs,
      const DeprecatedOptionalField<R>& rhs);
  template <typename L, typename R>
  friend bool operator!=(
      const DeprecatedOptionalField<L>& lhs,
      const DeprecatedOptionalField<R>& rhs);
  template <typename U>
  friend bool operator==(
      const DeprecatedOptionalField<U>& lhs,
      const folly::Optional<U>& rhs);
  template <typename U>
  friend bool operator==(
      const folly::Optional<U>& lhs,
      const DeprecatedOptionalField<U>& rhs);
  template <typename U>
  friend bool operator!=(
      const DeprecatedOptionalField<U>& lhs,
      const folly::Optional<U>& rhs);
  template <typename U>
  friend bool operator!=(
      const folly::Optional<U>& lhs,
      const DeprecatedOptionalField<U>& rhs);
};

template <typename L, typename R>
bool operator==(
    const DeprecatedOptionalField<L>& lhs,
    const DeprecatedOptionalField<R>& rhs) {
  return lhs.toFolly() == rhs.toFolly();
}
template <typename L, typename R>
bool operator!=(
    const DeprecatedOptionalField<L>& lhs,
    const DeprecatedOptionalField<R>& rhs) {
  return lhs.toFolly() != rhs.toFolly();
}
template <typename U>
bool operator==(
    const DeprecatedOptionalField<U>& lhs,
    const folly::Optional<U>& rhs) {
  return lhs.toFolly() == rhs;
}
template <typename U>
bool operator==(
    const folly::Optional<U>& lhs,
    const DeprecatedOptionalField<U>& rhs) {
  return lhs == rhs.toFolly();
}
template <typename U>
bool operator!=(
    const DeprecatedOptionalField<U>& lhs,
    const folly::Optional<U>& rhs) {
  return lhs.toFolly() != rhs;
}
template <typename U>
bool operator!=(
    const folly::Optional<U>& lhs,
    const DeprecatedOptionalField<U>& rhs) {
  return lhs != rhs.toFolly();
}

template <class T>
folly::Optional<T> castToFolly(const DeprecatedOptionalField<T>& t) {
  return t;
}

} // namespace thrift
} // namespace apache

FOLLY_NAMESPACE_STD_BEGIN
template <class T>
struct hash<apache::thrift::DeprecatedOptionalField<T>>
    : hash<folly::Optional<T>> {};
FOLLY_NAMESPACE_STD_END
