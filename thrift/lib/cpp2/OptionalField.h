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
#include <folly/Traits.h>
#include <thrift/lib/cpp2/FieldRef.h>
#include <type_traits>
#include <utility>

namespace apache {
namespace thrift {

/**
 * DeprecatedOptionalField is used for thrift optional field with "optionals"
 * turned on. It will be eventually replaced by thrift::optional_field_ref.
 */
template <typename T>
class DeprecatedOptionalField {
  static_assert(
      !std::is_reference<T>::value,
      "Optional may not be used with reference types");

 public:
  using value_type = T;

  DeprecatedOptionalField() = default;
  DeprecatedOptionalField(const DeprecatedOptionalField&) = default;
  DeprecatedOptionalField(DeprecatedOptionalField&& other) noexcept(
      std::is_nothrow_move_constructible<T>::value)
      : value_(std::move(other.value_)), is_set_(other.is_set_) {
    other.is_set_ = false; // TODO: do not unset moved-from Optional
  }

  explicit DeprecatedOptionalField(const T& t) noexcept(
      std::is_nothrow_copy_constructible<T>::value)
      : value_(t), is_set_(true) {}

  explicit DeprecatedOptionalField(T&& t) noexcept(
      std::is_nothrow_move_constructible<T>::value)
      : value_(std::move(t)), is_set_(true) {}

  DeprecatedOptionalField& operator=(const DeprecatedOptionalField&) = default;
  DeprecatedOptionalField& operator=(DeprecatedOptionalField&& other) noexcept(
      std::is_nothrow_move_assignable<T>::value) {
    if (this != &other) {
      value_ = std::move(other.value_);
      is_set_ = other.is_set_;
      other.is_set_ = false; // TODO: do not unset moved-from Optional
    }
    return *this;
  }

  auto&& operator=(const T& other) noexcept(
      std::is_nothrow_copy_assignable<T>::value) {
    is_set_ = true;
    value_ = other;
    return *this;
  }

  auto&& operator=(T&& other) noexcept(
      std::is_nothrow_move_assignable<T>::value) {
    is_set_ = true;
    value_ = std::move(other);
    return *this;
  }

  void copy_from(optional_field_ref<const T&> other) {
    if (other) {
      value_ = *other;
    }
    is_set_ = other.has_value();
  }

  void move_from(optional_field_ref<T&&> other) {
    if (other) {
      value_ = std::move(*other);
    }
    is_set_ = other.has_value();
  }

  void copy_from(optional_field_ref<T&&> other) = delete;
  void copy_from(DeprecatedOptionalField&& other) = delete;

  auto&& value() const& {
    require_value();
    return value_;
  }
  auto&& value() & {
    require_value();
    return value_;
  }
  auto&& value() && {
    require_value();
    return std::move(value_);
  }
  auto&& value() const&& {
    require_value();
    return std::move(value_);
  }

  auto&& operator*() const& {
    return value();
  }
  auto&& operator*() & {
    return value();
  }
  auto&& operator*() const&& {
    return std::move(value());
  }
  auto&& operator*() && {
    return std::move(value());
  }

  bool has_value() const noexcept {
    return is_set_;
  }
  explicit operator bool() const noexcept {
    return has_value();
  }

  const T* operator->() const {
    return &value();
  }
  T* operator->() {
    return &value();
  }

  void reset() noexcept {
    // Thrift field's destructor shouldn't have side-effect, thus we could just
    // clear the memory.
    is_set_ = false;
    value_ = T();
  }

  template <class... Args>
  T& emplace(Args&&... args) {
    is_set_ = true;
    return value_ = T(std::forward<Args>(args)...);
  }

  template <class U>
  T& emplace(std::initializer_list<U> ilist) {
    is_set_ = true;
    return value_ = T(ilist);
  }

  template <class U>
  T value_or(U&& dflt) const& {
    if (has_value()) {
      return value_;
    }
    return std::forward<U>(dflt);
  }
  template <class U>
  T value_or(U&& dflt) && {
    if (has_value()) {
      return std::move(value_);
    }
    return std::forward<U>(dflt);
  }

  // TODO: codemod following incompatbile APIs
  void assign(T&& u) {
    *this = std::move(u);
  }

  void assign(const T& u) {
    *this = u;
  }

  operator optional_field_ref<T&>() & {
    return {value_, is_set_};
  }

  operator optional_field_ref<const T&>() const& {
    return {value_, is_set_};
  }

  operator optional_field_ref<T&&>() && {
    return {std::move(value_), is_set_};
  }

  operator optional_field_ref<const T&&>() const&& {
    return {std::move(value_), is_set_};
  }

 private:
  void require_value() const {
    if (!has_value()) {
      folly::throw_exception<folly::OptionalEmptyException>();
    }
  }

