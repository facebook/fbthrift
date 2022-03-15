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

#include <folly/Portability.h>
#include <thrift/lib/cpp2/FieldRef.h>
#include <thrift/lib/cpp2/op/Clear.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

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
  template <typename T>
  void apply(const field_ref<T>& field) const {
    derived().apply(*field);
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
  using Base::Base;

  template <typename U = value_type>
  static Derived createAssign(U&& val) {
    Derived patch;
    patch.assign(std::forward<U>(val));
    return patch;
  }

  bool hasAssign() const noexcept { return patch_.assign().has_value(); }
  void assign(const value_type& val) { resetAnd().assign().emplace(val); }
  void assign(value_type&& val) { resetAnd().assign().emplace(std::move(val)); }

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
    return hasAssign() ? *patch_.assign() : value;
  }

  bool applyAssign(value_type& val) const {
    if (hasAssign()) {
      val = *patch_.assign();
      return true;
    }
    return false;
  }

  template <typename U>
  bool mergeAssign(U&& next) {
    if (next.hasAssign()) {
      patch_ = std::forward<U>(next).get();
      return true;
    }
    if (hasAssign()) {
      next.apply(*patch_.assign());
      return true;
    }
    return false;
  }
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
