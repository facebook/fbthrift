/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cstdint>
#include <stdexcept>

#include <folly/GLog.h>
#include <folly/lang/Assume.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>

namespace apache {
namespace thrift {

namespace detail {
namespace nimble {

using protocol::TType;

constexpr int kFieldChunkHintBits = 2;
constexpr int kComplexMetadataBits = 3;
constexpr int kStructyTypeMetadataBits = 7;

/*
 * The third lowest bit in a complex type's field chunk is used to encode the
 * structy vs stringy of the complex type. A 0 value means the field is structy
 * (i.e. list/set/map/union/struct), whereas a 1 value means the field is
 * stringy (i.e. string or binary).
 */
enum ComplexType : std::uint32_t {
  STRUCTY = 0,
  STRINGY = 1,
};

/*
 * An enum representing the hint (lowest two bits) of the field stream chunks
 * (4-byte unit). COMPLEX_METADATA means the field is of stringy or structy
 * type. ONE_CHUNK_TYPE means the field is a 1-, 2-, or 4-byte integer,
 * bool, or float. TWO_CHUNKS_TYPE means the field is an 8-byte integer or
 * double. COMPLEX_TYPE means the field is a string/binary/collections or
 * struct.
 */
enum NimbleFieldChunkHint : std::uint32_t {
  COMPLEX_METADATA = 0,
  ONE_CHUNK_TYPE = 1,
  TWO_CHUNKS_TYPE = 2,
  COMPLEX_TYPE = 3,
};

/*
 * An enum indicating the type of the structy object.
 * LIST_OR_SET_* represent a list/set whose elements match the specified type.
 * MAP_* represent a map whose keys and values match the specified type.
 */
enum StructyType : std::uint32_t {
  NONE = 0,
  STRUCT = 1,
  UNION = 2,
  LIST_OR_SET_ONE_CHUNK = 3,
  LIST_OR_SET_TWO_CHUNKS = 4,
  LIST_OR_SET_COMPLEX = 5,
  MAP_ONE_TO_ONE_CHUNK = 6,
  MAP_ONE_TO_TWO_CHUNKS = 7,
  MAP_ONE_TO_COMPLEX = 8,
  MAP_TWO_TO_ONE_CHUNK = 9,
  MAP_TWO_TO_TWO_CHUNKS = 10,
  MAP_TWO_TO_COMPLEX = 11,
  MAP_COMPLEX_TO_ONE_CHUNK = 12,
  MAP_COMPLEX_TO_TWO_CHUNKS = 13,
  MAP_COMPLEX_TO_COMPLEX = 14,
};

/*
 * This method maps TType to NimbleFieldChunkHint enum.
 */
inline NimbleFieldChunkHint ttypeToNimbleFieldChunkHint(TType fieldType) {
  switch (fieldType) {
    case TType::T_BOOL:
    case TType::T_BYTE: // same enum value as TType::T_I08
    case TType::T_I16:
    case TType::T_I32:
    case TType::T_FLOAT:
      return NimbleFieldChunkHint::ONE_CHUNK_TYPE;
    case TType::T_U64:
    case TType::T_I64:
    case TType::T_DOUBLE:
      return NimbleFieldChunkHint::TWO_CHUNKS_TYPE;
    case TType::T_STRING: // same enum value as TType::T_UTF7
    case TType::T_LIST:
    case TType::T_SET:
    case TType::T_MAP:
    case TType::T_STRUCT:
      return NimbleFieldChunkHint::COMPLEX_TYPE;
    case TType::T_STOP:
      return NimbleFieldChunkHint::COMPLEX_METADATA;
    default:
      folly::assume_unreachable();
  }
}

/*
 * Return the StructyType enum of a map based on key/value's TTypes.
 */
inline StructyType getStructyTypeFromMap(TType keyType, TType valType) {
  const std::uint32_t kInvalid = -1;
  const static std::array<uint32_t, 16> structyTypeFromShift = {{
      // invalid key type
      kInvalid,
      kInvalid,
      kInvalid,
      kInvalid,
      // key type = 0b01, one-chunk:
      kInvalid,
      MAP_ONE_TO_ONE_CHUNK,
      MAP_ONE_TO_TWO_CHUNKS,
      MAP_ONE_TO_COMPLEX,
      // key type = 0b10, two-chunk:
      kInvalid,
      MAP_TWO_TO_ONE_CHUNK,
      MAP_TWO_TO_TWO_CHUNKS,
      MAP_TWO_TO_COMPLEX,
      // key type = 0b11, complex-chunk:
      kInvalid,
      MAP_COMPLEX_TO_ONE_CHUNK,
      MAP_COMPLEX_TO_TWO_CHUNKS,
      MAP_COMPLEX_TO_COMPLEX,
  }};
  auto keyHint = ttypeToNimbleFieldChunkHint(keyType);
  auto valHint = ttypeToNimbleFieldChunkHint(valType);
  DCHECK(keyHint < 4 && valHint < 4);

  int index = (keyHint << 2) | valHint;
  std::uint32_t result = structyTypeFromShift[index];
  DCHECK(result != kInvalid);
  return static_cast<StructyType>(result);
}

inline StructyType getStructyTypeFromListOrSet(TType elemType) {
  const std::uint32_t kInvalid = -1;
  auto elemFieldHint = ttypeToNimbleFieldChunkHint(elemType);

  const static std::array<uint32_t, 4> structyType = {{
      kInvalid,
      LIST_OR_SET_ONE_CHUNK,
      LIST_OR_SET_TWO_CHUNKS,
      LIST_OR_SET_COMPLEX,
  }};
  DCHECK(elemFieldHint < structyType.size());
  std::uint32_t result = structyType[elemFieldHint];
  DCHECK(result != kInvalid);
  return static_cast<StructyType>(result);
}
} // namespace nimble
} // namespace detail
} // namespace thrift
} // namespace apache
