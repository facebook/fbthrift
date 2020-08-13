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

#include <any>
#include <optional>
#include <type_traits>
#include <typeinfo>

#include <folly/CPortability.h>
#include <folly/Traits.h>
#include <folly/lang/Exception.h>

namespace apache::thrift::conformance {

class any_ref;
namespace detail {
template <typename T>
T* raw_any_cast(const any_ref* operand) noexcept;
}

// A reference wrapper to any c++ value.
//
// For example:
//     void foo(any_ref in) {
//       if (const auto* i = any_cast<const int>(&in); i != nullptr) {
//         ...
//       } else if (const auto* f = any_cast<const float>(&any_ref); f !=
//       nullptr)
//       {
//         ...
//       }
//     }
//
//     foo(1);
//     foo(0.5f);
//
// References to std::any refer to the stored value, if present.
// For example:
//    std::any foo;
//    any_ref fooRef = foo;
//    assert(fooRef.has_value());  // Is set to an empty std::any.
//    // The empty std::any advertises the any type.
//    assert(fooRef.type() == typeid(std::any));
//
//    // The std::any value can be set through the any_ref.
//    any_cast<std::any&>(fooRef) = 1;
//    // Now it advertises the int type.
//    assert(fooRef.type() == typeid(int));
//    // Which can be accessed directly through the any_ref.
//    any_cast<int&>(fooRef) = 2;
//    // The original value shows the change.
//    assert(std::any_cast<int>(foo) == 2);
//
//    // The std::any is still accessible.
//    any_cast<std::any&>(fooRef) = 2.0;
//    assert(fooRef.type() == typeid(double));
//
// TODO(afuller): move to folly.
class any_ref final {
  template <typename T>
  using disable_self =
      std::enable_if_t<!std::is_same_v<folly::remove_cvref_t<T>, any_ref>>;

 public:
  any_ref() noexcept {}
  any_ref(const any_ref&) noexcept = default;
  any_ref(any_ref&&) noexcept = default;

  any_ref& operator=(const any_ref&) noexcept = default;
  any_ref& operator=(any_ref&&) noexcept = default;

  // Implicit constructors.
  template <typename T, typename = disable_self<T>>
  /* implicit */ any_ref(T& value) noexcept
      : details_(details<T&>()),
        value_(const_cast<std::remove_cv_t<T>*>(&value)) {}
  template <typename T, typename = disable_self<T>>
  /* implicit */ any_ref(T&& value) noexcept
      : details_(details<T&&>()),
        value_(const_cast<std::remove_cv_t<T>*>(&value)) {}

  // Returns the type of the value being referenced.
  //
  // If a non-empty std::any is being stored, this returns
  // the inner type of the any.
  const std::type_info& type() const noexcept;

  // If a reference is being stored.
  bool has_value() const noexcept {
    return value_ != nullptr;
  }
  explicit operator bool() const noexcept {
    return has_value();
  }

  // Runtime equivalent of std::is_const_v;
  bool is_const() const noexcept {
    return details_->is_const;
  }

  // Runtime equivalent of sd::is_rvalue_reference_v.
  bool is_rvalue_reference() const noexcept {
    return details_->is_rvalue_reference;
  }

 private:
  template <typename T>
  friend T* detail::raw_any_cast(const any_ref* operand) noexcept;

  struct TypeDetails {
    bool is_const;
    bool is_rvalue_reference;
    const std::type_info& type;
  };

  const TypeDetails* details_ = details<void>();
  void* value_ = nullptr;

  template <typename T>
  FOLLY_EXPORT static const TypeDetails* details();
};

// Try to cast the any_ref to a pointer of the given type.
//
// @returns nullptr if the cast is invalid.
template <typename T>
T* any_cast_exact(const any_ref* operand) noexcept;

// Try to cast the any_ref to a value or reference of the given type.
//
// @throws std::bad_any_cast if the cast is invalid.
template <typename T>
T any_cast_exact(const any_ref& operand);

// Similar to any_cast_exact except supports the following implicit
// conversions of reference/pointer types
// - non-const -> const
// - && -> &
template <typename T>
T* any_cast(const any_ref* operand) noexcept;
template <typename T>
T any_cast(const any_ref& operand);

// Implemenation details.

namespace detail {

template <typename T>
T* raw_any_cast(const any_ref* operand) noexcept {
  if (operand->details_->type == typeid(T)) {
    return static_cast<T*>(operand->value_);
  }
  if (operand->details_->type == typeid(std::any)) {
    // Forward the cast to std::any.
    return std::any_cast<T>(static_cast<std::any*>(operand->value_));
  }
  return nullptr;
}

} // namespace detail

template <typename T>
FOLLY_EXPORT auto any_ref::details() -> const TypeDetails* {
  // TODO(afuller): Consider add support for volatile references.
  static_assert(
      !std::is_volatile_v<std::remove_reference_t<T>>,
      "references to volalite values not yet supported");
  // TODO(afuller): Consider matching std::optional's std::nullopt support.
  static_assert(
      !std::is_same_v<std::decay_t<T>, std::nullopt_t>,
      "any_ref cannot reference std::nullopt.");
  static_assert(
      std::is_trivially_destructible_v<TypeDetails>,
      "if this changes, switch to folly::Indestructible");
  static const TypeDetails kValue{std::is_const_v<std::remove_reference_t<T>>,
                                  std::is_rvalue_reference_v<T>,
                                  typeid(T)};
  return &kValue;
}

template <typename T>
T* any_cast_exact(const any_ref* operand) noexcept {
  if (!operand->has_value() || // No value.
      operand->is_const() != std::is_const_v<T>) { // Chagnes constness.
    return nullptr;
  }
  return detail::raw_any_cast<T>(operand);
}

template <typename T>
T any_cast_exact(const any_ref& operand) {
  static_assert(
      std::is_reference_v<T>, "any_cast_exact cannot return by value.");
  auto* result = any_cast_exact<std::remove_reference_t<T>>(&operand);
  if (result == nullptr) {
    folly::throw_exception<std::bad_any_cast>();
  }
  if (std::is_rvalue_reference_v<T> != operand.is_rvalue_reference()) {
    folly::throw_exception<std::bad_any_cast>(); // Changes reference type.
  }
  return std::forward<T>(*result);
}

template <typename T>
T* any_cast(const any_ref* operand) noexcept {
  if (!operand->has_value()) {
    return nullptr; // No value.
  }
  if constexpr (!std::is_const_v<T>) {
    if (operand->is_const()) {
      return nullptr; // Would lose const qualifier.
    }
  }
  return detail::raw_any_cast<T>(operand);
}

template <typename T>
T any_cast(const any_ref& operand) {
  using target_t = std::conditional_t<
      std::is_reference_v<T>,
      std::remove_reference_t<T>,
      const T>;
  auto* result = any_cast<target_t>(&operand);
  if (result == nullptr) {
    folly::throw_exception<std::bad_any_cast>();
  }
  if constexpr (std::is_rvalue_reference_v<T>) {
    if (!operand.is_rvalue_reference()) {
      folly::throw_exception<std::bad_any_cast>(); // Not an rvalue!
    }
    return std::move(*result);
  } else {
    return *result;
  }
}

} // namespace apache::thrift::conformance
