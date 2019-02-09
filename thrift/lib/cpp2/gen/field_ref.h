/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <type_traits>
#include <utility>

#include <folly/CPortability.h>
#include <folly/Likely.h>

// Let the optimizer remove these artificial functions completely.
#define THRIFT_NOLINK FOLLY_ALWAYS_INLINE FOLLY_ATTR_VISIBILITY_HIDDEN

namespace apache {
namespace thrift {
namespace detail {

template <typename T>
using is_set_t = std::conditional_t<std::is_const<T>::value, const bool, bool>;

[[noreturn]] void throw_on_bad_field_access();

} // namespace detail

// A reference to an unqualified field of the possibly const-qualified type
// std::remove_reference_t<T> in a Thrift-generated struct.
template <typename T>
class field_ref {
  static_assert(std::is_reference<T>::value, "not a reference");

 public:
  using value_type = std::remove_reference_t<T>;
  using reference_type = T;

  THRIFT_NOLINK field_ref(
      reference_type value,
      detail::is_set_t<value_type>& is_set) noexcept
      : value_(value), is_set_(is_set) {}

  THRIFT_NOLINK operator field_ref<const value_type&>() noexcept {
    return {value_, is_set_};
  }

  // Conversion from value_type&& to const value_type&&.
  template <
      typename U,
      std::enable_if_t<
          std::is_same<T, value_type&&>() && std::is_same<U, value_type>(),
          int> = 0>
  THRIFT_NOLINK operator field_ref<const U&&>() noexcept {
    return {value_, is_set_};
  }

  template <typename U = value_type>
  THRIFT_NOLINK
      std::enable_if_t<std::is_assignable<value_type&, U>::value, field_ref&>
      operator=(U&& value) noexcept(
          std::is_nothrow_assignable<value_type&, U>::value) {
    value_ = std::forward<U>(value);
    is_set_ = true;
    return *this;
  }

  // Assignment from field_ref is intentionally not provided to prevent
  // potential confusion between two possible behaviors, copying and reference
  // rebinding. The copy_from method is provided instead.
  template <typename U>
  THRIFT_NOLINK void copy_from(field_ref<U> other) noexcept(
      std::is_nothrow_assignable<value_type&, U>::value) {
    value_ = other.value();
    is_set_ = other.is_set();
  }

  THRIFT_NOLINK bool is_set() const noexcept {
    return is_set_;
  }

  // Returns a reference to the value.
  THRIFT_NOLINK reference_type value() const noexcept {
    return std::forward<reference_type>(value_);
  }

  THRIFT_NOLINK reference_type operator*() const noexcept {
    return std::forward<reference_type>(value_);
  }

  THRIFT_NOLINK value_type* operator->() const noexcept {
    return &value_;
  }

 private:
  value_type& value_;
  detail::is_set_t<value_type>& is_set_;
};

// A reference to an optional field of the possibly const-qualified type
// std::remove_reference_t<T> in a Thrift-generated struct.
template <typename T>
class optional_field_ref {
  static_assert(std::is_reference<T>::value, "not a reference");

 public:
  using value_type = std::remove_reference_t<T>;
  using reference_type = T;

 private:
  using pointee_type = std::conditional_t<
      std::is_same<T, value_type&&>{},
      const value_type,
      value_type>;

 public:
  THRIFT_NOLINK optional_field_ref(
      reference_type value,
      detail::is_set_t<value_type>& is_set) noexcept
      : value_(value), is_set_(is_set) {}

  THRIFT_NOLINK operator optional_field_ref<const value_type&>() noexcept {
    return {value_, is_set_};
  }

  // Conversion from value_type&& to const value_type&&.
  template <
      typename U,
      std::enable_if_t<
          std::is_same<T, value_type&&>() && std::is_same<U, value_type>(),
          int> = 0>
  THRIFT_NOLINK operator optional_field_ref<const U&&>() noexcept {
    return {value_, is_set_};
  }

  template <typename U = value_type>
  THRIFT_NOLINK std::enable_if_t<
      std::is_assignable<value_type&, U>::value,
      optional_field_ref&>
  operator=(U&& value) noexcept(
      std::is_nothrow_assignable<value_type&, U>::value) {
    value_ = std::forward<U>(value);
    is_set_ = true;
    return *this;
  }

  // Assignment from optional_field_ref is intentionally not provided to prevent
  // potential confusion between two possible behaviors, copying and reference
  // rebinding. The copy_from method is provided instead.
  template <typename U>
  THRIFT_NOLINK void copy_from(const optional_field_ref<U>& other) noexcept(
      std::is_nothrow_assignable<value_type&, U>::value) {
    value_ = other.value_unchecked();
    is_set_ = other.has_value();
  }

  THRIFT_NOLINK bool has_value() const noexcept {
    return is_set_;
  }

  THRIFT_NOLINK explicit operator bool() const noexcept {
    return is_set_;
  }

  THRIFT_NOLINK void reset() noexcept {
    value_ = value_type();
    is_set_ = false;
  }

  // Returns a reference to the value if this optional_field_ref has one; throws
  // bad_field_access otherwise.
  THRIFT_NOLINK reference_type value() const {
    if (FOLLY_UNLIKELY(!is_set_)) {
      detail::throw_on_bad_field_access();
    }
    return std::forward<reference_type>(value_);
  }

  template <typename U>
  THRIFT_NOLINK value_type value_or(U&& default_value) const {
    return is_set_ ? value_
                   : static_cast<value_type>(std::forward<U>(default_value));
  }

  // Returns a reference to the value without checking whether it is available.
  THRIFT_NOLINK reference_type value_unchecked() const {
    return std::forward<reference_type>(value_);
  }

  THRIFT_NOLINK reference_type operator*() const {
    return value();
  }

  THRIFT_NOLINK pointee_type* operator->() const {
    return &static_cast<pointee_type&>(value());
  }

 private:
  value_type& value_;
  detail::is_set_t<value_type>& is_set_;
};

} // namespace thrift
} // namespace apache
