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
#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/type/Field.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

// Base class for all patch types.
// - Patch: The Thrift struct representation for the patch.
template <typename Patch>
class BasePatch {
 public:
  BasePatch() = default;
  explicit BasePatch(const Patch& patch) : patch_(patch) {}
  explicit BasePatch(Patch&& patch) noexcept : patch_(std::move(patch)) {}

  const Patch& get() const& { return patch_; }
  Patch&& get() && { return std::move(patch_); }

  void reset() { op::clear<type::struct_t<Patch>>(patch_); }

 protected:
  Patch patch_;

  ~BasePatch() = default; // abstract base class

  // A fluent version of 'reset()'.
  FOLLY_NODISCARD Patch& resetAnd() { return (reset(), patch_); }
};

// Base class for value patch.
//
// Patch must have the following fields:
//   optional T assign;
template <typename Patch, typename Derived>
class BaseValuePatch : public BasePatch<Patch> {
  using Base = BasePatch<Patch>;

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

  Derived& derived() { return static_cast<Derived&>(*this); }
  const Derived& derived() const { return static_cast<Derived&>(*this); }
};

// A patch adapter that only supports 'assign',
// which is the minimum any patch should support.
//
// Patch must have the following fields:
//   optional T assign;
template <typename Patch>
class AssignPatch : public BaseValuePatch<Patch, AssignPatch<Patch>> {
  using Base = BaseValuePatch<Patch, AssignPatch>;
  using T = typename Base::value_type;

 public:
  using Base::Base;
  using Base::hasAssign;
  using Base::operator=;

  bool empty() const { return !hasAssign(); }
  void apply(T& val) const { applyAssign(val); }
  template <typename U>
  void merge(U&& next) {
    mergeAssign(std::forward<U>(next));
  }

 private:
  using Base::applyAssign;
  using Base::mergeAssign;
};

// Patch must have the following fields:
//   optional T assign;
//   bool invert;
template <typename Patch>
class BoolPatch : public BaseValuePatch<Patch, BoolPatch<Patch>> {
  using Base = BaseValuePatch<Patch, BoolPatch>;
  using T = typename Base::value_type;

 public:
  using Base::Base;
  using Base::get;
  using Base::hasAssign;
  using Base::operator=;

  static BoolPatch createInvert() { return !BoolPatch{}; }

  void invert() {
    auto& val = assignOr(*patch_.invert());
    val = !val;
  }

  bool empty() const { return !hasAssign() && !*patch_.invert(); }

  void apply(T& val) const {
    if (!applyAssign(val) && *patch_.invert()) {
      val = !val;
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssign(std::forward<U>(next))) {
      *patch_.invert() ^= *next.get().invert();
    }
  }

 private:
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssign;
  using Base::patch_;

  friend BoolPatch operator!(BoolPatch val) { return (val.invert(), val); }
};

// Patch must have the following fields:
//   optional T assign;
//   T add;
template <typename Patch>
class NumberPatch : public BaseValuePatch<Patch, NumberPatch<Patch>> {
  using Base = BaseValuePatch<Patch, NumberPatch>;
  using T = typename Base::value_type;

 public:
  using Base::Base;
  using Base::hasAssign;
  using Base::operator=;

  template <typename U>
  static NumberPatch createAdd(U&& val) {
    NumberPatch patch;
    patch.add(std::forward<U>(val));
    return patch;
  }
  template <typename U>
  static NumberPatch createSubtract(U&& val) {
    NumberPatch patch;
    patch.subtract(std::forward<U>(val));
    return patch;
  }

  template <typename U>
  void add(U&& val) {
    assignOr(*patch_.add()) += std::forward<U>(val);
  }

  template <typename U>
  void subtract(U&& val) {
    assignOr(*patch_.add()) -= std::forward<U>(val);
  }

  bool empty() const { return !hasAssign() && *patch_.add() == 0; }

  void apply(T& val) const {
    if (!applyAssign(val)) {
      val += *patch_.add();
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssign(std::forward<U>(next))) {
      *patch_.add() += *next.get().add();
    }
  }

  template <typename U>
  NumberPatch& operator+=(U&& val) {
    add(std::forward<U>(val));
    return *this;
  }

  template <typename U>
  NumberPatch& operator-=(U&& val) {
    subtract(std::forward<T>(val));
    return *this;
  }

 private:
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssign;
  using Base::patch_;

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
};

// Patch must have the following fields:
//   optional T assign;
//   T append;
//   T prepend;
template <typename Patch>
class StringPatch : public BaseValuePatch<Patch, StringPatch<Patch>> {
  using Base = BaseValuePatch<Patch, StringPatch>;
  using T = typename Base::value_type;

 public:
  using Base::Base;
  using Base::hasAssign;
  using Base::operator=;

  template <typename... Args>
  static StringPatch createAppend(Args&&... args) {
    StringPatch patch;
    patch.append(std::forward<Args>(args)...);
    return patch;
  }

  template <typename U>
  static StringPatch createPrepend(U&& val) {
    StringPatch patch;
    patch.prepend(std::forward<U>(val));
    return patch;
  }

  template <typename... Args>
  void append(Args&&... args) {
    assignOr(*patch_.append()).append(std::forward<Args>(args)...);
  }

