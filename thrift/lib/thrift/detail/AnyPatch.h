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

#include <vector>
#include <fmt/format.h>
#include <folly/container/F14Map.h>
#include <thrift/lib/cpp2/Adapter.h>
#include <thrift/lib/cpp2/op/detail/BasePatch.h>
#include <thrift/lib/cpp2/protocol/Patch.h>
#include <thrift/lib/cpp2/type/Type.h>
#include <thrift/lib/thrift/detail/DynamicPatch.h>
#include <thrift/lib/thrift/gen-cpp2/any_patch_detail_types.h>
#include <thrift/lib/thrift/gen-cpp2/any_rep_types.h>

namespace apache {
namespace thrift {
namespace op {
namespace detail {

template <typename Patch>
class AnyPatch;

} // namespace detail

class AnyPatchStruct;
class TypeToPatchInternalDoNotUse;
class AnyPatchStruct;
class AnySafePatch;

namespace detail {
[[noreturn]] void throwDuplicatedType(const type::Type& type);
[[noreturn]] void throwTypeNotValid(const type::Type& type);
[[noreturn]] void throwAnyNotValid(const type::AnyStruct& any);
[[noreturn]] void throwUnsupportedAnyProtocol(const type::AnyStruct& any);

inline void throwIfInvalidOrUnsupportedAny(const type::AnyStruct& any) {
  if (!type::AnyData::isValid(any)) {
    throwAnyNotValid(any);
  }
  if (any.protocol() != type::Protocol::get<type::StandardProtocol::Binary>() &&
      any.protocol() !=
          type::Protocol::get<type::StandardProtocol::Compact>()) {
    throwUnsupportedAnyProtocol(any);
  }
}

struct TypeToPatchMapAdapter {
  using StandardType = std::vector<TypeToPatchInternalDoNotUse>;
  using AdaptedType = folly::F14FastMap<type::Type, protocol::DynamicPatch>;

  static AdaptedType fromThrift(StandardType&& vec);
  static StandardType toThrift(const AdaptedType& map);

  template <typename Tag, typename Protocol>
  static uint32_t encode(Protocol& prot, const AdaptedType& map) {
    uint32_t s = 0;
    s += prot.writeListBegin(protocol::TType::T_STRUCT, map.size());
    for (const auto& [type, patch] : map) {
      s += prot.writeStructBegin(
          op::get_class_name_v<TypeToPatchInternalDoNotUse>.data());
      s += prot.writeFieldBegin("type", protocol::TType::T_STRUCT, 1);
      s += op::encode<type::infer_tag<type::Type>>(prot, type);
      s += prot.writeFieldEnd();
      s += prot.writeFieldBegin("patch", protocol::TType::T_STRUCT, 2);
      s +=
          op::encode<type::struct_t<type::AnyStruct>>(prot, toAny(patch, type));
      s += prot.writeFieldEnd();
      s += prot.writeFieldStop();
      s += prot.writeStructEnd();
    }
    s += prot.writeListEnd();
    return s;
  }

  template <typename Tag, typename Protocol>
  static void decode(Protocol& prot, AdaptedType& map) {
    protocol::TType t;
    uint32_t s;
    prot.readListBegin(t, s);
    if (t != protocol::TType::T_STRUCT) {
      while (s--) {
        prot.skip(t);
      }
    } else {
      while (s--) {
        TypeToPatchInternalDoNotUse typeToPatchStruct;
        op::decode<type::struct_t<TypeToPatchInternalDoNotUse>>(
            prot, typeToPatchStruct);
        if (!addDynamicPatchToMap(map, typeToPatchStruct)) {
          throwDuplicatedType(typeToPatchStruct.type().value());
        }
      }
    }
    prot.readListEnd();
  }

  static bool equal(const AdaptedType&, const AdaptedType&);

 private:
  static bool addDynamicPatchToMap(
      AdaptedType&, const TypeToPatchInternalDoNotUse&);

  static type::AnyStruct toAny(
      const protocol::DynamicPatch&, const type::Type&);
};

/// Patch for Thrift Any.
/// * `optional AnyStruct assign`
/// * `terse bool clear`
/// * `terse map<Type, DynamicPatch> patchIfTypeIsPrior`
/// * `optional AnyStruct ensureAny`
/// * `terse map<Type, DynamicPatch> patchIfTypeIsAfter`
template <typename Patch>
class AnyPatch : public BaseClearPatch<Patch, AnyPatch<Patch>> {
  using Base = BaseClearPatch<Patch, AnyPatch>;

 public:
  using Base::apply;
  using Base::Base;
  using Base::operator=;
  using Base::clear;

  /// @copybrief AssignPatch::customVisit
  ///
  /// Users should provide a visitor with the following methods
  ///
  ///     struct Visitor {
  ///       void assign(const AnyStruct&);
  ///       void clear();
  ///       void patchIfTypeIs(const Type&, const DynamicPatch&);
  ///       void ensureAny(const AnyStruct&);
  ///     }
  ///
  template <typename Visitor>
  void customVisit(Visitor&& v) const {
    if (false) {
      // Test whether the required methods exist in Visitor
      v.assign(type::AnyStruct{});
      v.clear();
      v.patchIfTypeIs(type::Type{}, protocol::DynamicPatch{});
      v.ensureAny(type::AnyStruct{});
    }
    if (!Base::template customVisitAssignAndClear(v)) {
      // patchIfTypeIsPrior
      for (const auto& [type, patch] : data_.patchIfTypeIsPrior().value()) {
        v.patchIfTypeIs(type, patch);
      }

      // ensureAny
      if (data_.ensureAny().has_value()) {
        v.ensureAny(data_.ensureAny().value());
      }

      // patchIfTypeIsAfter
      for (const auto& [type, patch] : data_.patchIfTypeIsAfter().value()) {
        v.patchIfTypeIs(type, patch);
      }
    }
  }

