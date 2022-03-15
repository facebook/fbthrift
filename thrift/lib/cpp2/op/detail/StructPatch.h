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
  using Base::apply;
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

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache
