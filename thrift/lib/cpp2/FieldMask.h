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
  // Returns true if there is a field copied from src to dst (not removed).
  // Throws a runtime exception if the mask and objects are incompatible.
  bool copy(const protocol::Object& src, protocol::Object& dst) const;

 private:
  // Gets all fields that need to be copied from src to dst.
  // Only contains fields either in src or dst.
  std::unordered_set<FieldId> getFieldsToCopy(
      const protocol::Object& src, const protocol::Object& dst) const;
};

// Throws an error if the given value is not a dynamic object value.
void errorIfNotObject(const protocol::Value& value);

} // namespace detail
} // namespace apache::thrift::protocol