  T value_{};
  bool is_set_ = false;
};

namespace detail {

template <class T>
bool has_value(const DeprecatedOptionalField<T>& a) {
  return a.has_value();
}
template <class T>
bool has_value(optional_field_ref<T> a) {
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
const auto& value(optional_field_ref<T> a) {
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

#define THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(OP)                \
  template <class T>                                                         \
  bool operator OP(                                                          \
      const DeprecatedOptionalField<T>& a,                                   \
      const DeprecatedOptionalField<T>& b) {                                 \
    return detail::compare(a, b, [](auto&& x, auto&& y) { return x OP y; }); \
  }                                                                          \
  template <class U, class V>                                                \
  bool operator OP(const U& a, const DeprecatedOptionalField<V>& b) {        \
    return detail::compare(a, b, [](auto&& x, auto&& y) { return x OP y; }); \
  }                                                                          \
  template <class U, class V>                                                \
  bool operator OP(const DeprecatedOptionalField<U>& a, const V& b) {        \
    return detail::compare(a, b, [](auto&& x, auto&& y) { return x OP y; }); \
  }
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(==)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(!=)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(<)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(<=)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(>)
THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP(>=)
#undef THRIFT_DETAIL_DEPRECATED_OPTIONAL_FIELD_DEFINE_OP

#ifdef THRIFT_HAS_OPTIONAL
template <class T>
bool operator==(const DeprecatedOptionalField<T>& a, std::nullopt_t) {
  return !a.has_value();
}
template <class T>
bool operator==(std::nullopt_t, const DeprecatedOptionalField<T>& a) {
  return !a.has_value();
}
template <class T>
bool operator!=(const DeprecatedOptionalField<T>& a, std::nullopt_t) {
  return a.has_value();
}
template <class T>
bool operator!=(std::nullopt_t, const DeprecatedOptionalField<T>& a) {
  return a.has_value();
}
#endif

template <class T>
folly::Optional<T> copyToFollyOptional(const DeprecatedOptionalField<T>& t) {
  if (t) {
    return *t;
  }
  return {};
}

template <class T>
folly::Optional<T> moveToFollyOptional(DeprecatedOptionalField<T>&& t) {
  if (t) {
    return std::move(*t);
  }
  return {};
}

template <class T>
folly::Optional<T> moveToFollyOptional(DeprecatedOptionalField<T>& t) {
  return moveToFollyOptional(std::move(t));
}

template <class T>
void fromFollyOptional(
    DeprecatedOptionalField<T>& lhs,
    const folly::Optional<T>& rhs) {
  if (rhs) {
    lhs = *rhs;
  } else {
    lhs.reset();
  }
}

template <class T>
void fromFollyOptional(
    DeprecatedOptionalField<T>& lhs,
    folly::Optional<T>&& rhs) {
  if (rhs) {
    lhs = std::move(*rhs);
  } else {
    lhs.reset();
  }
}

template <class T>
[[deprecated(
    "Use std::optional with optional_field_ref::to_optional() instead")]] folly::
    Optional<std::remove_const_t<T>>
    copyToFollyOptional(optional_field_ref<T&> t) {
  if (t) {
    return *t;
  }
  return {};
}

template <class T>
[[deprecated(
    "Use std::optional with optional_field_ref::to_optional() instead")]] folly::
    Optional<std::remove_const_t<T>>
    moveToFollyOptional(optional_field_ref<T&&> t) {
  if (t) {
    return std::move(*t);
  }
  return {};
}

template <class T>
[[deprecated(
    "Use std::optional with optional_field_ref::to_optional() instead")]] folly::
    Optional<T>
    moveToFollyOptional(optional_field_ref<T&> t) {
  if (t) {
    return std::move(*t);
  }
  return {};
}

template <class T>
[[deprecated(
    "Use std::optional with optional_field_ref::from_optional(...) instead")]] auto
fromFollyOptional(optional_field_ref<T&> lhs, const folly::Optional<T>& rhs) {
  if (rhs) {
    lhs = *rhs;
  } else {
    lhs.reset();
  }
  return lhs;
}

template <class T>
[[deprecated(
    "Use std::optional with optional_field_ref::from_optional(...) instead")]] auto
fromFollyOptional(optional_field_ref<T&> lhs, folly::Optional<T>&& rhs) {
  if (rhs) {
    lhs = std::move(*rhs);
  } else {
    lhs.reset();
  }

  return lhs;
}

template <class T>
bool equalToFollyOptional(
    optional_field_ref<T> a,
    const folly::Optional<folly::remove_cvref_t<T>>& b) {
  return a && b ? *a == *b : a.has_value() == b.has_value();
}

template <class T>
bool equalToFollyOptional(
    const DeprecatedOptionalField<T>& a,
    const folly::Optional<T>& b) {
  return a && b ? *a == *b : a.has_value() == b.has_value();
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
