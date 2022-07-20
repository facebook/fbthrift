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

#include <thrift/lib/cpp2/FieldMask.h>

using apache::thrift::protocol::FieldIdToMask;

namespace apache::thrift::protocol {

void clear(const Mask& mask, protocol::Object& obj) {
  (detail::MaskRef{mask, false}).clear(obj);
}

void copy(
    const Mask& mask, const protocol::Object& src, protocol::Object& dst) {
  (detail::MaskRef{mask, false}).copy(src, dst);
}

void insertIfNotNoneMask(
    FieldIdToMask& map, int16_t fieldId, const Mask& mask) {
  // This doesn't use isNoneMask() as we just want to remove includes{} mask
  // rather than masks that logically contain no fields.
  if (mask != noneMask()) {
    map[fieldId] = mask;
  }
}

// Implementation of FieldMask's Logical Operators
// -----------------------------------------------
// To implement the logical operators for Field Mask while keeping the code
// manageable is to assume the Field Mask is not complement first, and apply
// boolean algebra such as De Morgan's laws to handle all cases.
// Logical operators for FieldMask easily by applying laws of boolean algebra.
// If the result is a complement set, we can set that to excludes mask.
// P and Q are the set of fields in the mask for lhs and rhs.

// Intersect
// - lhs = includes, rhs = includes : P·Q
// - lhs = includes, rhs = excludes : P·~Q​=P-Q
// - lhs = excludes, rhs = includes: ~P·Q=Q-P
// - lhs = excludes, rhs = excludes: ~P·~Q​=~(P+Q)​
// Union
// - lhs = includes, rhs = includes : P+Q
// - lhs = includes, rhs = excludes : P+~Q​=~(~P·Q)​=~(Q-P)​
// - lhs = excludes, rhs = includes: ~P+Q=~(P·~Q)​​=~(P-Q)​
// - lhs = excludes, rhs = excludes: ~P+~Q​=~(P·Q)​
// Subtract
// - lhs = includes, rhs = includes : P-Q
// - lhs = includes, rhs = excludes : P-~Q​=P·Q
// - lhs = excludes, rhs = includes: ~P-Q=~P·~Q​=~(P+Q)​
// - lhs = excludes, rhs = excludes: ~P-~Q​=(~P·Q)=Q/P

// Returns the intersection, union, or subtraction of the given FieldIdToMasks.
// This basically treats the maps as inclusive sets.
FieldIdToMask intersectMask(
    const FieldIdToMask& lhs, const FieldIdToMask& rhs) {
  FieldIdToMask map;
  for (auto& [fieldId, lhsMask] : lhs) {
    if (!rhs.contains(fieldId)) { // Only lhs contains the field.
      continue;
    }
    // Both maps have the field, so the mask is their intersection.
    insertIfNotNoneMask(map, fieldId, lhsMask & rhs.at(fieldId));
  }
  return map;
}
FieldIdToMask unionMask(const FieldIdToMask& lhs, const FieldIdToMask& rhs) {
  FieldIdToMask map;
  for (auto& [fieldId, lhsMask] : lhs) {
    if (!rhs.contains(fieldId)) { // Only lhs contains the field.
      insertIfNotNoneMask(map, fieldId, lhsMask);
      continue;
    }
    // Both maps have the field, so the mask is their union.
    insertIfNotNoneMask(map, fieldId, lhsMask | rhs.at(fieldId));
  }
  for (auto& [fieldId, rhsMask] : rhs) {
    if (!lhs.contains(fieldId)) { // Only rhs contains the field.
      insertIfNotNoneMask(map, fieldId, rhsMask);
    }
  }
  return map;
}
FieldIdToMask subtractMask(const FieldIdToMask& lhs, const FieldIdToMask& rhs) {
  FieldIdToMask map;
  for (auto& [fieldId, lhsMask] : lhs) {
    if (!rhs.contains(fieldId)) { // Only lhs contains the field.
      insertIfNotNoneMask(map, fieldId, lhsMask);
      continue;
    }
    // Both maps have the field, so the mask is their subtraction.
    insertIfNotNoneMask(map, fieldId, lhsMask - rhs.at(fieldId));
  }
  return map;
}

Mask createIncludesMask(FieldIdToMask&& map) {
  Mask mask;
  mask.includes_ref() = map;
  return mask;
}
Mask createExcludesMask(FieldIdToMask&& map) {
  Mask mask;
  mask.excludes_ref() = map;
  return mask;
}

Mask operator&(const Mask& lhs, const Mask& rhs) {
  if (lhs.includes_ref()) {
    if (rhs.includes_ref()) { // lhs=includes rhs=includes
      return createIncludesMask(intersectMask(
          lhs.includes_ref().value(), rhs.includes_ref().value()));
    }
    // lhs=includes rhs=excludes
    return createIncludesMask(
        subtractMask(lhs.includes_ref().value(), rhs.excludes_ref().value()));
  }
  if (rhs.includes_ref()) { // lhs=excludes rhs=includes
    return createIncludesMask(
        subtractMask(rhs.includes_ref().value(), lhs.excludes_ref().value()));
  }
  // lhs=excludes rhs=excludes
  return createExcludesMask(
      unionMask(lhs.excludes_ref().value(), rhs.excludes_ref().value()));
}
Mask operator|(const Mask& lhs, const Mask& rhs) {
  if (lhs.includes_ref()) {
    if (rhs.includes_ref()) { // lhs=includes rhs=includes
      return createIncludesMask(
          unionMask(lhs.includes_ref().value(), rhs.includes_ref().value()));
    }
    // lhs=includes rhs=excludes
    return createExcludesMask(
        subtractMask(rhs.excludes_ref().value(), lhs.includes_ref().value()));
  }
  if (rhs.includes_ref()) { // lhs=excludes rhs=includes
    return createExcludesMask(
        subtractMask(lhs.excludes_ref().value(), rhs.includes_ref().value()));
  }
  // lhs=excludes rhs=excludes
  return createExcludesMask(
      intersectMask(lhs.excludes_ref().value(), rhs.excludes_ref().value()));
}
Mask operator-(const Mask& lhs, const Mask& rhs) {
  if (lhs.includes_ref()) {
    if (rhs.includes_ref()) { // lhs=includes rhs=includes
      return createIncludesMask(
          subtractMask(lhs.includes_ref().value(), rhs.includes_ref().value()));
    }
    // lhs=includes rhs=excludes
    return createIncludesMask(
        intersectMask(lhs.includes_ref().value(), rhs.excludes_ref().value()));
  }
  if (rhs.includes_ref()) { // lhs=excludes rhs=includes
    return createExcludesMask(
        unionMask(lhs.excludes_ref().value(), rhs.includes_ref().value()));
  }
  // lhs=excludes rhs=excludes
  return createIncludesMask(
      subtractMask(rhs.excludes_ref().value(), lhs.excludes_ref().value()));
}

namespace detail {

// Gets the mask of the given field id if it exists in the map, otherwise,
// returns noneMask.
const Mask& getMask(const FieldIdToMask& map, int16_t fieldId) {
  return map.contains(fieldId) ? map.at(fieldId) : noneMask();
}

MaskRef MaskRef::get(FieldId id) const {
  int16_t fieldId = static_cast<int16_t>(id);
  if (mask.includes_ref()) {
    return MaskRef{getMask(mask.includes_ref().value(), fieldId), is_exclusion};
  }
  return MaskRef{getMask(mask.excludes_ref().value(), fieldId), !is_exclusion};
}

bool MaskRef::isAllMask() const {
  return (is_exclusion && mask == noneMask()) ||
      (!is_exclusion && mask == allMask());
}

bool MaskRef::isNoneMask() const {
  return (is_exclusion && mask == allMask()) ||
      (!is_exclusion && mask == noneMask());
}

bool MaskRef::isExclusive() const {
  return (mask.includes_ref() && is_exclusion) ||
      (mask.excludes_ref() && !is_exclusion);
}

void MaskRef::clear(protocol::Object& obj) const {
  for (auto& [id, value] : obj) {
    MaskRef ref = get(FieldId{id});
    // Id doesn't exist in field mask, skip.
    if (ref.isNoneMask()) {
      continue;
    }
    // Id that we want to clear.
    if (ref.isAllMask()) {
      obj.erase(FieldId(id));
      continue;
    }
    errorIfNotObject(value);
    ref.clear(value.objectValue_ref().value());
  }
}

void MaskRef::copy(const protocol::Object& src, protocol::Object& dst) const {
  // Get all fields that are possibly masked (either in src or dst).
  for (FieldId fieldId : getFieldsToCopy(src, dst)) {
    MaskRef ref = get(fieldId);
    // Id doesn't exist in field mask, skip.
    if (ref.isNoneMask()) {
      continue;
    }
    // Id that we want to copy.
    if (ref.isAllMask()) {
      if (src.contains(fieldId)) {
        dst[fieldId] = src.at(fieldId);
      } else {
        dst.erase(fieldId);
      }
      continue;
    }
    // Field doesn't exist in src, so just clear dst with the mask.
    if (!src.contains(fieldId)) {
      errorIfNotObject(dst.at(fieldId));
      ref.clear(dst.at(fieldId).objectValue_ref().value());
      continue;
    }
    // Field exists in both src and dst, so call copy recursively.
    errorIfNotObject(src.at(fieldId));
    if (dst.contains(fieldId)) {
      errorIfNotObject(dst.at(fieldId));
      ref.copy(
          src.at(fieldId).objectValue_ref().value(),
          dst.at(fieldId).objectValue_ref().value());
      continue;
    }
    // Field only exists in src. Need to construct object only if there's
    // a field to add.
    protocol::Object newObject;
    ref.copy(src.at(fieldId).objectValue_ref().value(), newObject);
    if (!newObject.empty()) {
      dst[fieldId].emplace_object() = std::move(newObject);
    }
  }
}

std::unordered_set<FieldId> MaskRef::getFieldsToCopy(
    const protocol::Object& src, const protocol::Object& dst) const {
  std::unordered_set<FieldId> fieldIds;
  if (isExclusive()) {
    // With exclusive mask, copies fields in either src or dst.
    fieldIds.reserve(src.size() + dst.size());
    for (auto& [id, _] : src) {
      fieldIds.insert(FieldId{id});
    }
    for (auto& [id, _] : dst) {
      fieldIds.insert(FieldId{id});
    }
    return fieldIds;
  }

  // With inclusive mask, just copies fields in the mask.
  const FieldIdToMask& map =
      is_exclusion ? mask.excludes_ref().value() : mask.includes_ref().value();
  fieldIds.reserve(map.size());
  for (auto& [fieldId, _] : map) {
    if (src.contains(FieldId{fieldId}) || dst.contains(FieldId{fieldId})) {
      fieldIds.insert(FieldId{fieldId});
    }
  }
  return fieldIds;
}

void errorIfNotObject(const protocol::Value& value) {
  if (!value.objectValue_ref()) {
    throw std::runtime_error("The field mask and object are incompatible.");
  }
}
} // namespace detail
} // namespace apache::thrift::protocol
