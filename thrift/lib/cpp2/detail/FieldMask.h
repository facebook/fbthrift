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

#include <thrift/lib/cpp2/op/Clear.h>
#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/thrift/gen-cpp2/field_mask_types.h>
#include <thrift/lib/thrift/gen-cpp2/protocol_types.h>

namespace apache::thrift::protocol::detail {

// MaskRef struct represents the Field Mask and whether the mask is coming from
// excludes mask. MaskRef is used for inputs and outputs for Field Mask
// methods to determine the status of the mask, because if the mask is coming
// from excludes mask, the mask actually represents the complement set.
class MaskRef {
 public:
  const Mask& mask;
  bool is_exclusion = false; // Whether the mask comes from excludes mask

  // Get nested MaskRef with the given field id. If the id does not exist in the
  // map, it returns noneMask or allMask depending on whether the field should
  // be included.
  MaskRef get(FieldId id) const;

  // Returns whether the ref includes all fields.
  bool isAllMask() const;

  // Returns whether the ref includes no fields.
  bool isNoneMask() const;

  // Returns whether the ref is logically exclusive in context.
  bool isExclusive() const;

  // Returns true if the mask is a field mask.
  bool isFieldMask() const;

  // Returns true if the mask is a map mask.
  bool isMapMask() const;

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

// Throws an error if a thrift struct type is not compatible with the mask.
// TODO(aoka): Check compatibility in ensure, clear, and copy methods.
template <typename T>
void errorIfNotCompatible(const Mask& mask) {
  if (!is_compatible_with<T>(mask)) {
    throw std::runtime_error("The field mask and struct are incompatible.");
  }
}

// Validates the fields in the StructTag with the MaskRef.
template <typename StructTag>
bool validate_fields(MaskRef ref) {
  // Get the field ids in the thrift struct type.
  std::unordered_set<FieldId> ids;
  ids.reserve(op::size_v<StructTag>);
  op::for_each_ordinal<StructTag>([&](auto fieldOrdinalTag) {
    ids.insert(op::get_field_id<StructTag, decltype(fieldOrdinalTag)>());
  });
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
  bool isValid = true;
  op::for_each_ordinal<StructTag>([&](auto fieldOrdinalTag) {
    if (!isValid) { // short circuit
      return;
    }
    using OrdinalTag = decltype(fieldOrdinalTag);
    MaskRef next = ref.get(op::get_field_id<StructTag, OrdinalTag>());
    if (next.isAllMask() || next.isNoneMask()) {
      return;
    }
    // Check if the field is a thrift struct type. It uses native_type
    // as we don't support adapted struct fields in field mask.
    using FieldType = op::get_native_type<StructTag, OrdinalTag>;
    if constexpr (is_thrift_struct_v<FieldType>) {
      // Need to validate the struct type.
      isValid &= detail::validate_fields<type::struct_t<FieldType>>(next);
      return;
    }
    isValid = false;
  });
  return isValid;
}

// Ensures the masked fields in the given thrift struct.
template <typename T>
void ensure_fields(MaskRef ref, T& t) {
  op::for_each_ordinal<type::struct_t<T>>([&](auto fieldOrdinalTag) {
    using StructTag = type::struct_t<T>;
    using OrdinalTag = decltype(fieldOrdinalTag);
    MaskRef next = ref.get(op::get_field_id<StructTag, OrdinalTag>());
    if (next.isNoneMask()) {
      return;
    }
    auto& field = op::get<StructTag, OrdinalTag>(t).ensure();
    // Need to ensure the struct object.
    using FieldType = op::get_native_type<StructTag, OrdinalTag>;
    if constexpr (is_thrift_struct_v<FieldType>) {
      return ensure_fields(next, field);
    }
  });
}

// Clears the masked fields in the given thrift struct.
template <typename T>
void clear_fields(MaskRef ref, T& t) {
  op::for_each_ordinal<type::struct_t<T>>([&](auto fieldOrdinalTag) {
    using StructTag = type::struct_t<T>;
    using OrdinalTag = decltype(fieldOrdinalTag);
    MaskRef next = ref.get(op::get_field_id<StructTag, OrdinalTag>());
    if (next.isNoneMask()) {
      return;
    }
    // TODO(aoka): Support smart pointers and thrift box references.
    auto field_ref = op::get<StructTag, OrdinalTag>(t);
    if (next.isAllMask()) {
      op::clear_field<op::get_field_tag<StructTag, OrdinalTag>>(field_ref, t);
      return;
    }
    if constexpr (apache::thrift::detail::is_optional_field_ref<
                      decltype(field_ref)>::value) {
      if (!field_ref.has_value()) {
        return;
      }
    }
    // Need to clear the struct object.
    using FieldType = op::get_native_type<StructTag, OrdinalTag>;
    if constexpr (is_thrift_struct_v<FieldType>) {
      clear_fields(next, field_ref.value());
    }
  });
}

// Copies the masked fields from src thrift struct to dst.
// Returns true if it copied a field from src to dst.
template <typename T>
bool copy_fields(MaskRef ref, const T& src, T& dst) {
  bool copied = false;
  op::for_each_ordinal<type::struct_t<T>>([&](auto fieldOrdinalTag) {
    using StructTag = type::struct_t<T>;
    using OrdinalTag = decltype(fieldOrdinalTag);
    MaskRef next = ref.get(op::get_field_id<StructTag, OrdinalTag>());
    // Id doesn't exist in field mask, skip.
    if (next.isNoneMask()) {
      return;
    }
    // TODO(aoka): Support smart pointers and thrift box references.
    auto src_ref = op::get<StructTag, OrdinalTag>(src);
    auto dst_ref = op::get<StructTag, OrdinalTag>(dst);
    // Field ref has a value unless it is optional ref and not set.
    bool srcHasValue = true;
    bool dstHasValue = true;
    if constexpr (apache::thrift::detail::is_optional_field_ref<
                      decltype(src_ref)>::value) {
      srcHasValue = src_ref.has_value();
      dstHasValue = dst_ref.has_value();
    }
    if (!srcHasValue && !dstHasValue) { // skip
      return;
    }
    // Id that we want to copy.
    if (next.isAllMask()) {
      if (srcHasValue) {
        dst_ref.copy_from(src_ref);
        copied = true;
      } else {
        op::clear_field<op::get_field_tag<StructTag, OrdinalTag>>(dst_ref, dst);
      }
      return;
    }
    using FieldType = op::get_native_type<StructTag, OrdinalTag>;
    if constexpr (is_thrift_struct_v<FieldType>) {
      // Field doesn't exist in src, so just clear dst with the mask.
      if (!srcHasValue) {
        clear_fields(next, dst_ref.value());
        return;
      }
      // Field exists in both src and dst, so call copy recursively.
      if (dstHasValue) {
        copied |= copy_fields(next, src_ref.value(), dst_ref.value());
        return;
      }
      // Field only exists in src. Need to construct object only if there's
      // a field to add.
      FieldType newObject;
      bool constructObject = copy_fields(next, src_ref.value(), newObject);
      if (constructObject) {
        dst_ref = std::move(newObject);
        copied = true;
      }
    }
  });
  return copied;
}

// This converts ident list to a field mask with a single field.
template <typename T>
Mask path(const Mask& other) {
  // This is the base case as there is no more ident.
  return other;
}

template <typename T, typename Ident, typename... Idents>
Mask path(const Mask& other) {
  if constexpr (is_thrift_struct_v<T>) {
    Mask mask;
    using fieldId = op::get_field_id<type::struct_t<T>, Ident>;
    static_assert(fieldId::value != FieldId{});
    using FieldType = op::get_native_type<type::struct_t<T>, Ident>;
    mask.includes_ref().emplace()[static_cast<int16_t>(fieldId::value)] =
        path<FieldType, Idents...>(other);
    return mask;
  }
  throw std::runtime_error("field doesn't exist");
}
} // namespace apache::thrift::protocol::detail
