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

// A reference to an unqualified field of the possibly const-qualified type T in
// a Thrift-generated struct.
template <typename T>
class field_ref {
 public:
  using value_type = T;

  THRIFT_NOLINK field_ref(T& value, detail::is_set_t<T>& is_set) noexcept
      : value_(value), is_set_(is_set) {}

  THRIFT_NOLINK operator field_ref<const T>() noexcept {
    return {value_, is_set_};
  }

  template <typename U = T>
  THRIFT_NOLINK std::enable_if_t<std::is_assignable<T&, U>::value, field_ref&>
  operator=(U&& value) noexcept(std::is_nothrow_assignable<T&, U>::value) {
    value_ = std::forward<U>(value);
    is_set_ = true;
    return *this;
  }

  THRIFT_NOLINK bool is_set() const noexcept {
    return is_set_;
  }

  // Returns a reference to the value.
  THRIFT_NOLINK T& value() const noexcept {
    return value_;
  }

  THRIFT_NOLINK T& operator*() const noexcept {
    return value_;
  }

  THRIFT_NOLINK T* operator->() const noexcept {
    return &value_;
  }

 private:
  T& value_;
  detail::is_set_t<T>& is_set_;
};

// A reference to an optional field of the possibly const-qualified type T in a
// Thrift-generated struct.
template <typename T>
class optional_field_ref {
 public:
  using value_type = T;

  THRIFT_NOLINK optional_field_ref(
      T& value,
      detail::is_set_t<T>& is_set) noexcept
      : value_(value), is_set_(is_set) {}

  THRIFT_NOLINK operator optional_field_ref<const T>() noexcept {
    return {value_, is_set_};
  }

  template <typename U = T>
  THRIFT_NOLINK
      std::enable_if_t<std::is_assignable<T&, U>::value, optional_field_ref&>
      operator=(U&& value) noexcept(std::is_nothrow_assignable<T&, U>::value) {
    value_ = std::forward<U>(value);
    is_set_ = true;
    return *this;
  }

  THRIFT_NOLINK bool has_value() const noexcept {
    return is_set_;
  }

  THRIFT_NOLINK explicit operator bool() const noexcept {
    return is_set_;
  }

  THRIFT_NOLINK void reset() noexcept {
    value_ = T();
    is_set_ = false;
  }

  // Returns a reference to the value if this optional_field_ref has one; throws
  // bad_field_access otherwise.
  THRIFT_NOLINK T& value() const {
    if (FOLLY_UNLIKELY(!is_set_)) {
      detail::throw_on_bad_field_access();
    }
    return value_;
  }

  // Returns a reference to the value without checking whether it is available.
  THRIFT_NOLINK T& value_unchecked() const {
    return value_;
  }

  THRIFT_NOLINK T& operator*() const {
    return value();
  }

  THRIFT_NOLINK T* operator->() const {
    return &value();
  }

 private:
  T& value_;
  detail::is_set_t<T>& is_set_;
};

} // namespace thrift
} // namespace apache
