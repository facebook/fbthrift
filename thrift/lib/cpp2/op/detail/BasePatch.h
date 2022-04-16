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

#include <stdexcept>
#include <type_traits>

#include <folly/Portability.h>
#include <folly/Traits.h>
#include <thrift/lib/cpp2/FieldRef.h>
#include <thrift/lib/cpp2/op/Clear.h>

namespace apache {
namespace thrift {
namespace op {

class bad_patch_access : public std::runtime_error {
 public:
  bad_patch_access() noexcept
      : std::runtime_error("Patch guarantees value is unset.") {}
};

namespace detail {

// Helpers for detecting compatible optional types.
template <typename T>
struct is_optional_type : std::false_type {};
template <typename T>
struct is_optional_type<optional_field_ref<T>> : std::true_type {};
template <typename T>
struct is_optional_type<optional_boxed_field_ref<T>> : std::true_type {};
template <typename T>
struct is_optional_type<union_field_ref<T>> : std::true_type {};
#ifdef THRIFT_HAS_OPTIONAL
template <typename T>
struct is_optional_type<std::optional<T>> : std::true_type {};
#endif

template <typename T, typename R = void>
using if_opt_type = std::enable_if_t<is_optional_type<T>::value, R>;
template <typename T, typename R = void>
using if_not_opt_type = std::enable_if_t<!is_optional_type<T>::value, R>;

template <typename T>
if_opt_type<T, bool> hasValue(const T& opt) {
  return opt.has_value();
}
template <typename T>
bool hasValue(field_ref<T> val) {
  return !thrift::empty(*val);
}
template <typename T>
if_opt_type<T> clearValue(T& opt) {
  opt.reset();
}
template <typename T>
if_not_opt_type<T> clearValue(T& unn) {
  thrift::clear(unn);
}
template <typename T, typename U>
if_opt_type<T, bool> sameType(const T& opt1, const U& opt2) {
  return opt1.has_value() == opt2.has_value();
}
template <typename T, typename U>
bool sameType(field_ref<T> unn1, const U& unn2) {
  return unn1->getType() == unn2.getType();
}

// Base class for all patch types.
// - Patch: The Thrift struct representation for the patch.
// - Derived: The leaf type deriving from this class.
template <typename Patch, typename Derived>
class BasePatch {
 public:
  using underlying_type = Patch;

  BasePatch() = default;
  explicit BasePatch(const underlying_type& patch) : patch_(patch) {}
  explicit BasePatch(underlying_type&& patch) noexcept
      : patch_(std::move(patch)) {}

  FOLLY_NODISCARD const underlying_type& get() const& { return patch_; }
  FOLLY_NODISCARD underlying_type&& get() && { return std::move(patch_); }

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
  using value_type =
      folly::remove_cvref_t<decltype(*std::declval<Patch>().assign())>;
  using Base::apply;
  using Base::assign;
  using Base::operator=;
  using Base::Base;

  template <typename U = value_type>
  FOLLY_NODISCARD static Derived createAssign(U&& val) {
    Derived patch;
    patch.assign(std::forward<U>(val));
    return patch;
  }

  void assign(const value_type& val) { resetAnd().assign().emplace(val); }
  void assign(value_type&& val) { resetAnd().assign().emplace(std::move(val)); }

