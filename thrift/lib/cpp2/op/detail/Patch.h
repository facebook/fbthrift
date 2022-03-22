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
#include <utility>

#include <thrift/lib/cpp2/op/detail/BasePatch.h>
#include <thrift/lib/cpp2/op/detail/ValuePatch.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

class bad_optional_patch_access : public std::bad_optional_access {
 public:
  const char* what() const noexcept override {
    return "Patch guarantees optional value is unset.";
  }
};

// A patch for an 'optional' value.
//
// Patch must have the following fields:
//   bool clear;
//   P patch;
//   optional T ensure;
//   P patchAfter;
// Where P is the patch type for a non-optional value.
template <typename Patch>
class OptionalPatch : public BasePatch<Patch, OptionalPatch<Patch>> {
  using Base = BasePatch<Patch, OptionalPatch>;

 public:
  using value_type = std::decay_t<decltype(*std::declval<Patch>().ensure())>;
  using value_patch_type =
      std::decay_t<decltype(*std::declval<Patch>().patch())>;
  using Base::Base;
  using Base::operator=;
  using Base::apply;
  using Base::assign;

  // Unset all values.
  void clear() { resetAnd().clear() = true; }

  // Patch set values.
  value_patch_type& patch() {
    if (patch_.ensure().has_value()) {
      return *patch_.patchAfter();
    }
    if (*patch_.clear()) {
      folly::throw_exception<bad_optional_patch_access>();
    }
    return *patch_.patch();
  }
  value_patch_type& operator*() { return patch(); }
  value_patch_type* operator->() { return &patch(); }

  // Ensure unset values, are set to the default, and return
  // the patch for all set value.
  value_patch_type& ensure() {
    patch_.ensure().ensure();
    return *patch_.patchAfter();
  }

  // Ensure any unset values, are set to the given value, and return
  // the patch for all set value.
  template <typename U>
  value_patch_type& ensure(U&& val) {
    if (!patch_.ensure().has_value()) {
      patch_.ensure().emplace(std::forward<U>(val));
    }
    return *patch_.patchAfter();
  }

  // Ensure the value is set, then assign the value.
  template <typename U>
  if_not_opt_type<std::decay_t<U>> assign(U&& val) {
    ensure().assign(std::forward<U>(val));
  }
  // Patch the set state of the optional value.
  template <typename U>
  if_opt_type<std::decay_t<U>> assign(U&& val) {
    if (val.has_value()) {
      assign(*std::forward<U>(val));
    } else {
      clear();
    }
  }
#ifdef THRIFT_HAS_OPTIONAL
  void assign(std::nullopt_t) { clear(); }
  OptionalPatch& operator=(std::nullopt_t) {
    clear();
    return *this;
  }
#endif
  OptionalPatch& operator=(const value_type& val) {
    assign(val);
    return *this;
  }
  OptionalPatch& operator=(value_type&& val) {
    assign(std::move(val));
    return *this;
  }
  template <typename U>
  if_opt_type<std::decay_t<U>, OptionalPatch&> operator=(U&& val) {
    assign(std::forward<U>(val));
    return *this;
  }

  bool empty() const {
    return !*patch_.clear() && patch_.patch()->empty() &&
        !patch_.ensure().has_value() && patch_.patchAfter()->empty();
  }

  void apply(value_type& val) const {
    // A non-optional value cannot be unset (or ensured).
    patch_.patch()->apply(val);
    patch_.patchAfter()->apply(val);
  }
  template <typename U>
  if_opt_type<std::decay_t<U>> apply(U&& val) const {
    // Clear first.
    if (*patch_.clear()) {
      val.reset();
    }
    // Apply the first patch.
    patch_.patch()->apply(val);
    // Ensure the value if needed.
    if (patch_.ensure().has_value() && !val.has_value()) {
      val.emplace(*patch_.ensure());
    }
    // Apply the second patch.
    patch_.patchAfter()->apply(val);
  }

  template <typename U>
  void merge(U&& next) {
    if (*next.get().clear()) {
      // It's a complete replacement, but we only need to copy the applicable
      // parts.
      if (!next.get().ensure().has_value()) {
        clear();
        return;
      }

      // We can ignore next.patch, but need ensure and patchAfter.
      patch_.clear() = true;
      patch_.patch()->reset();
      patch_.ensure() = *std::forward<U>(next).get().ensure();
      patch_.patchAfter() = *std::forward<U>(next).get().patchAfter();
      return;
    }

    if (patch_.ensure().has_value()) {
      // All values will be set before next, so ignore next.ensure.
      // Consume next.patch and next.patchAfter.
      auto temp = *std::forward<U>(next).get().patch();
      temp.merge(*std::forward<U>(next).get().patchAfter());
      patch_.patchAfter()->merge(std::move(temp));
      return;
    }

    // At this point both this.ensure and next.clear are known to be empty.
    // Merge anything (oddly) in patchAfter into patch.
    patch_.patch()->merge(std::move(*patch_.patchAfter()));
    // Merge in next.patch into patch.
    patch_.patch()->merge(*std::forward<U>(next).get().patch());
    // Consume next.ensure, if any.
    if (next.get().ensure().has_value()) {
      patch_.ensure() = *std::forward<U>(next).get().ensure();
    }
    // Consume next.patchAfter.
    patch_.patchAfter() = *std::forward<U>(next).get().patchAfter();
  }

 private:
  using Base::patch_;
  using Base::resetAnd;
};

template <template <typename> class PatchType>
struct PatchAdapter {
  template <typename Patch>
  static decltype(auto) toThrift(Patch&& value) {
    return std::forward<Patch>(value).get();
  }

  template <typename Patch>
  static PatchType<Patch> fromThrift(Patch&& value) {
    return PatchType<Patch>{std::forward<Patch>(value)};
  }
};

// Adapter for all base types.
using AssignPatchAdapter = PatchAdapter<AssignPatch>;
using BoolPatchAdapter = PatchAdapter<BoolPatch>;
using NumberPatchAdapter = PatchAdapter<NumberPatch>;
using StringPatchAdapter = PatchAdapter<StringPatch>;

// Adapter for all optional values.
using OptionalPatchAdapter = PatchAdapter<OptionalPatch>;

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
