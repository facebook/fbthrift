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

#include <initializer_list>
#include <type_traits>
#include <utility>

#if (!defined(_MSC_VER) && __has_include(<optional>)) ||        \
    (defined(_MSC_VER) && __cplusplus >= 201703L)
#include <optional>
// Technically it should be 201606 but std::optional is present with 201603.
#if __cpp_lib_optional >= 201603
#define THRIFT_HAS_OPTIONAL
#endif
#endif

#include <folly/CPortability.h>
#include <folly/Likely.h>

namespace apache {
namespace thrift {
namespace detail {

template <typename T>
using is_set_t = std::conditional_t<std::is_const<T>::value, const bool, bool>;

[[noreturn]] void throw_on_bad_field_access();

} // namespace detail

template <typename T>
class DeprecatedOptionalField;

// A reference to an unqualified field of the possibly const-qualified type
// std::remove_reference_t<T> in a Thrift-generated struct.
template <typename T>
class field_ref {
  static_assert(std::is_reference<T>::value, "not a reference");

  template <typename U>
  friend class field_ref;

 public:
  using value_type = std::remove_reference_t<T>;
  using reference_type = T;

  FOLLY_ERASE field_ref(
      reference_type value,
      detail::is_set_t<value_type>& is_set) noexcept
      : value_(value), is_set_(is_set) {}

  template <
      typename U,
      std::enable_if_t<
          std::is_same<
              std::add_const_t<std::remove_reference_t<U>>,
              value_type>{} &&
              !(std::is_rvalue_reference<T>{} && std::is_lvalue_reference<U>{}),
          int> = 0>
  FOLLY_ERASE /* implicit */ field_ref(const field_ref<U>& other) noexcept
      : value_(other.value_), is_set_(other.is_set_) {}

  template <typename U = value_type>
  FOLLY_ERASE
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
  FOLLY_ERASE void copy_from(field_ref<U> other) noexcept(
      std::is_nothrow_assignable<value_type&, U>::value) {
    value_ = other.value();
    is_set_ = other.is_set();
  }

  // Returns true iff the field is set. field_ref doesn't provide conversion to
  // bool to avoid confusion between checking if the field is set and getting
  // the field's value, particularly for bool fields.
  FOLLY_ERASE bool has_value() const noexcept {
    return is_set_;
  }

  FOLLY_ERASE bool is_set() const noexcept {
    return is_set_;
  }

  // Returns a reference to the value.
  FOLLY_ERASE reference_type value() const noexcept {
    return std::forward<reference_type>(value_);
  }

  FOLLY_ERASE reference_type operator*() const noexcept {
    return std::forward<reference_type>(value_);
  }

  FOLLY_ERASE value_type* operator->() const noexcept {
    return &value_;
  }

  template <typename Index>
  FOLLY_ERASE auto operator[](const Index& index) const
      -> decltype(std::declval<reference_type>()[index]) {
    return value_[index];
  }

  template <typename... Args>
  FOLLY_ERASE value_type& emplace(Args&&... args) {
    value_ = value_type(static_cast<Args&&>(args)...);
    is_set_ = true;
    return value_;
  }

