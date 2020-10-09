/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <string_view>

#include <folly/FBString.h>
#include <folly/Range.h>

namespace apache::thrift::conformance {

using type_id_size_t = int8_t;
inline constexpr type_id_size_t kDisableTypeId = 0;
inline constexpr type_id_size_t kMinTypeIdBytes = 16;
inline constexpr type_id_size_t kMaxTypeIdBytes = 32;

// Validates that name is a valid universal type name of the form:
// {domain}/{path}. For example: facebook.com/thrift/Value.
//
// Throws std::invalid_argument on failure.
void validateUniversalType(std::string_view name);

// Validates that the given type id meets the size requirements.
//
// Throws std::invalid_argument on failure.
void validateTypeId(folly::StringPiece typeId);

// Validates that the given type id bytes size meets size requirements.
//
// Throws std::invalid_argument on failure.
void validateTypeIdBytes(type_id_size_t typeIdBytes);

// Returns the type id for the given universal type name.
folly::fbstring getTypeId(std::string_view name);

// Shrinks the fullTypeId to fit in the given number of bytes.
folly::StringPiece getPartialTypeId(
    folly::StringPiece fullTypeId,
    type_id_size_t typeIdBytes);

// Returns true iff partialTypeId was derived from fullTypeId.
bool matchesTypeId(
    folly::StringPiece fullTypeId,
    folly::StringPiece partialTypeId);

// Clamps the given type id bytes to a valid range.
type_id_size_t clampTypeIdBytes(type_id_size_t typeIdBytes);

// Returns the type id iff smaller than the name.
folly::fbstring maybeGetTypeId(
    std::string_view name,
    type_id_size_t typeIdBytes);

// Returns true, if the given map contains an entry that matches the given
// partial type id.
template <typename C, typename K>
bool containsTypeId(C& sortedMap, const K& partialTypeId) {
  auto itr = sortedMap.lower_bound(partialTypeId);
  return itr != sortedMap.end() && matchesTypeId(itr->first, partialTypeId);
}

// Finds a matching id within the given sorted map.
//
// Raises a std::runtime_error if the result is ambigous.
template <typename C, typename K>
auto findByTypeId(C& sortedMap, const K& partialTypeId) {
  auto itr = sortedMap.lower_bound(partialTypeId);
  if (itr == sortedMap.end() || !matchesTypeId(itr->first, partialTypeId)) {
    return sortedMap.end();
  }
  auto next = itr;
  if (++next != sortedMap.end() && matchesTypeId(next->first, partialTypeId)) {
    folly::throw_exception<std::runtime_error>("type id look up ambigous");
  }
  return itr;
}

} // namespace apache::thrift::conformance