  template <typename U>
  void prepend(U&& val) {
    T& cur = assignOr(*patch_.prepend());
    cur = std::forward<U>(val) + std::move(cur);
  }

  bool empty() const {
    return !hasAssign() && patch_.prepend()->empty() &&
        patch_.append()->empty();
  }

  void apply(T& val) const {
    if (!applyAssign(val)) {
      val = *patch_.prepend() + val + *patch_.append();
    }
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssign(std::forward<U>(next))) {
      *patch_.prepend() =
          *std::forward<U>(next).get().prepend() + std::move(*patch_.prepend());
      patch_.append()->append(*std::forward<U>(next).get().append());
    }
  }

  template <typename U>
  StringPatch& operator+=(U&& val) {
    assignOr(*patch_.append()) += std::forward<U>(val);
    return *this;
  }

 private:
  using Base::applyAssign;
  using Base::assignOr;
  using Base::mergeAssign;
  using Base::patch_;

  template <typename U>
  friend StringPatch operator+(StringPatch lhs, U&& rhs) {
    return lhs += std::forward<U>(rhs);
  }
  template <typename U>
  friend StringPatch operator+(U&& lhs, StringPatch rhs) {
    rhs.prepend(std::forward<U>(lhs));
    return rhs;
  }
};

// Helpers for unpacking and folding field tags.
template <FieldId Id, typename P, typename T>
void applyFieldPatch(const P& patch, T& val) {
  op::getById<Id>(patch)->apply(*op::getById<Id>(val));
}
template <FieldId Id, typename P1, typename P2>
void mergeFieldPatch(P1& lhs, const P2& rhs) {
  op::getById<Id>(lhs)->merge(*op::getById<Id>(rhs));
}
template <FieldId Id, typename P, typename T>
void forwardToFieldPatch(P& patch, T&& val) {
  op::getById<Id>(patch)->assign(*op::getById<Id>(std::forward<T>(val)));
}
template <typename F>
struct FieldPatch;
template <typename... FieldTags>
struct FieldPatch<type::fields<FieldTags...>> {
  template <typename P, typename T>
  static void apply(const P& patch, T& val) {
    (..., applyFieldPatch<type::field_id_v<FieldTags>>(patch, val));
  }
  template <typename P1, typename P2>
  static void merge(P1& lhs, const P2& rhs) {
    (..., mergeFieldPatch<type::field_id_v<FieldTags>>(lhs, rhs));
  }
  template <typename T, typename P>
  static void forwardTo(T&& val, P& patch) {
    (...,
     forwardToFieldPatch<type::field_id_v<FieldTags>>(
         patch, std::forward<T>(val)));
  }
};

// Patch must have the following fields:
//   optional T assign;
//   bool clear;
//   P patch;
template <typename Patch>
class StructPatch : public BaseValuePatch<Patch, StructPatch<Patch>> {
  using Base = BaseValuePatch<Patch, StructPatch>;
  using T = typename Base::value_type;

 public:
  using Base::Base;
  using Base::hasAssign;
  using Base::operator=;
  using patch_type = std::decay_t<decltype(*std::declval<Patch>().patch())>;

  static StructPatch createClear() {
    StructPatch patch;
    patch.clear();
    return patch;
  }

  void clear() { *patch_.clear() = true; }

  // Convert to a patch, if needed, and return the
  // patch object.
  patch_type& patch() { return ensurePatch(); }
  patch_type* operator->() { return &ensurePatch(); }

  bool empty() const {
    return !hasAssign() && !*patch_.clear() &&
        // TODO(afuller): Use terse writes and switch to op::empty.
        *patch_.patch() == patch_type{};
  }
  void apply(T& val) const {
    if (applyAssign(val)) {
      return;
    }
    if (*patch_.clear()) {
      thrift::clear(val);
    }
    FieldPatch::apply(*patch_.patch(), val);
  }

  template <typename U>
  void merge(U&& next) {
    // Clear is slightly stronger than assigning a 'cleared' struct,
    // in the presense of non-terse, non-optional fields with custom defaults
    // and missmatched schemas... it's also smaller, so prefer it.
    if (*next.get().clear() && !next.hasAssign()) {
      // Next patch completely replaces this one.
      *this = std::forward<U>(next);
    } else if (!mergeAssign(std::forward<U>(next))) {
      // Merge field patches.
      FieldPatch::merge(*patch_.patch(), *std::forward<U>(next).get().patch());
    }
  }

 private:
  using Base::applyAssign;
  using Base::mergeAssign;
  using Base::patch_;
  using Base::resetAnd;
  using Fields = ::apache::thrift::detail::st::struct_private_access::fields<T>;
  using FieldPatch = detail::FieldPatch<Fields>;

  patch_type& ensurePatch() {
    if (hasAssign()) {
      // Ensure even unknown fields are cleared.
      *patch_.clear() = true;

      // Split the assignment patch into a patch of assignments.
      FieldPatch::forwardTo(std::move(*patch_.assign()), *patch_.patch());
      patch_.assign().reset();
    }
    assert(!hasAssign());
    return *patch_.patch();
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
using AssignPatchAdapter = PatchAdapter<AssignPatch>;
using BoolPatchAdapter = PatchAdapter<BoolPatch>;
using NumberPatchAdapter = PatchAdapter<NumberPatch>;
using StringPatchAdapter = PatchAdapter<StringPatch>;

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
