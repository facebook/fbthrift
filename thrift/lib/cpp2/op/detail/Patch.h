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

template <typename Patch, typename Derived>
class BasePatch {
 public:
  using value_type = std::decay_t<decltype(*std::declval<Patch>().assign())>;

  BasePatch() = default;
  explicit BasePatch(const Patch& patch) : patch_(patch) {}
  explicit BasePatch(Patch&& patch) noexcept : patch_(std::move(patch)) {}

  const Patch& get() const& { return patch_; }
  Patch&& get() && { return std::move(patch_); }

  void reset() { resetAnd(); }

  bool hasAssign() const noexcept { return patch_.assign().has_value(); }
  void assign(const value_type& val) { resetAnd().assign().emplace(val); }
  void assign(value_type&& val) { resetAnd().assign().emplace(std::move(val)); }

  static Derived createAssign(value_type&& val) {
    Derived patch;
    patch.assign(std::move(val));
    return patch;
  }
  static Derived createAssign(const value_type& val) {
    Derived patch;
    patch.assign(val);
    return patch;
  }

  Derived& operator=(const value_type& val) { return (assign(val), derived()); }
  Derived& operator=(value_type&& val) {
    assign(std::move(val));
    return derived();
  }

 protected:
  Patch patch_;

  ~BasePatch() = default; // abstract base class

  Patch& resetAnd() {
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
  static BoolPatch createInvert() { return !BoolPatch{}; }

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
  NumberPatch& operator+=(U&& val) noexcept {
    add(std::forward<U>(val));
    return *this;
  }
  template <typename U>
  static NumberPatch createAdd(U&& val) {
    NumberPatch patch;
    patch.add(std::forward<U>(val));
    return patch;
  }

  template <typename U>
  void subtract(U&& val) noexcept {
    assignOr(add_()) -= std::forward<U>(val);
  }
  template <typename U>
  NumberPatch& operator-=(U&& val) noexcept {
    subtract(std::forward<T>(val));
    return *this;
  }
  template <typename U>
  static NumberPatch createSubtract(U&& val) {
    NumberPatch patch;
    patch.subtract(std::forward<U>(val));
    return patch;
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
  StringPatch& operator+=(U&& val) {
    assignOr(append_()) += std::forward<U>(val);
    return *this;
  }
  template <typename... Args>
  static StringPatch createAppend(Args&&... args) {
    StringPatch patch;
    patch.append(std::forward<Args>(args)...);
    return patch;
  }

  template <typename U>
  void prepend(U&& val) {
    T& cur = assignOr(prepend_());
    cur = std::forward<U>(val) + std::move(cur);
  }
  template <typename U>
  static StringPatch createPrepend(U&& val) {
    StringPatch patch;
    patch.prepend(std::forward<U>(val));
    return patch;
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

template <typename Patch>
class StructPatch : public BasePatch<Patch, StructPatch<Patch>> {
  using Base = BasePatch<Patch, StructPatch>;
  using T = typename Base::value_type;
  using Base::applyAssign;
  using Base::mergeAssign;
  using Base::resetAnd;
  using Fields = ::apache::thrift::detail::st::struct_private_access::fields<T>;
  using FieldPatch = detail::FieldPatch<Fields>;

 public:
  using Base::Base;
  using Base::hasAssign;
  using Base::operator=;
  using patch_type = std::decay_t<decltype(*std::declval<Patch>().patch())>;

  bool empty() const noexcept {
    return !hasAssign() && !clear_() &&
        // TODO(afuller): Use terse writes and switch to op::empty.
        fieldPatch_() == patch_type{};
  }
  void apply(T& val) const noexcept {
    if (applyAssign(val)) {
      return;
    }
    if (clear_()) {
      thrift::clear(val);
    }
    FieldPatch::apply(fieldPatch_(), val);
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
      FieldPatch::merge(fieldPatch_(), *std::forward<U>(next).get().patch());
    }
  }

  void clear() { clear_() = true; }
  static StructPatch createClear() {
    StructPatch patch;
    patch.clear();
    return patch;
  }

  // Convert to a patch, if needed, and return the
  // patch object.
  patch_type& patch() { return ensurePatch(); }
  patch_type* operator->() { return &ensurePatch(); }

 private:
  bool& clear_() { return *this->patch_.clear(); }
  const bool& clear_() const { return *this->patch_.clear(); }
  patch_type& fieldPatch_() { return *this->patch_.patch(); }
  const patch_type& fieldPatch_() const { return *this->patch_.patch(); }

  patch_type& ensurePatch() {
    if (hasAssign()) {
      // Ensure even unknown fields are cleared.
      clear_() = true;

      // Split the assignment patch into a patch of assignments.
      FieldPatch::forwardTo(std::move(*this->patch_.assign()), fieldPatch_());
      this->patch_.assign().reset();
    }
    assert(!hasAssign());
    return fieldPatch_();
  }
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
