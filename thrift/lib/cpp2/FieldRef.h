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
#include <memory>
#include <type_traits>

#if (!defined(_MSC_VER) && __has_include(<optional>)) ||        \
    (defined(_MSC_VER) && __cplusplus >= 201703L)
#include <optional>
// Technically it should be 201606 but std::optional is present with 201603.
#if __cpp_lib_optional >= 201603
#define THRIFT_HAS_OPTIONAL
#endif
#endif

#include <folly/CPortability.h>
#include <folly/CppAttributes.h>
#include <folly/Portability.h>

namespace apache {
namespace thrift {
namespace detail {

template <typename T>
using is_set_t = std::conditional_t<std::is_const<T>::value, const bool, bool>;

[[noreturn]] void throw_on_bad_field_access();

struct ensure_isset_unsafe_fn;
struct unset_unsafe_fn;
struct alias_isset_fn;

} // namespace detail

// A reference to an unqualified field of the possibly const-qualified type
// std::remove_reference_t<T> in a Thrift-generated struct.
template <typename T>
class field_ref {
  static_assert(std::is_reference<T>::value, "not a reference");

  template <typename U>
  friend class field_ref;
  friend struct detail::unset_unsafe_fn;

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
      std::enable_if_t<std::is_assignable<value_type&, U&&>::value, field_ref&>
      operator=(U&& value) noexcept(
          std::is_nothrow_assignable<value_type&, U&&>::value) {
    value_ = static_cast<U&&>(value);
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
    return static_cast<reference_type>(value_);
  }

  FOLLY_ERASE reference_type operator*() const noexcept {
    return static_cast<reference_type>(value_);
  }

  FOLLY_ERASE value_type* operator->() const noexcept {
    return &value_;
  }

  FOLLY_ERASE reference_type ensure() noexcept {
    is_set_ = true;
    return static_cast<reference_type>(value_);
  }

  template <typename Index>
  FOLLY_ERASE auto operator[](const Index& index) const -> decltype(auto) {
    return value_[index];
  }

  template <typename... Args>
  FOLLY_ERASE value_type& emplace(Args&&... args) {
    is_set_ = false; // C++ Standard requires *this to be empty if
                     // `std::optional::emplace(...)` throws
    value_ = value_type(static_cast<Args&&>(args)...);
    is_set_ = true;
    return value_;
  }

  template <class U, class... Args>
  FOLLY_ERASE std::enable_if_t<
      std::is_constructible<value_type, std::initializer_list<U>, Args&&...>::
          value,
      value_type&>
  emplace(std::initializer_list<U> ilist, Args&&... args) {
    is_set_ = false;
    value_ = value_type(ilist, static_cast<Args&&>(args)...);
    is_set_ = true;
    return value_;
  }

 private:
  value_type& value_;
  detail::is_set_t<value_type>& is_set_;
};

template <typename T, typename U>
bool operator==(field_ref<T> lhs, field_ref<U> rhs) {
  return *lhs == *rhs;
}

template <typename T, typename U>
bool operator!=(field_ref<T> lhs, field_ref<U> rhs) {
  return *lhs != *rhs;
}

template <typename T, typename U>
bool operator<(field_ref<T> lhs, field_ref<U> rhs) {
  return *lhs < *rhs;
}

template <typename T, typename U>
bool operator>(field_ref<T> lhs, field_ref<U> rhs) {
  return *lhs > *rhs;
}

template <typename T, typename U>
bool operator<=(field_ref<T> lhs, field_ref<U> rhs) {
  return *lhs <= *rhs;
}

template <typename T, typename U>
bool operator>=(field_ref<T> lhs, field_ref<U> rhs) {
  return *lhs >= *rhs;
}

template <typename T, typename U>
bool operator==(field_ref<T> lhs, const U& rhs) {
  return *lhs == rhs;
}

template <typename T, typename U>
bool operator!=(field_ref<T> lhs, const U& rhs) {
  return *lhs != rhs;
}

template <typename T, typename U>
bool operator<(field_ref<T> lhs, const U& rhs) {
  return *lhs < rhs;
}

template <typename T, typename U>
bool operator>(field_ref<T> lhs, const U& rhs) {
  return *lhs > rhs;
}

template <typename T, typename U>
bool operator<=(field_ref<T> lhs, const U& rhs) {
  return *lhs <= rhs;
}

template <typename T, typename U>
bool operator>=(field_ref<T> lhs, const U& rhs) {
  return *lhs >= rhs;
}

template <typename T, typename U>
bool operator==(const T& lhs, field_ref<U> rhs) {
  return lhs == *rhs;
}

template <typename T, typename U>
bool operator!=(const T& lhs, field_ref<U> rhs) {
  return lhs != *rhs;
}

template <typename T, typename U>
bool operator<(const T& lhs, field_ref<U> rhs) {
  return lhs < *rhs;
}

template <typename T, typename U>
bool operator>(const T& lhs, field_ref<U> rhs) {
  return lhs > *rhs;
}

template <typename T, typename U>
bool operator<=(const T& lhs, field_ref<U> rhs) {
  return lhs <= *rhs;
}

template <typename T, typename U>
bool operator>=(const T& lhs, field_ref<U> rhs) {
  return lhs >= *rhs;
}

// A reference to an optional field of the possibly const-qualified type
// std::remove_reference_t<T> in a Thrift-generated struct.
template <typename T>
class optional_field_ref {
  static_assert(std::is_reference<T>::value, "not a reference");

