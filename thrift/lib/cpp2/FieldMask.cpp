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

#include <thrift/lib/cpp/Field.h>
#include <thrift/lib/cpp2/FieldMask.h>
#include <thrift/lib/thrift/gen-cpp2/protocol_constants.h>

using apache::thrift::protocol::FieldIdToMask;

namespace apache::thrift::protocol {

void clear(const Mask& mask, protocol::Object& obj) {
  (detail::MaskRef{mask, false}).clear(obj);
}

namespace detail {

// Gets the mask of the given field id if it exists in the map, otherwise,
// returns noneMask.
const Mask& getMask(const FieldIdToMask& map, int16_t fieldId) {
  return map.contains(fieldId) ? map.at(fieldId)
                               : protocol_constants::noneMask();
}

MaskRef MaskRef::get(FieldId id) const {
  int16_t fieldId = static_cast<int16_t>(id);
  if (mask.inclusive_ref()) {
    return MaskRef{
        getMask(mask.inclusive_ref().value(), fieldId), is_exclusion};
  }
  return MaskRef{getMask(mask.exclusive_ref().value(), fieldId), !is_exclusion};
}

bool MaskRef::isAllMask() const {
  return (is_exclusion && mask == protocol_constants::noneMask()) ||
      (!is_exclusion && mask == protocol_constants::allMask());
}

bool MaskRef::isNoneMask() const {
  return (is_exclusion && mask == protocol_constants::allMask()) ||
      (!is_exclusion && mask == protocol_constants::noneMask());
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
    // If it is not a dynamic object value, throw an error.
    if (!value.objectValue_ref()) {
      throw std::runtime_error("The field mask and object are incompatible.");
    }
    ref.clear(value.objectValue_ref().value());
  }
}
} // namespace detail
} // namespace apache::thrift::protocol
