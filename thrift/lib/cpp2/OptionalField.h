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
class DeprecatedOptionalField : private folly::Optional<T> {
 private:
  using Base = folly::Optional<T>;
  const Base& toFolly() const {
    return *this;
  }

 public:
  using Base::operator->;
  using Base::operator*;
  using Base::operator bool;
  using Base::assign;
  using Base::emplace;
  using Base::get_pointer;
  using Base::has_value;
  using Base::reset;
  using Base::value;
  using Base::value_or;
  using typename Base::value_type;

  DeprecatedOptionalField() = default;
  DeprecatedOptionalField(const DeprecatedOptionalField&) = default;
  DeprecatedOptionalField(DeprecatedOptionalField&&) = default;
  explicit DeprecatedOptionalField(const T& t) noexcept(
      std::is_nothrow_copy_constructible<T>::value)
      : Base(t) {}
  explicit DeprecatedOptionalField(T&& t) noexcept(
      std::is_nothrow_move_constructible<T>::value)
      : Base(std::move(t)) {}

  DeprecatedOptionalField& operator=(const DeprecatedOptionalField&) = default;
  DeprecatedOptionalField& operator=(DeprecatedOptionalField&&) = default;

  auto&& operator=(const T& other) noexcept(
      std::is_nothrow_copy_assignable<T>::value) {
    Base::operator=(other);
    return *this;
  }

  auto&& operator=(T&& other) noexcept(
      std::is_nothrow_move_assignable<T>::value) {
    Base::operator=(std::move(other));
    return *this;
  }
};

namespace detail {

template <class T>
bool has_value(const DeprecatedOptionalField<T>& a) {
  return a.has_value();
}
template <class T>
bool has_value(const folly::Optional<T>& a) {
  return a.has_value();
}
template <class T>
bool has_value(const T&) {
  return true;
}
template <class T>
const auto& value(const DeprecatedOptionalField<T>& a) {
  return *a;
}
template <class T>
const auto& value(const folly::Optional<T>& a) {
  return *a;
}
template <class T>
const auto& value(const T& a) {
  return a;
}

template <class U, class V, class Comp>
bool compare(const U& a, const V& b, Comp comp) {
  if (has_value(a) != has_value(b)) {
    return comp(has_value(a), has_value(b));
  }
  if (has_value(a)) {
    return comp(value(a), value(b));
  }
  return comp(0, 0);
}
} // namespace detail

#define THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(OP)                 \
  template <class U, class V>                                                 \
  bool operator OP(                                                           \
      const DeprecatedOptionalField<U>& a,                                    \
      const DeprecatedOptionalField<V>& b) {                                  \
    return detail::compare(a, b, [](auto&& x, auto&& y) { return x OP y; });  \
  }                                                                           \
  template <class U, class V>                                                 \
  bool operator OP(                                                           \
      const folly::Optional<U>& a, const DeprecatedOptionalField<V>& b) {     \
    return detail::compare(a, b, [](auto&& x, auto&& y) { return x OP y; });  \
  }                                                                           \
  template <class U, class V>                                                 \
  bool operator OP(                                                           \
      const DeprecatedOptionalField<U>& a, const folly::Optional<V>& b) {     \
    return detail::compare(a, b, [](auto&& x, auto&& y) { return x OP y; });  \
  }                                                                           \
  template <class U, class V>                                                 \
  bool operator OP(const U& a, const DeprecatedOptionalField<V>& b) {         \
    return detail::compare(a, b, [](auto&& x, auto&& y) { return x OP y; });  \
  }                                                                           \
  template <class U, class V>                                                 \
  bool operator OP(const DeprecatedOptionalField<U>& a, const V& b) {         \
    return detail::compare(a, b, [](auto&& x, auto&& y) { return x OP y; });  \
  }                                                                           \
  template <class T>                                                          \
  bool operator OP(const folly::None&, const DeprecatedOptionalField<T>& t) { \
    return false OP t.has_value();                                            \
  }                                                                           \
  template <class T>                                                          \
  bool operator OP(const DeprecatedOptionalField<T>& t, const folly::None&) { \
    return t.has_value() OP false;                                            \
  }
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(==)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(!=)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(<)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(<=)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(>)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(>=)
#undef THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP

template <class T>
folly::Optional<T> castToFolly(const DeprecatedOptionalField<T>& t) {
  if (t) {
    return *t;
  }
  return {};
}

template <class T>
folly::Optional<T> castToFolly(DeprecatedOptionalField<T>&& t) {
  if (t) {
    return std::move(*t);
  }
  return {};
}

template <class T>
folly::Optional<T> castToFollyOrForward(const DeprecatedOptionalField<T>& t) {
  return castToFolly(t);
}

template <class T>
folly::Optional<T> castToFollyOrForward(DeprecatedOptionalField<T>& t) {
  return castToFolly(t);
}

template <class T>
folly::Optional<T> castToFollyOrForward(DeprecatedOptionalField<T>&& t) {
  return castToFolly(std::move(t));
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

} // namespace thrift
} // namespace apache

FOLLY_NAMESPACE_STD_BEGIN
template <class T>
struct hash<apache::thrift::DeprecatedOptionalField<T>> {
  size_t operator()(
      apache::thrift::DeprecatedOptionalField<T> const& obj) const {
    if (!obj.has_value()) {
      return 0;
    }
    return hash<remove_const_t<T>>()(*obj);
  }
};
FOLLY_NAMESPACE_STD_END