  template <typename U>
  friend class optional_field_ref;
  friend struct detail::ensure_isset_unsafe_fn;
  friend struct detail::alias_isset_fn;

 public:
  using value_type = std::remove_reference_t<T>;
  using reference_type = T;

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
      std::is_assignable<value_type&, U&&>::value,
      optional_field_ref&>
  operator=(U&& value) noexcept(
      std::is_nothrow_assignable<value_type&, U&&>::value) {
    value_ = static_cast<U&&>(value);
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
      std::is_nothrow_assignable<value_type&, std::remove_reference_t<U>&&>::
          value) {
    value_ = static_cast<std::remove_reference_t<U>&&>(other.value_);
    is_set_ = other.is_set_;
  }

#ifdef THRIFT_HAS_OPTIONAL
  template <typename U>
  FOLLY_ERASE void from_optional(const std::optional<U>& other) noexcept(
      std::is_nothrow_assignable<value_type&, const U&>::value) {
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
      std::is_nothrow_assignable<value_type&, U&&>::value) {
    // Use if instead of a shorter ternary expression to prevent a potential
    // copy if T and U mismatch.
    if (other) {
      value_ = static_cast<U&&>(*other);
    } else {
      value_ = {};
    }
    is_set_ = other.has_value();
  }

  FOLLY_ERASE std::optional<std::remove_const_t<value_type>> to_optional()
      const {
    using type = std::optional<std::remove_const_t<value_type>>;
    return is_set_ ? type(value_) : type();
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
    throw_if_unset();
    return static_cast<reference_type>(value_);
  }

  template <typename U = std::remove_const_t<value_type>>
  FOLLY_ERASE std::remove_const_t<value_type> value_or(
      U&& default_value) const {
    using type = std::remove_const_t<value_type>;
    return is_set_ ? type(static_cast<reference_type>(value_))
                   : type(static_cast<U&&>(default_value));
  }

  // Returns a reference to the value without checking whether it is available.
  FOLLY_ERASE reference_type value_unchecked() const {
    return static_cast<reference_type>(value_);
  }

  FOLLY_ERASE reference_type operator*() const {
    return value();
  }

  FOLLY_ERASE value_type* operator->() const {
    throw_if_unset();
    return &value_;
  }

  FOLLY_ERASE reference_type ensure() noexcept {
    if (!is_set_) {
      emplace();
    }
    return static_cast<reference_type>(value_);
  }

  template <typename... Args>
  FOLLY_ERASE value_type& emplace(Args&&... args) {
    reset(); // C++ Standard requires *this to be empty if
             // `std::optional::emplace(...)` throws
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
    value_ = value_type(ilist, static_cast<Args&&>(args)...);
    is_set_ = true;
    return value_;
  }

