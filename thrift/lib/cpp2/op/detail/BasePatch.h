/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <type_traits>

#include <folly/Portability.h>
#include <thrift/lib/cpp2/FieldRef.h>
#include <thrift/lib/cpp2/op/Clear.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

// Helpers for detecting compatible optional types.
template <typename T>
struct is_optional_type : std::false_type {};
template <typename T>
struct is_optional_type<optional_field_ref<T>> : std::true_type {};
template <typename T>
struct is_optional_type<optional_boxed_field_ref<T>> : std::true_type {};
#ifdef THRIFT_HAS_OPTIONAL
template <typename T>
struct is_optional_type<std::optional<T>> : std::true_type {};
#endif

template <typename T, typename R = void>
using if_opt_type = std::enable_if_t<is_optional_type<T>::value, R>;
template <typename T, typename R = void>
using if_not_opt_type = std::enable_if_t<!is_optional_type<T>::value, R>;

// Base class for all patch types.
// - Patch: The Thrift struct representation for the patch.
// - Derived: The leaf type deriving from this class.
template <typename Patch, typename Derived>
class BasePatch {
 public:
  BasePatch() = default;
  explicit BasePatch(const Patch& patch) : patch_(patch) {}
  explicit BasePatch(Patch&& patch) noexcept : patch_(std::move(patch)) {}

  const Patch& get() const& { return patch_; }
  Patch&& get() && { return std::move(patch_); }

  void reset() { op::clear<type::struct_t<Patch>>(patch_); }

  // Automatically dereference non-optional fields.
  template <typename U>
  void apply(field_ref<U> field) const {
    derived().apply(*field);
  }
  template <typename U>
  void assign(field_ref<U> val) {
    derived().assign(std::forward<U>(*val));
  }
  template <typename U>
  Derived& operator=(field_ref<U> field) {
    derived().assign(std::forward<U>(*field));
    return derived();
  }

 protected:
  Patch patch_;

  ~BasePatch() = default; // abstract base class

  // A fluent version of 'reset()'.
  FOLLY_NODISCARD Patch& resetAnd() { return (reset(), patch_); }

  Derived& derived() { return static_cast<Derived&>(*this); }
  const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

// Base class for value patch types.
//
// Patch must have the following fields:
//   optional T assign;
template <typename Patch, typename Derived>
class BaseValuePatch : public BasePatch<Patch, Derived> {
  using Base = BasePatch<Patch, Derived>;

 public:
  using value_type = std::decay_t<decltype(*std::declval<Patch>().assign())>;
  using Base::apply;
  using Base::assign;
  using Base::operator=;
  using Base::Base;

  template <typename U = value_type>
  static Derived createAssign(U&& val) {
    Derived patch;
    patch.assign(std::forward<U>(val));
    return patch;
  }

  void assign(const value_type& val) { resetAnd().assign().emplace(val); }
  void assign(value_type&& val) { resetAnd().assign().emplace(std::move(val)); }

  template <typename U>
  if_opt_type<std::decay_t<U>> apply(U&& field) const {
    if (field.has_value()) {
      derived().apply(*std::forward<U>(field));
    }
  }

  Derived& operator=(const value_type& val) { return (assign(val), derived()); }
  Derived& operator=(value_type&& val) {
    assign(std::move(val));
    return derived();
  }

 protected:
  using Base::derived;
  using Base::patch_;
  using Base::resetAnd;

  ~BaseValuePatch() = default; // abstract base class

  value_type& assignOr(value_type& value) noexcept {
    return patch_.assign().has_value() ? *patch_.assign() : value;
  }

  bool applyAssign(value_type& val) const {
    if (patch_.assign().has_value()) {
      val = *patch_.assign();
      return true;
    }
    return false;
  }

  template <typename U>
  bool mergeAssign(U&& next) {
    if (next.get().assign().has_value()) {
      patch_ = std::forward<U>(next).get();
      return true;
    }
    if (patch_.assign().has_value()) {
      next.apply(*patch_.assign());
      return true;
    }
    return false;
  }
};

// Base class for clearable value patch types.
//
// Patch must have the following fields:
//   optional T assign;
//   bool clear;
template <typename Patch, typename Derived>
class BaseClearablePatch : public BaseValuePatch<Patch, Derived> {
  using Base = BaseValuePatch<Patch, Derived>;
  using T = typename Base::value_type;

 public:
  using Base::Base;
  using Base::operator=;

  static Derived createClear() {
    Derived patch;
    patch.clear();
    return patch;
  }

  void clear() { resetAnd().clear() = true; }

 protected:
  using Base::applyAssign;
  using Base::mergeAssign;
  using Base::patch_;
  using Base::resetAnd;

  ~BaseClearablePatch() = default;

  template <typename U>
  bool mergeAssignAndClear(U&& next) {
    // Clear is slightly stronger than assigning a 'cleared' value in some
    // cases. For example a struct with non-terse, non-optional fields with
    // custom defaults and missmatched schemas... it's also smaller, so prefer
    // it.
    if (*next.get().clear() && !next.get().assign().has_value()) {
      // Next patch completely replaces this one.
      patch_ = std::forward<U>(next).get();
      return true;
    }
    return mergeAssign(std::forward<U>(next));
  }
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
