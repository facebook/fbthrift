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

#include <cstdint>
#include <stdexcept>

#include <folly/GLog.h>
#include <folly/Utility.h>
#include <folly/lang/Assume.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>

namespace apache {
namespace thrift {

namespace detail {
namespace nimble {

using protocol::TType;

// FIELD METADATA AND TYPE ENCODING
// There are two encodings we use to encode (field-id, nimble-type) pairs.
//
// Note:
// Below, we'll talk about "adjusted field ids". The adjustment is just
// subtracting 1, so that we don't waste an encoding on a field ID of 0.
//
// COMPACT ENCODING:
// If the adjusted field id is < 32, then we use a single byte to encode it. The
// low 3 bits store the type (this avoids ever emitting a 0 byte, which we
// can then use as the "end of struct" marker), and the high 5 bits encode the
// adjusted field ID.
//
// LARGE-FIELD-ID ENCODING:
// Otherwise, we'll use a multiple bytes to encode it. The low 3 bits of the
// first byte will be 1 (which would correspond to an encoded type of INVALID),
// and the high 3 bits will encode the field type.
//
// If the adjusted field ID fits in 1 byte, we set the fourth lowest bit to 0
// and set the next byte to the adjusted field ID. Otherwise, we set the fourth
// lowest bit to 1 and set the next 2 bytes to the adjusted field ID, in
// little-endian order.
//
// CONTAINERS
// Container types get 1 extra byte, to indicate element types. For lists, the
// element type lives in the low 3 bits. For maps, the key type lives in the
// low 3 bits and the value type lives in the high 3 bits.

// The NimbleType is all we see on the wire. (We include explicit enum values to
// emphasize that this is wire-visible; it's not safe to change order without
// keeping the value).
enum class NimbleType {
  // An end-of-struct "STOP" field.
  STOP = 0,
  // Primitive types; those that cannot contain any other types. In particular,
  // primitive types can use the 1-byte-field-metadata encoding scheme.
  ONE_CHUNK = 1,
  TWO_CHUNK = 2,
  STRING = 3,
  // Complex types; those that *can* contain other types (and therefore a
  // variable-length number of field stream bytes).
  STRUCT = 4,
  LIST = 5,
  MAP = 6,
  // We sometimes obtain a NimbleType by masking off 3 bits. The value 7,
  // though, does not correspond to any type. In field ID encoding, it should
  // indicate a multi-byte encoding. In (e.g.) map or list encoding, it's just
  // invalid data.
  INVALID = 7,
};

// For byte the first byte of some field ID, determine whether or not it uses
// the compact 1-byte encoding;
inline bool isCompactMetadata(std::uint8_t byte) {
  return (byte & 7) != 7;
}

inline NimbleType nimbleTypeFromCompactMetadata(std::uint8_t byte) {
  DCHECK(isCompactMetadata(byte));
  return (NimbleType)(byte & 7);
}

inline std::int16_t fieldIdFromCompactMetadata(std::uint8_t byte) {
  DCHECK(isCompactMetadata(byte));
  return (byte >> 3) + 1;
}

inline NimbleType nimbleTypeFromLargeMetadata(std::uint8_t byte1) {
  DCHECK(!isCompactMetadata(byte1));
  return (NimbleType)(byte1 >> 5);
}

inline bool isLargeMetadataTwoByte(std::uint8_t byte1) {
  DCHECK(!isCompactMetadata(byte1));
  return (byte1 & (1 << 3)) == 0;
}

inline std::uint16_t fieldIdFromTwoByteMetadata(
    std::uint8_t byte1,
    std::uint8_t byte2) {
  DCHECK(isLargeMetadataTwoByte(byte1));
  return byte2 + 1;
}

inline std::uint16_t fieldIdFromThreeByteMetadata(
    std::uint8_t byte1,
    std::uint16_t short_) {
  DCHECK(!isLargeMetadataTwoByte(byte1));
  return short_ + 1;
}

inline NimbleType ttypeToNimbleType(TType ttype) {
  // Don't worry about the performance of this switch; this function is only
  // ever called when its type is a compile-time constant.
  switch (ttype) {
    case TType::T_STOP:
      return NimbleType::STOP;
    case TType::T_BOOL:
      return NimbleType::ONE_CHUNK;
    case TType::T_BYTE: // == TType::T_I08
      return NimbleType::ONE_CHUNK;
    case TType::T_DOUBLE:
      return NimbleType::TWO_CHUNK;
    case TType::T_I16:
      return NimbleType::ONE_CHUNK;
    case TType::T_I32:
      return NimbleType::ONE_CHUNK;
    case TType::T_U64:
      return NimbleType::TWO_CHUNK;
    case TType::T_I64:
      return NimbleType::TWO_CHUNK;
    case TType::T_STRING:
      return NimbleType::STRING;
    case TType::T_STRUCT:
      return NimbleType::STRUCT;
    case TType::T_MAP:
      return NimbleType::MAP;
    case TType::T_SET:
      return NimbleType::LIST;
    case TType::T_LIST:
      return NimbleType::LIST;
    case TType::T_UTF8:
      return NimbleType::STRING;
    case TType::T_UTF16:
      return NimbleType::STRING;
    case TType::T_FLOAT:
      return NimbleType::ONE_CHUNK;
    default:
      // A TType never comes in off the wire (it couldn't; we encode Nimble
      // types on the wire).
      folly::assume_unreachable();
  }
}

struct FieldBytes {
  FieldBytes() : len(0), bytes{0, 0, 0} {}
  std::size_t len;
  std::uint8_t bytes[3];
};

inline FieldBytes stopBytes() {
  FieldBytes result;
  result.len = 1;
  result.bytes[0] = 0;
  return result;
}

inline FieldBytes mapBeginByte(NimbleType key, NimbleType value) {
  FieldBytes result;
  result.len = 1;
  result.bytes[0] = (int)key | ((int)value << 5);
  return result;
}

// We take a reference (instead of returning the type directly) to match the map
// and protocol interface equivalents.
inline void listTypeFromByte(std::uint8_t byte, NimbleType& elem) {
  elem = (NimbleType)(byte & 7);
}

inline void
mapTypesFromByte(std::uint8_t byte, NimbleType& key, NimbleType& val) {
  key = (NimbleType)(byte & 7);
  val = (NimbleType)(byte >> 5);
}

inline FieldBytes listBeginByte(NimbleType elem) {
  FieldBytes result;
  result.len = 1;
  result.bytes[0] = (int)elem;
  return result;
}

// This is always called with static values, in readNoXfer. It compiles out;
// there's no code-size risk in inlining it.
FOLLY_ALWAYS_INLINE
FieldBytes fieldBeginBytes(NimbleType type, std::uint16_t fieldId) {
  // This is only called with trusted values, never a type off the wire; that
  // type should always be valid.
  DCHECK(type != NimbleType::INVALID);

  FieldBytes result;

  if (type == NimbleType::STOP) {
    result.len = 1;
    result.bytes[0] = 0;
    return result;
  }

  std::uint16_t adjustedFieldId = fieldId - 1;
  if (adjustedFieldId < 32) {
    result.len = 1;
    result.bytes[0] = (adjustedFieldId << 3) | (int)type;
  } else {
    std::uint8_t lengthBit;
    if (adjustedFieldId < 256) {
      lengthBit = 0;
      result.len = 2;
    } else {
      lengthBit = (1 << 3);
      result.len = 3;
    }
    std::uint8_t lowTypeBits = (int)NimbleType::INVALID;
    std::uint8_t highTypeBits = (int)type << 5;
    result.bytes[0] = lowTypeBits | highTypeBits | lengthBit;
    result.bytes[1] = adjustedFieldId & 0xFF;
    result.bytes[2] = adjustedFieldId >> 8;
  };
  return result;
}

} // namespace nimble
} // namespace detail
} // namespace thrift
} // namespace apache
