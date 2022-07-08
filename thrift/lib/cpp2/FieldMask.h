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

#include <thrift/lib/cpp2/type/Field.h>
#include <thrift/lib/thrift/gen-cpp2/protocol_types.h>

namespace apache::thrift::protocol {
// Removes masked fields in schemaless Thrift Object (Protocol Object).
// Throws a runtime exception if the mask and object are incompatible.
void clear(const Mask& mask, protocol::Object& t);

// Copies masked fields from one object to another (schemaless).
// If the masked field doesn't exist in src, the field in dst will be removed.
// Throws a runtime exception if the mask and objects are incompatible.
void copy(const Mask& mask, const protocol::Object& src, protocol::Object& dst);

namespace detail {

// MaskRef struct represents the Field Mask and whether the mask is coming from
// excludes mask. MaskRef is used for inputs and outputs for Field Mask
// methods to determine the status of the mask, because if the mask is coming
// from excludes mask, the mask actually represents the complement set.
class MaskRef {
 public:
  const Mask& mask;
  bool is_exclusion = false; // Whether the mask comes from excludes mask

  // Get nested MaskRef with the given field id. If the id does not exist in the
  // map, it returns noneMask or fullMask depending on whether the field should
  // be included.
  MaskRef get(FieldId id) const;

  // Returns whether the ref includes all fields.
  bool isAllMask() const;

  // Returns whether the ref includes no fields.
  bool isNoneMask() const;

  // Returns whether the ref is logically exclusive in context.
  bool isExclusive() const;

  // Removes masked fields in schemaless Thrift Object (Protocol Object).
  // Throws a runtime exception if the mask and object are incompatible.
  void clear(protocol::Object& t) const;

  // Copies masked fields from one object to another (schemaless).
  // If the masked field doesn't exist in src, the field in dst will be removed.
  // Throws a runtime exception if the mask and objects are incompatible.
  void copy(const protocol::Object& src, protocol::Object& dst) const;

 private:
  // Gets all fields that need to be copied from src to dst.
  // Only contains fields either in src or dst.
  std::unordered_set<FieldId> getFieldsToCopy(
      const protocol::Object& src, const protocol::Object& dst) const;
};

// Throws an error if the given value is not a dynamic object value.
void errorIfNotObject(const protocol::Value& value);

template <typename StructTag, size_t... I>
bool validate_fields(MaskRef ref, std::index_sequence<I...>) {
  std::unordered_set<FieldId> ids{
      (field::id<StructTag, field_ordinal<I + 1>>())...};
  const FieldIdToMask& map = ref.mask.includes_ref()
      ? ref.mask.includes_ref().value()
      : ref.mask.excludes_ref().value();
  for (auto& [id, _] : map) {
    // Mask contains a field not in the struct.
    if (!ids.contains(FieldId{id})) {
      return false;
    }
  }
  // Validates each field in the struct.
  return (... && validate_field<StructTag, I + 1>(ref));
}

template <typename StructTag, size_t I>
bool validate_field(MaskRef ref) {
  MaskRef next = ref.get(field::id<StructTag, field_ordinal<I>>());
  if (next.isAllMask() || next.isNoneMask()) {
    return true;
  }
  // Check if the field is a thrift struct type. It uses type::native_type to
  // extract the type as we don't support adapted struct fields in field mask.
  using Nested =
      type::native_type<field::type_tag<StructTag, field_ordinal<I>>>;
  if constexpr (is_thrift_struct_v<Nested>) {
    // Need to validate the nested struct.
    return is_compatible_with<Nested>(next.mask);
  }
  return false;
}

template <typename T>
using get_ordinal_sequence =
    std::make_integer_sequence<size_t, type::field_size_v<type::struct_t<T>>>;
} // namespace detail

// Returns whether field mask is compatible with thrift struct T.
// It is incompatible if the mask contains a field that doesn't exist in the
// struct or that exists with a different type.
template <typename T>
bool is_compatible_with(const Mask& mask) {
  detail::MaskRef ref{mask, false};
  if (ref.isAllMask() || ref.isNoneMask()) {
    return true;
  }
  return detail::validate_fields<type::struct_t<T>>(
      ref, detail::get_ordinal_sequence<T>{});
}
} // namespace apache::thrift::protocol
