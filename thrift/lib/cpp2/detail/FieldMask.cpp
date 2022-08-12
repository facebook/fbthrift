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

#include <folly/Utility.h>
#include <thrift/lib/cpp2/detail/FieldMask.h>

using apache::thrift::protocol::field_mask_constants;

namespace apache::thrift::protocol::detail {
// Gets the mask of the given field id if it exists in the map, otherwise,
// returns noneMask.
const Mask& getMask(const FieldIdToMask& map, FieldId id) {
  auto fieldId = folly::to_underlying(id);
  return map.contains(fieldId) ? map.at(fieldId)
                               : field_mask_constants::noneMask();
}

// Gets the mask of the given map id if it exists in the map, otherwise,
// returns noneMask.
const Mask& getMask(const MapIdToMask& map, MapId id) {
  auto mapId = folly::to_underlying(id);
  return map.contains(mapId) ? map.at(mapId) : field_mask_constants::noneMask();
}

void MaskRef::throwIfNotFieldMask() const {
  if (!isFieldMask()) {
    throw std::runtime_error("not a field mask");
  }
}

void MaskRef::throwIfNotMapMask() const {
  if (!isMapMask()) {
    throw std::runtime_error("not a map mask");
  }
}

MaskRef MaskRef::get(FieldId id) const {
  throwIfNotFieldMask();
  if (mask.includes_ref()) {
    return MaskRef{getMask(mask.includes_ref().value(), id), is_exclusion};
  }
  return MaskRef{getMask(mask.excludes_ref().value(), id), !is_exclusion};
}

MaskRef MaskRef::get(MapId id) const {
  throwIfNotMapMask();
  if (mask.includes_map_ref()) {
    return MaskRef{getMask(mask.includes_map_ref().value(), id), is_exclusion};
  }
  return MaskRef{getMask(mask.excludes_map_ref().value(), id), !is_exclusion};
}

bool MaskRef::isAllMask() const {
  return (is_exclusion && mask == field_mask_constants::noneMask()) ||
      (!is_exclusion && mask == field_mask_constants::allMask());
}

bool MaskRef::isNoneMask() const {
  return (is_exclusion && mask == field_mask_constants::allMask()) ||
      (!is_exclusion && mask == field_mask_constants::noneMask());
}

bool MaskRef::isExclusive() const {
  return (mask.includes_ref() && is_exclusion) ||
      (mask.excludes_ref() && !is_exclusion) ||
      (mask.includes_map_ref() && is_exclusion) ||
      (mask.excludes_map_ref() && !is_exclusion);
}

bool MaskRef::isFieldMask() const {
  return mask.includes_ref() || mask.excludes_ref();
}

bool MaskRef::isMapMask() const {
  return mask.includes_map_ref() || mask.excludes_map_ref();
}

int64_t getIntFromValue(Value v) {
  if (v.is_byte()) {
    return v.byteValue_ref().value();
  }
  if (v.is_i16()) {
    return v.i16Value_ref().value();
  }
  if (v.is_i32()) {
    return v.i32Value_ref().value();
  }
  if (v.is_i64()) {
    return v.i64Value_ref().value();
  }
  throw std::runtime_error("mask map only works with an integer key.");
}

void clear_impl(MaskRef ref, auto& obj, auto id, Value& value) {
  // Id doesn't exist in field mask, skip.
  if (ref.isNoneMask()) {
    return;
  }
  // Id that we want to clear.
  if (ref.isAllMask()) {
    obj.erase(id);
    return;
  }
  // clear fields in object recursively
  if (value.objectValue_ref()) {
    ref.clear(value.objectValue_ref().value());
    return;
  }
  // clear fields in map recursively
  if (value.mapValue_ref()) {
    ref.clear(value.mapValue_ref().value());
    return;
  }
  throw std::runtime_error("The field mask and object are incompatible.");
}

void MaskRef::clear(protocol::Object& obj) const {
  throwIfNotFieldMask();
  for (auto& [id, value] : obj) {
    MaskRef ref = get(FieldId{id});
    clear_impl(ref, obj, FieldId{id}, value);
  }
}

void MaskRef::clear(std::map<Value, Value>& map) const {
  throwIfNotMapMask();
  for (auto& [key, value] : map) {
    MaskRef ref = get(MapId{getIntFromValue(key)});
    clear_impl(ref, map, key, value);
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

void throwIfContainsMapMask(const Mask& mask) {
  if (mask.includes_map_ref() || mask.excludes_map_ref()) {
    throw std::runtime_error("map mask is not implemented");
  }
  const FieldIdToMask& map = mask.includes_ref() ? mask.includes_ref().value()
                                                 : mask.excludes_ref().value();
  for (auto& [_, nestedMask] : map) {
    throwIfContainsMapMask(nestedMask);
  }
}

} // namespace apache::thrift::protocol::detail
