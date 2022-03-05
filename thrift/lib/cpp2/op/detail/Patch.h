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

#include <thrift/lib/cpp2/op/Clear.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

template <typename Patch, typename Derived>
class BasePatch {
 public:
  using value_type = std::decay_t<decltype(*std::declval<Patch>().assign())>;

  BasePatch() = default;
  explicit BasePatch(const Patch& patch) : patch_(patch) {}
  explicit BasePatch(Patch&& patch) noexcept : patch_(std::move(patch)) {}

  const Patch& get() const& { return patch_; }
  Patch&& get() && { return std::move(patch_); }

  void clear() { clearAnd(); }

  bool hasAssign() const noexcept { return patch_.assign().has_value(); }
  void assign(const value_type& val) { clearAnd().assign().emplace(val); }
  void assign(value_type&& val) { clearAnd().assign().emplace(std::move(val)); }

  Derived& operator=(const value_type& val) { return (assign(val), derived()); }
  Derived& operator=(value_type&& val) {
    assign(std::move(val));
    return derived();
  }

 protected:
  Patch patch_;

  ~BasePatch() = default; // abstract base class

  Patch& clearAnd() {
    op::clear<type::struct_t<Patch>>(patch_);
    return patch_;
  }

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

  Derived& derived() { return static_cast<Derived&>(*this); }
  const Derived& derived() const { return static_cast<Derived&>(*this); }
};

template <typename Patch>
class BoolPatch : public BasePatch<Patch, BoolPatch<Patch>> {
  using Base = BasePatch<Patch, BoolPatch>;
  using T = typename Base::value_type;
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssign;

 public:
  using Base::Base;
  using Base::get;
  using Base::hasAssign;
  using Base::operator=;

  bool empty() const noexcept { return !hasAssign() && !invert_(); }

  void apply(T& val) const noexcept {
    if (!applyAssign(val) && invert_()) {
      val = !val;
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssign(std::forward<U>(next))) {
      invert_() ^= *next.get().invert();
    }
  }

  void invert() noexcept {
    auto& val = assignOr(invert_());
    val = !val;
  }

 private:
  friend BoolPatch operator!(BoolPatch val) { return (val.invert(), val); }

  T& invert_() noexcept { return *this->patch_.invert(); }
  const T& invert_() const noexcept { return *this->patch_.invert(); }
};

template <typename Patch>
class NumberPatch : public BasePatch<Patch, NumberPatch<Patch>> {
  using Base = BasePatch<Patch, NumberPatch>;
  using T = typename Base::value_type;
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssign;

 public:
  using Base::Base;
  using Base::hasAssign;
  using Base::operator=;

  bool empty() const noexcept { return !hasAssign() && add_() == 0; }

  void apply(T& val) const noexcept {
    if (!applyAssign(val)) {
      val += add_();
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssign(std::forward<U>(next))) {
      add_() += *next.get().add();
    }
  }

  template <typename U>
  void add(U&& val) {
    assignOr(add_()) += std::forward<U>(val);
  }
  template <typename U>
  void subtract(U&& val) noexcept {
    assignOr(add_()) -= std::forward<U>(val);
  }

  template <typename U>
  NumberPatch& operator+=(U&& val) noexcept {
    add(std::forward<U>(val));
    return *this;
  }
  template <typename U>
  NumberPatch& operator-=(U&& val) noexcept {
    subtract(std::forward<T>(val));
    return *this;
  }

 private:
  template <typename U>
  friend NumberPatch operator+(NumberPatch lhs, U&& rhs) {
    lhs.add(std::forward<U>(rhs));
    return lhs;
  }

  template <typename U>
  friend NumberPatch operator+(U&& lhs, NumberPatch rhs) {
    rhs.add(std::forward<U>(lhs));
    return rhs;
  }

  template <typename U>
  friend NumberPatch operator-(NumberPatch lhs, U&& rhs) {
    lhs.subtract(std::forward<U>(rhs));
    return lhs;
  }

  T& add_() noexcept { return *this->patch_.add(); }
  const T& add_() const noexcept { return *this->patch_.add(); }
};

template <typename Patch>
class StringPatch : public BasePatch<Patch, StringPatch<Patch>> {
  using Base = BasePatch<Patch, StringPatch>;
  using T = typename Base::value_type;
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssign;

 public:
  using Base::Base;
  using Base::hasAssign;
  using Base::operator=;

  bool empty() const noexcept {
    return !hasAssign() && prepend_().empty() && append_().empty();
  }

  void apply(T& val) const {
    if (!applyAssign(val)) {
      val = prepend_() + val + append_();
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssign(std::forward<U>(next))) {
      prepend_() =
          *std::forward<U>(next).get().prepend() + std::move(prepend_());
      append_().append(*std::forward<U>(next).get().append());
    }
  }

  template <typename... Args>
  void append(Args&&... args) {
    assignOr(append_()).append(std::forward<Args>(args)...);
  }

  template <typename U>
  void prepend(U&& val) {
    T& cur = assignOr(prepend_());
    cur = std::forward<U>(val) + std::move(cur);
  }

  template <typename U>
  StringPatch& operator+=(U&& val) {
    assignOr(append_()) += std::forward<U>(val);
    return *this;
  }

 private:
  template <typename U>
  friend StringPatch operator+(StringPatch lhs, U&& rhs) {
    return lhs += std::forward<U>(rhs);
  }
  template <typename U>
  friend StringPatch operator+(U&& lhs, StringPatch rhs) {
    rhs.prepend(std::forward<U>(lhs));
    return rhs;
  }

  T& append_() noexcept { return *this->patch_.append(); }
  const T& append_() const noexcept { return *this->patch_.append(); }
  T& prepend_() noexcept { return *this->patch_.prepend(); }
  const T& prepend_() const noexcept { return *this->patch_.prepend(); }
};

// A patch adapter that only supports 'assign',
// which is the minimum any patch should support.
template <typename Patch>
class AssignPatch : public BasePatch<Patch, AssignPatch<Patch>> {
  using Base = BasePatch<Patch, AssignPatch>;
  using T = typename Base::value_type;
  using Base::applyAssign;
  using Base::mergeAssign;

 public:
  using Base::Base;
  using Base::hasAssign;
  using Base::operator=;

  bool empty() const noexcept { return !hasAssign(); }
  void apply(T& val) const noexcept { applyAssign(val); }
  template <typename U>
  void merge(U&& next) {
    mergeAssign(std::forward<U>(next));
  }
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
using BoolPatchAdapter = PatchAdapter<BoolPatch>;
using NumberPatchAdapter = PatchAdapter<NumberPatch>;
using StringPatchAdapter = PatchAdapter<StringPatch>;
using AssignPatchAdapter = PatchAdapter<AssignPatch>;

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