 private:
  FOLLY_ERASE void throw_if_unset() const {
    if (!is_set_) {
      detail::throw_on_bad_field_access();
    }
  }

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

template <typename T, typename U>
bool operator==(optional_field_ref<T> a, const U& b) {
  return a ? *a == b : false;
}

template <typename T, typename U>
bool operator!=(optional_field_ref<T> a, const U& b) {
  return !(a == b);
}

template <typename T, typename U>
bool operator==(const U& a, optional_field_ref<T> b) {
  return b == a;
}

template <typename T, typename U>
bool operator!=(const U& a, optional_field_ref<T> b) {
  return b != a;
}

template <typename T, typename U>
bool operator<(optional_field_ref<T> a, const U& b) {
  return a ? *a < b : true;
}

template <typename T, typename U>
bool operator>(optional_field_ref<T> a, const U& b) {
  return a ? *a > b : false;
}

template <typename T, typename U>
bool operator<=(optional_field_ref<T> a, const U& b) {
  return !(a > b);
}

template <typename T, typename U>
bool operator>=(optional_field_ref<T> a, const U& b) {
  return !(a < b);
}

template <typename T, typename U>
bool operator<(const U& a, optional_field_ref<T> b) {
  return b > a;
}

template <typename T, typename U>
bool operator<=(const U& a, optional_field_ref<T> b) {
  return b >= a;
}

template <typename T, typename U>
bool operator>(const U& a, optional_field_ref<T> b) {
  return b < a;
}

template <typename T, typename U>
bool operator>=(const U& a, optional_field_ref<T> b) {
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

struct ensure_isset_unsafe_fn {
  template <typename T>
  void operator()(optional_field_ref<T> ref) const noexcept {
    ref.is_set_ = true;
  }
};

struct unset_unsafe_fn {
  template <typename T>
  void operator()(field_ref<T> ref) const noexcept {
    ref.is_set_ = false;
  }
};

struct alias_isset_fn {
  template <typename T, typename F>
  auto operator()(optional_field_ref<T> ref, F functor) const noexcept {
    auto&& result = functor(ref.value_);
    return optional_field_ref<decltype(result)>(
        static_cast<decltype(result)>(result), ref.is_set_);
  }
};

constexpr unset_unsafe_fn unset_unsafe;

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

[[deprecated("Use `emplace` or `operator=` to set Thrift fields.")]] //
constexpr detail::ensure_isset_unsafe_fn ensure_isset_unsafe;

[[deprecated]] //
constexpr detail::alias_isset_fn alias_isset;

// A reference to an required field of the possibly const-qualified type
// std::remove_reference_t<T> in a Thrift-generated struct.
template <typename T>
class required_field_ref {
  static_assert(std::is_reference<T>::value, "not a reference");

  template <typename U>
  friend class required_field_ref;

 public:
  using value_type = std::remove_reference_t<T>;
  using reference_type = T;

  FOLLY_ERASE explicit required_field_ref(reference_type value) noexcept
      : value_(value) {}

  template <
      typename U,
      std::enable_if_t<
          std::is_same<
              std::add_const_t<std::remove_reference_t<U>>,
              value_type>{} &&
              !(std::is_rvalue_reference<T>{} && std::is_lvalue_reference<U>{}),
          int> = 0>
  FOLLY_ERASE /* implicit */ required_field_ref(
      const required_field_ref<U>& other) noexcept
      : value_(other.value_) {}

  template <typename U = value_type>
  FOLLY_ERASE std::enable_if_t<
      std::is_assignable<value_type&, U&&>::value,
      required_field_ref&>
  operator=(U&& value) noexcept(
      std::is_nothrow_assignable<value_type&, U&&>::value) {
    value_ = static_cast<U&&>(value);
    return *this;
  }

  // Assignment from required_field_ref is intentionally not provided to prevent
  // potential confusion between two possible behaviors, copying and reference
  // rebinding. The copy_from method is provided instead.
  template <typename U>
  FOLLY_ERASE void copy_from(required_field_ref<U> other) noexcept(
      std::is_nothrow_assignable<value_type&, U>::value) {
    value_ = other.value();
  }

  // Returns true iff the field is set. required_field_ref doesn't provide
  // conversion to bool to avoid confusion between checking if the field is set
  // and getting the field's value, particularly for bool fields.
  FOLLY_ERASE bool has_value() const noexcept {
    return true;
  }

  // Returns a reference to the value.
  FOLLY_ERASE reference_type value() const noexcept {
    return static_cast<reference_type>(value_);
  }

  FOLLY_ERASE reference_type operator*() const noexcept {
    return static_cast<reference_type>(value_);
  }

  FOLLY_ERASE value_type* operator->() const noexcept {
    return &value_;
  }

  FOLLY_ERASE reference_type ensure() noexcept {
    return static_cast<reference_type>(value_);
  }

  template <typename Index>
  FOLLY_ERASE auto operator[](const Index& index) const -> decltype(auto) {
    return value_[index];
  }

  template <typename... Args>
  FOLLY_ERASE value_type& emplace(Args&&... args) {
    return value_ = value_type(static_cast<Args&&>(args)...);
  }

  template <class U, class... Args>
  FOLLY_ERASE std::enable_if_t<
      std::is_constructible<value_type, std::initializer_list<U>, Args&&...>::
          value,
      value_type&>
  emplace(std::initializer_list<U> ilist, Args&&... args) {
    return value_ = value_type(ilist, static_cast<Args&&>(args)...);
  }

 private:
  value_type& value_;
};

template <typename T, typename U>
bool operator==(required_field_ref<T> lhs, required_field_ref<U> rhs) {
  return *lhs == *rhs;
}

template <typename T, typename U>
bool operator!=(required_field_ref<T> lhs, required_field_ref<U> rhs) {
  return *lhs != *rhs;
}

template <typename T, typename U>
bool operator<(required_field_ref<T> lhs, required_field_ref<U> rhs) {
  return *lhs < *rhs;
}

template <typename T, typename U>
bool operator>(required_field_ref<T> lhs, required_field_ref<U> rhs) {
  return *lhs > *rhs;
}

template <typename T, typename U>
bool operator<=(required_field_ref<T> lhs, required_field_ref<U> rhs) {
  return *lhs <= *rhs;
}

template <typename T, typename U>
bool operator>=(required_field_ref<T> lhs, required_field_ref<U> rhs) {
  return *lhs >= *rhs;
}

template <typename T, typename U>
bool operator==(required_field_ref<T> lhs, const U& rhs) {
  return *lhs == rhs;
}

template <typename T, typename U>
bool operator!=(required_field_ref<T> lhs, const U& rhs) {
  return *lhs != rhs;
}

template <typename T, typename U>
bool operator<(required_field_ref<T> lhs, const U& rhs) {
  return *lhs < rhs;
}

template <typename T, typename U>
bool operator>(required_field_ref<T> lhs, const U& rhs) {
  return *lhs > rhs;
}

template <typename T, typename U>
bool operator<=(required_field_ref<T> lhs, const U& rhs) {
  return *lhs <= rhs;
}

template <typename T, typename U>
bool operator>=(required_field_ref<T> lhs, const U& rhs) {
  return *lhs >= rhs;
}

template <typename T, typename U>
bool operator==(const T& lhs, required_field_ref<U> rhs) {
  return lhs == *rhs;
}

template <typename T, typename U>
bool operator!=(const T& lhs, required_field_ref<U> rhs) {
  return lhs != *rhs;
}

template <typename T, typename U>
bool operator<(const T& lhs, required_field_ref<U> rhs) {
  return lhs < *rhs;
}

template <typename T, typename U>
bool operator>(const T& lhs, required_field_ref<U> rhs) {
  return lhs > *rhs;
}

template <typename T, typename U>
bool operator<=(const T& lhs, required_field_ref<U> rhs) {
  return lhs <= *rhs;
}

template <typename T, typename U>
bool operator>=(const T& lhs, required_field_ref<U> rhs) {
  return lhs >= *rhs;
}

namespace detail {

struct union_field_ref_owner_vtable {
  using reset_t = void(void*);

  reset_t* reset;
};

struct union_field_ref_owner_vtable_impl {
  template <typename T>
  static void reset(void* obj) {
    static_cast<T*>(obj)->__clear();
  }
};

template <typename T>
FOLLY_INLINE_VARIABLE constexpr union_field_ref_owner_vtable //
    union_field_ref_owner_vtable_for{nullptr};
template <typename T>
FOLLY_INLINE_VARIABLE constexpr union_field_ref_owner_vtable //
    union_field_ref_owner_vtable_for<T&>{
        &union_field_ref_owner_vtable_impl::reset<T>};
template <typename T>
FOLLY_INLINE_VARIABLE constexpr union_field_ref_owner_vtable //
    union_field_ref_owner_vtable_for<T const&>{nullptr};

} // namespace detail

// A reference to an union field of the possibly const-qualified type
template <typename T>
class union_field_ref {
  static_assert(std::is_reference<T>::value, "not a reference");

  template <typename>
  friend class union_field_ref;

 public:
  using value_type = std::remove_reference_t<T>;
  using reference_type = T;

 private:
  using int_t =
      std::conditional_t<std::is_const<value_type>::value, const int, int>;
  using owner =
      std::conditional_t<std::is_const<value_type>::value, void const*, void*>;
  using vtable = detail::union_field_ref_owner_vtable;

 public:
  FOLLY_ERASE union_field_ref(
      reference_type value,
      int_t& type,
      int field_type,
      owner ow,
      vtable const& vt) noexcept
      : value_(value),
        type_(type),
        field_type_(field_type),
        owner_(ow),
        vtable_(vt) {}

  template <
      typename U,
      std::enable_if_t<
          std::is_convertible<U, value_type>::value ||
              std::is_assignable<value_type, U>::value,
          int> = 0>
  FOLLY_ERASE union_field_ref& operator=(U&& other) noexcept(
      std::is_nothrow_constructible<value_type, U>::value&&
          std::is_nothrow_assignable<value_type, U>::value) {
    if (has_value()) {
      value_ = static_cast<U&&>(other);
    } else {
      emplace(static_cast<U&&>(other));
    }
    return *this;
  }

  FOLLY_ERASE bool has_value() const {
    return type_ == field_type_;
  }

  FOLLY_ERASE explicit operator bool() const {
    return has_value();
  }

  // Returns a reference to the value if this is union's active field,
  // bad_field_access otherwise.
  FOLLY_ERASE reference_type value() const {
    throw_if_unset();
    return static_cast<reference_type>(value_);
  }

  FOLLY_ERASE reference_type operator*() const {
    return value();
  }

  FOLLY_ERASE value_type* operator->() const {
    throw_if_unset();
    return &value_;
  }

  FOLLY_ERASE reference_type ensure() {
    if (!has_value()) {
      emplace();
    }
    return static_cast<reference_type>(value_);
  }

  template <typename... Args>
  FOLLY_ERASE value_type& emplace(Args&&... args) {
    vtable_.reset(owner_);
    ::new (&value_) value_type(static_cast<Args&&>(args)...);
    type_ = field_type_;
    return value_;
  }

  template <class U, class... Args>
  FOLLY_ERASE std::enable_if_t<
      std::is_constructible<value_type, std::initializer_list<U>, Args&&...>::
          value,
      value_type&>
  emplace(std::initializer_list<U> ilist, Args&&... args) {
    vtable_.reset(owner_);
    ::new (&value_) value_type(ilist, static_cast<Args&&>(args)...);
    type_ = field_type_;
    return value_;
  }

 private:
  FOLLY_ERASE void throw_if_unset() const {
    if (!has_value()) {
      detail::throw_on_bad_field_access();
    }
  }

  reference_type value_;
  int_t& type_;
  const int field_type_;
  owner owner_;
  vtable const& vtable_;
};

template <typename T1, typename T2>
bool operator==(union_field_ref<T1> a, union_field_ref<T2> b) {
  return a && b ? *a == *b : a.has_value() == b.has_value();
}

template <typename T1, typename T2>
bool operator!=(union_field_ref<T1> a, union_field_ref<T2> b) {
  return !(a == b);
}

template <typename T1, typename T2>
bool operator<(union_field_ref<T1> a, union_field_ref<T2> b) {
  if (a.has_value() != b.has_value()) {
    return a.has_value() < b.has_value();
  }
  return a ? *a < *b : false;
}

template <typename T1, typename T2>
bool operator>(union_field_ref<T1> a, union_field_ref<T2> b) {
  return b < a;
}

template <typename T1, typename T2>
bool operator<=(union_field_ref<T1> a, union_field_ref<T2> b) {
  return !(a > b);
}

template <typename T1, typename T2>
bool operator>=(union_field_ref<T1> a, union_field_ref<T2> b) {
  return !(a < b);
}

template <typename T, typename U>
bool operator==(union_field_ref<T> a, const U& b) {
  return a ? *a == b : false;
}

template <typename T, typename U>
bool operator!=(union_field_ref<T> a, const U& b) {
  return !(a == b);
}

template <typename T, typename U>
bool operator==(const U& a, union_field_ref<T> b) {
  return b == a;
}

template <typename T, typename U>
bool operator!=(const U& a, union_field_ref<T> b) {
  return b != a;
}

template <typename T, typename U>
bool operator<(union_field_ref<T> a, const U& b) {
  return a ? *a < b : true;
}

template <typename T, typename U>
bool operator>(union_field_ref<T> a, const U& b) {
  return a ? *a > b : false;
}

template <typename T, typename U>
bool operator<=(union_field_ref<T> a, const U& b) {
  return !(a > b);
}

template <typename T, typename U>
bool operator>=(union_field_ref<T> a, const U& b) {
  return !(a < b);
}

template <typename T, typename U>
bool operator<(const U& a, union_field_ref<T> b) {
  return b > a;
}

template <typename T, typename U>
bool operator<=(const U& a, union_field_ref<T> b) {
  return b >= a;
}

template <typename T, typename U>
bool operator>(const U& a, union_field_ref<T> b) {
  return b < a;
}

template <typename T, typename U>
bool operator>=(const U& a, union_field_ref<T> b) {
  return b <= a;
}

} // namespace thrift
} // namespace apache