  template <class U, class... Args>
  FOLLY_ERASE std::enable_if_t<
      std::is_constructible<value_type, std::initializer_list<U>&, Args&&...>::
          value,
      value_type&>
  emplace(std::initializer_list<U> ilist, Args&&... args) {
    value_ = value_type(ilist, std::forward<Args>(args)...);
    is_set_ = true;
    return value_;
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

  template <typename U>
  friend class optional_field_ref;

 public:
  using value_type = std::remove_reference_t<T>;
  using reference_type = T;

 private:
  using pointee_type = std::conditional_t<
      std::is_rvalue_reference<T>{},
      const value_type,
      value_type>;

 public:
  FOLLY_ERASE optional_field_ref(
      reference_type value,
      detail::is_set_t<value_type>& is_set) noexcept
      : value_(value), is_set_(is_set) {}

  template <
      typename U,
      std::enable_if_t<
          std::is_same<
              std::add_const_t<std::remove_reference_t<U>>,
              value_type>{} &&
              !(std::is_rvalue_reference<T>{} && std::is_lvalue_reference<U>{}),
          int> = 0>
  FOLLY_ERASE /* implicit */ optional_field_ref(
      const optional_field_ref<U>& other) noexcept
      : value_(other.value_), is_set_(other.is_set_) {}

  template <
      typename U,
      std::enable_if_t<
          std::is_same<T, U&&>{} || std::is_same<T, const U&&>{},
          int> = 0>
  FOLLY_ERASE explicit optional_field_ref(
      const optional_field_ref<U&>& other) noexcept
      : value_(other.value_), is_set_(other.is_set_) {}

  template <typename U = value_type>
  FOLLY_ERASE std::enable_if_t<
      std::is_assignable<value_type&, U>::value,
      optional_field_ref&>
  operator=(U&& value) noexcept(
      std::is_nothrow_assignable<value_type&, U>::value) {
    value_ = std::forward<U>(value);
    is_set_ = true;
    return *this;
  }

  // Copies the data (the set flag and the value if available) from another
  // optional_field_ref object.
  //
  // Assignment from optional_field_ref is intentionally not provided to prevent
  // potential confusion between two possible behaviors, copying and reference
  // rebinding. This copy_from method is provided instead.
  template <typename U>
  FOLLY_ERASE void copy_from(const optional_field_ref<U>& other) noexcept(
      std::is_nothrow_assignable<value_type&, U>::value) {
    value_ = other.value_unchecked();
    is_set_ = other.has_value();
  }

  template <typename U>
  FOLLY_ERASE void move_from(optional_field_ref<U> other) noexcept(
      std::is_nothrow_assignable<value_type&, U&&>::value) {
    value_ = std::move(other.value_);
    is_set_ = other.is_set_;
  }

#ifdef THRIFT_HAS_OPTIONAL
  template <typename U>
  FOLLY_ERASE void from_optional(const std::optional<U>& other) noexcept(
      std::is_nothrow_assignable<value_type&, U>::value) {
    // Use if instead of a shorter ternary expression to prevent a potential
    // copy if T and U mismatch.
    if (other) {
      value_ = *other;
    } else {
      value_ = {};
    }
    is_set_ = other.has_value();
  }

  // Moves the value from std::optional. As std::optional's move constructor,
  // move_from doesn't make other empty.
  template <typename U>
  FOLLY_ERASE void from_optional(std::optional<U>&& other) noexcept(
      std::is_nothrow_assignable<value_type&, U>::value) {
    // Use if instead of a shorter ternary expression to prevent a potential
    // copy if T and U mismatch.
    if (other) {
      value_ = std::move(*other);
    } else {
      value_ = {};
    }
    is_set_ = other.has_value();
  }

  FOLLY_ERASE std::optional<value_type> to_optional() const {
    return is_set_ ? std::make_optional(value_) : std::nullopt;
  }
#endif

  FOLLY_ERASE bool has_value() const noexcept {
    return is_set_;
  }

  FOLLY_ERASE explicit operator bool() const noexcept {
    return is_set_;
  }

  FOLLY_ERASE void reset() noexcept {
    value_ = value_type();
    is_set_ = false;
  }

  // Returns a reference to the value if this optional_field_ref has one; throws
  // bad_field_access otherwise.
  FOLLY_ERASE reference_type value() const {
    if (FOLLY_UNLIKELY(!is_set_)) {
      detail::throw_on_bad_field_access();
    }
    return std::forward<reference_type>(value_);
  }

  template <typename U = value_type>
  FOLLY_ERASE value_type value_or(U&& default_value) const {
    return is_set_
        ? static_cast<value_type>(std::forward<reference_type>(value_))
        : static_cast<value_type>(std::forward<U>(default_value));
  }

  // Returns a reference to the value without checking whether it is available.
  FOLLY_ERASE reference_type value_unchecked() const {
    return std::forward<reference_type>(value_);
  }

  FOLLY_ERASE reference_type operator*() const {
    return value();
  }

  FOLLY_ERASE pointee_type* operator->() const {
    return &static_cast<pointee_type&>(value());
  }

  template <typename... Args>
  FOLLY_ERASE value_type& emplace(Args&&... args) {
    reset(); // C++ Standard requires *this to be empty if `emplace(...)` throws
    value_ = value_type(static_cast<Args&&>(args)...);
    is_set_ = true;
    return value_;
  }

  template <class U, class... Args>
  FOLLY_ERASE std::enable_if_t<
      std::is_constructible<value_type, std::initializer_list<U>&, Args&&...>::
          value,
      value_type&>
  emplace(std::initializer_list<U> ilist, Args&&... args) {
    reset();
    value_ = value_type(ilist, std::forward<Args>(args)...);
    is_set_ = true;
    return value_;
  }

  FOLLY_ERASE void copy_from(const DeprecatedOptionalField<value_type>& other) {
    copy_from(optional_field_ref<const value_type&>(other));
  }

  FOLLY_ERASE void move_from(DeprecatedOptionalField<value_type>&& other) {
    move_from(optional_field_ref<value_type&&>(std::move(other)));
  }

  FOLLY_ERASE void copy_from(DeprecatedOptionalField<value_type>&& other) =
      delete;

 private:
  value_type& value_;
  detail::is_set_t<value_type>& is_set_;
};

template <typename T1, typename T2>
bool operator==(optional_field_ref<T1> a, optional_field_ref<T2> b) {
  return a && b ? *a == *b : a.has_value() == b.has_value();
}

template <typename T1, typename T2>
bool operator!=(optional_field_ref<T1> a, optional_field_ref<T2> b) {
  return !(a == b);
}

template <typename T1, typename T2>
bool operator<(optional_field_ref<T1> a, optional_field_ref<T2> b) {
  if (a.has_value() != b.has_value()) {
    return a.has_value() < b.has_value();
  }
  return a ? *a < *b : false;
}

template <typename T1, typename T2>
bool operator>(optional_field_ref<T1> a, optional_field_ref<T2> b) {
  return b < a;
}

template <typename T1, typename T2>
bool operator<=(optional_field_ref<T1> a, optional_field_ref<T2> b) {
  return !(a > b);
}

template <typename T1, typename T2>
bool operator>=(optional_field_ref<T1> a, optional_field_ref<T2> b) {
  return !(a < b);
}

template <typename T>
bool operator==(
    optional_field_ref<T> a,
    const typename optional_field_ref<T>::value_type& b) {
  return a ? *a == b : false;
}

template <typename T>
bool operator!=(
    optional_field_ref<T> a,
    const typename optional_field_ref<T>::value_type& b) {
  return !(a == b);
}

template <typename T>
bool operator==(
    const typename optional_field_ref<T>::value_type& a,
    optional_field_ref<T> b) {
  return b == a;
}

template <typename T>
bool operator!=(
    const typename optional_field_ref<T>::value_type& a,
    optional_field_ref<T> b) {
  return b != a;
}

template <typename T>
bool operator<(
    optional_field_ref<T> a,
    const typename optional_field_ref<T>::value_type& b) {
  return a ? *a < b : true;
}

template <typename T>
bool operator>(
    optional_field_ref<T> a,
    const typename optional_field_ref<T>::value_type& b) {
  return a ? *a > b : false;
}

template <typename T>
bool operator<=(
    optional_field_ref<T> a,
    const typename optional_field_ref<T>::value_type& b) {
  return !(a > b);
}

template <typename T>
bool operator>=(
    optional_field_ref<T> a,
    const typename optional_field_ref<T>::value_type& b) {
  return !(a < b);
}

template <typename T>
bool operator<(
    const typename optional_field_ref<T>::value_type& a,
    optional_field_ref<T> b) {
  return b > a;
}

template <typename T>
bool operator<=(
    const typename optional_field_ref<T>::value_type& a,
    optional_field_ref<T> b) {
  return b >= a;
}

template <typename T>
bool operator>(
    const typename optional_field_ref<T>::value_type& a,
    optional_field_ref<T> b) {
  return b < a;
}

template <typename T>
bool operator>=(
    const typename optional_field_ref<T>::value_type& a,
    optional_field_ref<T> b) {
  return b <= a;
}

#ifdef THRIFT_HAS_OPTIONAL
template <class T>
bool operator==(const optional_field_ref<T>& a, std::nullopt_t) {
  return !a.has_value();
}
template <class T>
bool operator==(std::nullopt_t, const optional_field_ref<T>& a) {
  return !a.has_value();
}
template <class T>
bool operator!=(const optional_field_ref<T>& a, std::nullopt_t) {
  return a.has_value();
}
template <class T>
bool operator!=(std::nullopt_t, const optional_field_ref<T>& a) {
  return a.has_value();
}
#endif

namespace detail {

struct get_pointer_fn {
  template <class T>
  T* operator()(optional_field_ref<T&> field) const {
    return field ? &*field : nullptr;
  }
};

struct can_throw_fn {
  template <typename T>
  FOLLY_ERASE T&& operator()(T&& value) const {
    return static_cast<T&&>(value);
  }
};

} // namespace detail

constexpr detail::get_pointer_fn get_pointer;

//  can_throw
//
//  Used to annotate optional field accesses that can throw,
//  suppressing any linter warning about unchecked access.
//
//  Example:
//
//    auto value = apache::thrift::can_throw(*obj.field_ref());
constexpr detail::can_throw_fn can_throw;

} // namespace thrift
} // namespace apache
