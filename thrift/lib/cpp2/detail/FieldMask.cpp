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

#include <thrift/lib/cpp2/detail/FieldMask.h>
#include <thrift/lib/thrift/gen-cpp2/field_mask_constants.h>

using apache::thrift::protocol::field_mask_constants;

namespace apache::thrift::protocol::detail {
// Gets the mask of the given field id if it exists in the map, otherwise,
// returns noneMask.
const Mask& getMask(const FieldIdToMask& map, int16_t fieldId) {
  return map.contains(fieldId) ? map.at(fieldId)
                               : field_mask_constants::noneMask();
}

MaskRef MaskRef::get(FieldId id) const {
  int16_t fieldId = static_cast<int16_t>(id);
  if (mask.includes_ref()) {
    return MaskRef{getMask(mask.includes_ref().value(), fieldId), is_exclusion};
  }
  return MaskRef{getMask(mask.excludes_ref().value(), fieldId), !is_exclusion};
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