  template <typename U>
  if_opt_type<folly::remove_cvref_t<U>> apply(U&& field) const {
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

  FOLLY_NODISCARD value_type& assignOr(value_type& value) noexcept {
    return hasValue(patch_.assign()) ? *patch_.assign() : value;
  }

  bool applyAssign(value_type& val) const {
    if (hasValue(patch_.assign())) {
      val = *patch_.assign();
      return true;
    }
    return false;
  }

  template <typename U>
  bool mergeAssign(U&& next) {
    if (hasValue(next.get().assign())) {
      patch_ = std::forward<U>(next).get();
      return true;
    }
    if (hasValue(patch_.assign())) {
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
class BaseClearValuePatch : public BaseValuePatch<Patch, Derived> {
  using Base = BaseValuePatch<Patch, Derived>;
  using T = typename Base::value_type;

 public:
  using Base::Base;
  using Base::operator=;

  FOLLY_NODISCARD static Derived createClear() {
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

  ~BaseClearValuePatch() = default;

  template <typename U>
  bool mergeAssignAndClear(U&& next) {
    // Clear is slightly stronger than assigning a 'cleared' value in some
    // cases. For example a struct with non-terse, non-optional fields with
    // custom defaults and missmatched schemas... it's also smaller, so prefer
    // it.
    if (*next.get().clear() && !hasValue(next.get().assign())) {
      // Next patch completely replaces this one.
      patch_ = std::forward<U>(next).get();
      return true;
    }
    return mergeAssign(std::forward<U>(next));
  }
};

// Patch must have the following fields:
//   bool clear;
//   P patch;
//   (optional) T ensure;
//   P patchAfter;
template <typename Patch, typename Derived>
class BaseEnsurePatch : public BasePatch<Patch, Derived> {
  using Base = BasePatch<Patch, Derived>;

 public:
  using value_type =
      folly::remove_cvref_t<decltype(*std::declval<Patch>().ensure())>;
  using value_patch_type =
      folly::remove_cvref_t<decltype(*std::declval<Patch>().patch())>;
  using Base::assign;
  using Base::operator=;
  using Base::Base;

  // Ensure the value is set to the given value.
  template <typename U = value_type>
  FOLLY_NODISCARD static Derived createAssign(U&& val) {
    Derived patch;
    patch.assign(std::forward<U>(val));
    return patch;
  }
  void assign(const value_type& val) { clearAnd().ensure().emplace(val); }
  void assign(value_type&& val) { clearAnd().ensure().emplace(std::move(val)); }
  Derived& operator=(const value_type& val) { return (assign(val), derived()); }
  Derived& operator=(value_type&& val) {
    assign(std::move(val));
    return derived();
  }

  // Unset any value.
  FOLLY_NODISCARD static Derived createClear() {
    Derived patch;
    patch.clear();
    return patch;
  }
  void clear() { resetAnd().clear() = true; }

  // Patch any set value.
  FOLLY_NODISCARD value_patch_type& patch() {
    if (hasValue(patch_.ensure())) {
      return *patch_.patchAfter();
    } else if (*patch_.clear()) {
      folly::throw_exception<bad_patch_access>();
    }
    return *patch_.patch();
  }

 protected:
  using Base::derived;
  using Base::patch_;
  using Base::resetAnd;

  ~BaseEnsurePatch() = default;

  Patch& clearAnd() { return (clear(), patch_); }
  template <typename U = value_type>
  Patch& ensureAnd(U&& _default) {
    if (!hasValue(patch_.ensure())) {
      patch_.ensure().emplace(std::forward<U>(_default));
    }
    return patch_;
  }

  bool emptyEnsure() const {
    return !*patch_.clear() && patch_.patch()->empty() &&
        !hasValue(patch_.ensure()) && patch_.patchAfter()->empty();
  }

  template <typename U>
  bool mergeEnsure(U&& next) {
    if (*next.get().clear()) {
      if (hasValue(next.get().ensure())) {
        patch_.clear() = true;
        patch_.patch()->reset(); // We can ignore next.patch.
        patch_.ensure() = *std::forward<U>(next).get().ensure();
        patch_.patchAfter() = *std::forward<U>(next).get().patchAfter();
      } else {
        clear(); // We can ignore everything else.
      }
      return true; // It's a complete replacement.
    }

    if (hasValue(patch_.ensure())) {
      // All values will be set before next, so ignore next.ensure and
      // merge next.patch and next.patchAfter into this.patchAfter.
      auto temp = *std::forward<U>(next).get().patchAfter();
      patch_.patchAfter()->merge(*std::forward<U>(next).get().patch());
      patch_.patchAfter()->merge(std::move(temp));
    } else { // Both this.ensure and next.clear are known to be empty.
      // Merge anything (oddly) in patchAfter into patch.
      patch_.patch()->merge(std::move(*patch_.patchAfter()));
      // Merge in next.patch into patch.
      patch_.patch()->merge(*std::forward<U>(next).get().patch());
      // Consume next.ensure, if any.
      if (hasValue(next.get().ensure())) {
        patch_.ensure() = *std::forward<U>(next).get().ensure();
      }
      // Consume next.patchAfter.
      patch_.patchAfter() = *std::forward<U>(next).get().patchAfter();
    }
    return false;
  }

  template <typename U>
  void applyEnsure(U& val) const {
    // Clear or patch.
    if (*patch_.clear()) {
      clearValue(val);
    } else {
      patch_.patch()->apply(val);
    }
    // Ensure if needed.
    if (hasValue(patch_.ensure()) && !sameType(patch_.ensure(), val)) {
      val = *patch_.ensure();
    }
    // Apply the patch after ensure.
    patch_.patchAfter()->apply(val);
  }
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
