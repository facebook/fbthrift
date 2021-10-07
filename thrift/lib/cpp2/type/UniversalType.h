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

#include <thrift/lib/thrift/gen-cpp2/type_constants.h>
#include <thrift/lib/thrift/gen-cpp2/type_types.h>

namespace apache::thrift::type {
using type_hash_size_t = int8_t;
inline constexpr type_hash_size_t kDisableTypeHash = 0;
inline constexpr type_hash_size_t kMinTypeHashBytes =
    type_constants::minTypeHashBytes();
inline constexpr type_hash_size_t kDefaultTypeHashBytes =
    type_constants::defaultTypeHashBytes();

// Validates that uri is a valid universal type uri of the form:
// {domain}/{path}. For example: facebook.com/thrift/Value.
//
// The scheme "fbthrift://"" is implied and not included in the uri.
//
// Throws std::invalid_argument on failure.
void validateUniversalType(std::string_view uri);

// Validates that the given type hash meets the size requirements.
//
// Throws std::invalid_argument on failure.
void validateTypeHash(TypeHashAlgorithm alg, folly::StringPiece typeHash);

// Validates that the given type hash bytes size meets size requirements.
//
// Throws std::invalid_argument on failure.
void validateTypeHashBytes(type_hash_size_t typeHashBytes);

// The number of bytes returned by the given type hash algorithm.
type_hash_size_t getTypeHashSize(TypeHashAlgorithm alg);

// Returns the type hash for the given universal type uri.
//
// The hash includes the implied scheme, "fbthrift://".
folly::fbstring getTypeHash(TypeHashAlgorithm alg, std::string_view uri);

// Shrinks the typeHash to fit in the given number of bytes.
folly::StringPiece getTypeHashPrefix(
    folly::StringPiece typeHash,
    type_hash_size_t typeHashBytes = kDefaultTypeHashBytes);

// Returns the type hash prefix iff smaller than the uri.
folly::fbstring maybeGetTypeHashPrefix(
    TypeHashAlgorithm alg,
    std::string_view uri,
    type_hash_size_t typeHashBytes = kDefaultTypeHashBytes);

// Returns true iff prefix was derived from typeHash.
bool matchesTypeHash(folly::StringPiece typeHash, folly::StringPiece prefix);

// Returns true, if the given sorted map contains an entry that matches the
// given type hash prefix.
template <typename C, typename K>
bool containsTypeHash(C& sortedMap, const K& typeHashPrefix) {
  auto itr = sortedMap.lower_bound(typeHashPrefix);
  return itr != sortedMap.end() && matchesTypeHash(itr->first, typeHashPrefix);
}

// Finds a matching hash within the given sorted map.
//
// Raises a std::runtime_error if the result is ambigous.
template <typename C, typename K>
auto findByTypeHash(C& sortedMap, const K& typeHashPrefix) {
  auto itr = sortedMap.lower_bound(typeHashPrefix);
  if (itr == sortedMap.end() || !matchesTypeHash(itr->first, typeHashPrefix)) {
    return sortedMap.end();
  }
  auto next = itr;
  if (++next != sortedMap.end() &&
      matchesTypeHash(next->first, typeHashPrefix)) {
    folly::throw_exception<std::runtime_error>("type hash look up ambigous");
  }
  return itr;
}

} // namespace apache::thrift::type