  void apply(type::AnyStruct& val) const;

  /// Ensures the given type is set in Thrift Any.
  void ensureAny(type::AnyStruct ensureAny) {
    throwIfInvalidOrUnsupportedAny(ensureAny);
    if (data_.assign().has_value()) {
      data_.clear() = true;
      data_.ensureAny() = std::move(data_.assign().value());
      data_.assign().reset();
    }

    if (ensures(ensureAny.type().value())) {
      return;
    }

    data_.ensureAny() = std::move(ensureAny);
  }

  /// Patches the value in Thrift Any if the type matches with the provided
  /// patch.
  template <typename VPatch>
  void patchIfTypeIs(const VPatch& patch) {
    // TODO(dokwon): Refactor PatchTrait to use is_patch_v.
    static_assert(std::is_base_of_v<
                  BasePatch<typename VPatch::underlying_type, VPatch>,
                  VPatch>);
    tryPatchable<VPatch>();
    patchIfTypeIsImpl(
        patch, ensures<type::infer_tag<typename VPatch::value_type>>());
  }

  /// Ensures the given value type is set in Thrift Any, and patches the value
  /// in Thrift Any if the type matches with the provided patch.
  template <typename VPatch>
  void ensureAndPatch(const VPatch& patch) {
    // TODO(dokwon): Refactor PatchTrait to use is_patch_v.
    static_assert(std::is_base_of_v<
                  BasePatch<typename VPatch::underlying_type, VPatch>,
                  VPatch>);
    using VTag = type::infer_tag<typename VPatch::value_type>;
    ensureAny(type::AnyData::toAny<VTag>({}).toThrift());
    patchIfTypeIsImpl(patch, true);
  }

  // The provided type MUST match with the value type of patch stored in
  // provided patch as Thrift Any.
  void patchIfTypeIs(type::Type type, type::AnyStruct patch) {
    if (!type.isValid()) {
      throwTypeNotValid(type);
    }
    throwIfInvalidOrUnsupportedAny(patch);
    tryPatchable(type);
    bool ensure = ensures(type);
    patchIfTypeIsImpl(std::move(type), std::move(patch), ensure);
  }

  // The provided type in ensureAny MUST match with the value type of patch
  // stored in provided patch as Thrift Any.
  void ensureAndPatch(type::AnyStruct ensure, type::AnyStruct patch) {
    throwIfInvalidOrUnsupportedAny(ensure);
    throwIfInvalidOrUnsupportedAny(patch);
    type::Type type = ensure.type().value();
    ensureAny(std::move(ensure));
    patchIfTypeIsImpl(std::move(type), std::move(patch), true);
  }

 private:
  using Base::assignOr;
  using Base::data_;
  using Base::hasAssign;

  bool ensures(const type::Type& type) {
    return data_.ensureAny().has_value() &&
        type::identicalType(data_.ensureAny()->type().value(), type);
  }

  template <typename Tag>
  bool ensures() {
    return ensures(type::Type::create<Tag>());
  }

  // If assign has value and specified 'VPatch' is the corresponding patch
  // type to the type in 'assign' operation, we ensure the patch is patchable
  // by making it to 'clear' + 'ensureAny' operation.
  template <typename VPatch>
  void tryPatchable() {
    using VType = typename VPatch::value_type;
    using VTag = type::infer_tag<VType>;
    tryPatchable(type::Type::create<VTag>());
  }
  void tryPatchable(const type::Type& type) {
    if (data_.assign().has_value()) {
      if (!type::identicalType(data_.assign()->type().value(), type)) {
        return;
      }
      data_.clear() = true;
      ensureAny(std::move(data_.assign().value()));
      data_.assign().reset();
    }
  }

  template <typename VPatch>
  void patchIfTypeIsImpl(const VPatch& patch, bool after) {
    auto type =
        type::Type::create<type::infer_tag<typename VPatch::value_type>>();
    auto anyStruct =
        type::AnyData::toAny<type::infer_tag<VPatch>>(patch).toThrift();
    patchIfTypeIsImpl(type, std::move(anyStruct), after);
  }

  void patchIfTypeIsImpl(type::Type type, type::AnyStruct any, bool after);

  // Needed for merge.
  void patchIfTypeIs(
      const type::Type& type, const protocol::DynamicPatch& patch);
};

template <class T>
struct PatchType;
template <class T>
struct SafePatchType;
template <class T>
struct SafePatchValueType;

template <>
struct PatchType<type::struct_t<::apache::thrift::type::AnyStruct>> {
  using type = AnyPatch<::apache::thrift::op::AnyPatchStruct>;
};

template <>
struct SafePatchType<type::struct_t<::apache::thrift::type::AnyStruct>> {
  using type = ::apache::thrift::op::AnySafePatch;
};

template <>
struct SafePatchValueType<::apache::thrift::op::AnySafePatch> {
  using type = ::apache::thrift::type::AnyStruct;
};

// Adapters for Thrift Any
template <typename T>
using AnyPatchAdapter = InlineAdapter<AnyPatch<T>>;

} // namespace detail
} // namespace op
} // namespace thrift
} // namespace apache