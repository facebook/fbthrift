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

#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/op/detail/BasePatch.h>
#include <thrift/lib/cpp2/type/Field.h>
#include <thrift/lib/cpp2/type/NativeType.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

// Helpers for unpacking and folding field tags.
template <typename Tag, typename Id, typename P, typename T>
void applySubPatch(const P& patch, T& val) {
  op::get<Tag, Id>(patch)->apply(op::get<Tag, Id>(val));
}
template <typename Tag, typename Id, typename P1, typename P2>
void mergeSubPatch(P1& lhs, const P2& rhs) {
  op::get<Tag, Id>(lhs)->merge(*op::get<Tag, Id>(rhs));
}
template <typename Tag, typename Id, typename P, typename T>
void forwardFromSubPatch(T&& from, P& to) {
  *op::get<Tag, Id>(to) = op::get<Tag, Id>(std::forward<T>(from));
}
template <
    typename Tag,
    typename T = type::native_type<Tag>,
    typename F = std::make_index_sequence<type::field_size_v<Tag>>>
struct FieldPatch;
template <typename Tag, typename T, std::size_t... idx>
struct FieldPatch<Tag, T, std::integer_sequence<std::size_t, idx...>> {
  template <typename P>
  static void apply(const P& patch, T& val) {
    (...,
     applySubPatch<Tag, field::id<Tag, field_ordinal<idx + 1>>>(patch, val));
  }
  template <typename P1, typename P2>
  static void merge(P1& lhs, const P2& rhs) {
    (..., mergeSubPatch<Tag, field::id<Tag, field_ordinal<idx + 1>>>(lhs, rhs));
  }
  template <typename P>
  static void forwardFrom(T&& from, P& to) {
    (...,
     forwardFromSubPatch<Tag, field::id<Tag, field_ordinal<idx + 1>>>(
         std::forward<T>(from), to));
  }
};

// Requires Patch have fields with ids 1:1 with the fields they patch.
template <template <typename> class TTag, typename Patch>
class StructuredPatch : public BasePatch<Patch, StructuredPatch<TTag, Patch>> {
  using Base = BasePatch<Patch, StructuredPatch>;

 public:
  using Base::apply;
  using Base::Base;
  using Base::operator=;
  using Base::toThrift;

  template <typename T>
  static StructuredPatch createFrom(T&& val) {
    StructuredPatch patch;
    patch.assignFrom(std::forward<T>(val));
    return patch;
  }

  Patch& toThrift() & noexcept { return data_; }
  Patch* operator->() noexcept { return &data_; }
  const Patch* operator->() const noexcept { return &data_; }
  Patch& operator*() noexcept { return data_; }
  const Patch& operator*() const noexcept { return data_; }

  template <typename T>
  void assignFrom(T&& val) {
    FieldPatch<TTag<T>>::forwardFrom(std::forward<T>(val), data_);
  }

  template <typename T>
  void apply(T& val) const {
    FieldPatch<TTag<T>>::apply(data_, val);
  }

  template <typename U>
  void merge(U&& next) {
    FieldPatch<type::struct_t<Patch>>::merge(
        data_, std::forward<U>(next).toThrift());
  }

 private:
  using Base::data_;

  friend bool operator==(
      const StructuredPatch& lhs, const StructuredPatch& rhs) {
    return lhs.data_ == rhs.data_;
  }
  friend bool operator==(const StructuredPatch& lhs, const Patch& rhs) {
    return lhs.data_ == rhs;
  }
  friend bool operator==(const Patch& lhs, const StructuredPatch& rhs) {
    return lhs == rhs.data_;
  }
  friend bool operator!=(
      const StructuredPatch& lhs, const StructuredPatch& rhs) {
    return lhs.data_ != rhs.data_;
  }
  friend bool operator!=(const StructuredPatch& lhs, const Patch& rhs) {
    return lhs.data_ != rhs;
  }
  friend bool operator!=(const Patch& lhs, const StructuredPatch& rhs) {
    return lhs != rhs.data_;
  }
};

template <typename Patch>
using StructPatch = StructuredPatch<type::struct_t, Patch>;
template <typename Patch>
using UnionPatch = StructuredPatch<type::union_t, Patch>;

// Patch must have the following fields:
//   optional T assign;
//   bool clear;
//   P patch;
template <typename Patch>
class StructValuePatch
    : public BaseClearValuePatch<Patch, StructValuePatch<Patch>> {
  using Base = BaseClearValuePatch<Patch, StructValuePatch>;
  using T = typename Base::value_type;

 public:
  using Base::apply;
  using Base::Base;
  using Base::operator=;
  using patch_type = std::decay_t<decltype(*std::declval<Patch>().patch())>;

  // Convert to a patch, if needed, and return the
  // patch object.
  patch_type& patch() { return ensurePatch(); }
  auto* operator->() { return patch().operator->(); }

  void apply(T& val) const {
    if (applyAssign(val)) {
      return;
    }
    if (*data_.clear()) {
      thrift::clear(val);
    }
    data_.patch()->apply(val);
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssignAndClear(std::forward<U>(next))) {
      data_.patch()->merge(*std::forward<U>(next).toThrift().patch());
    }
  }

 private:
  using Base::applyAssign;
  using Base::data_;
  using Base::mergeAssignAndClear;

  patch_type& ensurePatch() {
    if (data_.assign().has_value()) {
      // Ensure even unknown fields are cleared.
      *data_.clear() = true;

      // Split the assignment patch into a patch of assignments.
      data_.patch()->assignFrom(std::move(*data_.assign()));
      data_.assign().reset();
    }
    return *data_.patch();
  }
};

// A patch for an union value.
//
// Patch must have the following fields:
//   bool clear;
//   P patch;
//   T ensure;
//   P patchAfter;
// Where P is the patch type for the union type T.
template <typename Patch>
class UnionValuePatch : public BaseEnsurePatch<Patch, UnionValuePatch<Patch>> {
  using Base = BaseEnsurePatch<Patch, UnionValuePatch>;
  using T = typename Base::value_type;
  using P = typename Base::value_patch_type;

 public:
  using Base::Base;
  using Base::operator=;
  using Base::apply;

  template <typename U = T>
  FOLLY_NODISCARD static UnionValuePatch createEnsure(U&& _default) {
    UnionValuePatch patch;
    patch.ensure(std::forward<U>(_default));
    return patch;
  }
  T& ensure() { return *data_.ensure(); }
  P& ensure(const T& val) { return *ensureAnd(val).patchAfter(); }
  P& ensure(T&& val) { return *ensureAnd(std::move(val)).patchAfter(); }

  void apply(T& val) const { applyEnsure(val); }

  // TODO: "From this I think the only 'safe' choice for now, is to make
  // applying a 'clearing' optional patch to a union_field, an error,
  // just like it is with a non-optional value. This also implies that
  // a union_field should not be bundled in with other 'optional' types."
  template <typename U>
  if_opt_type<folly::remove_cvref_t<U>> apply(U&& field) const {
    if (field.has_value()) {
      apply(*std::forward<U>(field));
    }
  }

  template <typename U>
  void merge(U&& next) {
    mergeEnsure(std::forward<U>(next));
  }

 private:
  using Base::applyEnsure;
  using Base::data_;
  using Base::ensureAnd;
  using Base::mergeEnsure;
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
