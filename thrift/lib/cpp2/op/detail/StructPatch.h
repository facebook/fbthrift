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

namespace apache {
namespace thrift {
namespace op {
namespace detail {

// Helpers for unpacking and folding field tags.
template <FieldId Id, typename P, typename T>
void applyFieldPatch(const P& patch, T& val) {
  op::getById<Id>(patch)->apply(op::getById<Id>(val));
}
template <FieldId Id, typename P1, typename P2>
void mergeFieldPatch(P1& lhs, const P2& rhs) {
  op::getById<Id>(lhs)->merge(*op::getById<Id>(rhs));
}
template <FieldId Id, typename P, typename T>
void forwardFromFields(T&& from, P& to) {
  *op::getById<Id>(to) = op::getById<Id>(std::forward<T>(from));
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
  static void forwardFrom(T&& from, P& to) {
    (...,
     forwardFromFields<type::field_id_v<FieldTags>>(std::forward<T>(from), to));
  }
};

// Requires Patch have fields with ids 1:1 with the fields they patch.
template <typename Patch>
class StructuredPatch : public BasePatch<Patch, StructuredPatch<Patch>> {
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

  Patch& toThrift() & noexcept { return patch_; }
  Patch* operator->() noexcept { return &patch_; }
  const Patch* operator->() const noexcept { return &patch_; }
  Patch& operator*() noexcept { return patch_; }
  const Patch& operator*() const noexcept { return patch_; }

  template <typename T>
  void assignFrom(T&& val) {
    FieldPatch<T>::forwardFrom(std::forward<T>(val), patch_);
  }

  template <typename T>
  void apply(T& val) const {
    FieldPatch<T>::apply(patch_, val);
  }

  template <typename U>
  void merge(U&& next) {
    FieldPatch<Patch>::merge(patch_, std::forward<U>(next).toThrift());
  }

 private:
  using Base::patch_;
  template <typename T>
  using Fields = ::apache::thrift::detail::st::struct_private_access::fields<T>;
  template <typename T>
  using FieldPatch = detail::FieldPatch<Fields<T>>;

  friend bool operator==(
      const StructuredPatch& lhs, const StructuredPatch& rhs) {
    return lhs.patch_ == rhs.patch_;
  }
  friend bool operator==(const StructuredPatch& lhs, const Patch& rhs) {
    return lhs.patch_ == rhs;
  }
  friend bool operator==(const Patch& lhs, const StructuredPatch& rhs) {
    return lhs == rhs.patch_;
  }
  friend bool operator!=(
      const StructuredPatch& lhs, const StructuredPatch& rhs) {
    return lhs.patch_ != rhs.patch_;
  }
  friend bool operator!=(const StructuredPatch& lhs, const Patch& rhs) {
    return lhs.patch_ != rhs;
  }
  friend bool operator!=(const Patch& lhs, const StructuredPatch& rhs) {
    return lhs != rhs.patch_;
  }
};

// Patch must have the following fields:
//   optional T assign;
//   bool clear;
//   P patch;
template <typename Patch>
class StructPatch : public BaseClearValuePatch<Patch, StructPatch<Patch>> {
  using Base = BaseClearValuePatch<Patch, StructPatch>;
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
    if (*patch_.clear()) {
      thrift::clear(val);
    }
    patch_.patch()->apply(val);
  }

  template <typename U>
  void merge(U&& next) {
    if (!mergeAssignAndClear(std::forward<U>(next))) {
      patch_.patch()->merge(*std::forward<U>(next).toThrift().patch());
    }
  }

 private:
  using Base::applyAssign;
  using Base::mergeAssignAndClear;
  using Base::patch_;
  using Base::resetAnd;
  using Fields = ::apache::thrift::detail::st::struct_private_access::fields<T>;
  using FieldPatch = detail::FieldPatch<Fields>;

  patch_type& ensurePatch() {
    if (patch_.assign().has_value()) {
      // Ensure even unknown fields are cleared.
      *patch_.clear() = true;

      // Split the assignment patch into a patch of assignments.
      patch_.patch()->assignFrom(std::move(*patch_.assign()));
      patch_.assign().reset();
    }
    return *patch_.patch();
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
class UnionPatch : public BaseEnsurePatch<Patch, UnionPatch<Patch>> {
  using Base = BaseEnsurePatch<Patch, UnionPatch>;
  using T = typename Base::value_type;
  using P = typename Base::value_patch_type;

 public:
  using Base::Base;
  using Base::operator=;
  using Base::apply;

  template <typename U = T>
  FOLLY_NODISCARD static UnionPatch createEnsure(U&& _default) {
    UnionPatch patch;
    patch.ensure(std::forward<U>(_default));
    return patch;
  }
  T& ensure() { return *patch_.ensure(); }
  P& ensure(const T& val) { return *ensureAnd(val).patchAfter(); }
  P& ensure(T&& val) { return *ensureAnd(std::move(val)).patchAfter(); }

  void apply(T& val) const { applyEnsure(val); }

  template <typename U>
  void merge(U&& next) {
    mergeEnsure(std::forward<U>(next));
  }

 private:
  using Base::applyEnsure;
  using Base::ensureAnd;
  using Base::mergeEnsure;
  using Base::patch_;
};

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
