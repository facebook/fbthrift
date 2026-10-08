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

#include <thrift/lib/cpp2/transcode/ProtobufIntrinsics.h>

#include <thrift/lib/cpp2/transcode/IntrinsicsCommon.h>

#include <limits>

using apache::thrift::transcode::setCursorError;

namespace {

constexpr int64_t kMalformedProtobuf = 1;
constexpr uint8_t PB_WIRE_VARINT = 0;
constexpr uint8_t PB_WIRE_64BIT = 1;
constexpr uint8_t PB_WIRE_LENGTH_DELIMITED = 2;
constexpr uint8_t PB_WIRE_32BIT = 5;
// Protobuf writes each map entry as a nested message carrying the key in field
// 1 and the value in field 2.
constexpr int16_t kMapEntryKeyField = 1;
constexpr int16_t kMapEntryValueField = 2;
// Room left for a record's length while its body is written. Five bytes hold
// any 32-bit length as a varint, and protobuf readers accept the padding when
// the length needs fewer.
constexpr size_t kReservedLengthBytes = 5;

bool isSupportedWireType(uint8_t wireType) {
  switch (wireType) {
    case PB_WIRE_VARINT:
    case PB_WIRE_64BIT:
    case PB_WIRE_LENGTH_DELIMITED:
    case PB_WIRE_32BIT:
      return true;
    default:
      return false;
  }
}

// Legacy negative Thrift field IDs map above INT16_MAX on the protobuf wire as
// `32767 - id`, so -1 is field number 32768 and -32768 is 65535.
constexpr int64_t kMaxPositiveFieldNumber = std::numeric_limits<int16_t>::max();
constexpr int64_t kMaxFieldNumber =
    kMaxPositiveFieldNumber - std::numeric_limits<int16_t>::min();

int16_t fieldIdForNumber(uint64_t fieldNumber) {
  const auto number = static_cast<int64_t>(fieldNumber);
  return static_cast<int16_t>(
      number <= kMaxPositiveFieldNumber ? number
                                        : kMaxPositiveFieldNumber - number);
}

uint64_t numberForFieldId(int16_t fieldId) {
  return static_cast<uint64_t>(
      fieldId > 0 ? fieldId : kMaxPositiveFieldNumber - fieldId);
}

} // namespace

extern "C" {

uint8_t thrift_transcode_proto_read_field_header(
    TranscodeCursor* cursor, int16_t* fieldId, int16_t /*prevFieldId*/) {
  if (cursor == nullptr) {
    return 0;
  }
  if (fieldId == nullptr) {
    setCursorError(cursor, kMalformedProtobuf);
    return 0;
  }
  if (cursor->error != 0) {
    *fieldId = 0;
    return 0;
  }
  if (cursor->readPos >= cursor->readEnd) {
    *fieldId = 0;
    return 0; // end of message
  }
  uint64_t tag = thrift_transcode_read_unsigned_varint(cursor);
  if (cursor->error != 0) {
    *fieldId = 0;
    return 0;
  }
  const uint8_t wireType = static_cast<uint8_t>(tag & 0x07);
  const uint64_t fieldNumber = tag >> 3;
  if (fieldNumber == 0 ||
      fieldNumber > static_cast<uint64_t>(kMaxFieldNumber) ||
      !isSupportedWireType(wireType)) {
    *fieldId = 0;
    setCursorError(cursor, kMalformedProtobuf);
    return 0;
  }
  *fieldId = fieldIdForNumber(fieldNumber);
  // Offset wire type by 1 so 0 is reserved for "end of message".
  // The codegen only checks for 0 = stop, the actual wire type value is not
  // used for dispatch (field ID is used instead).
  return static_cast<uint8_t>(wireType + 1);
}

void thrift_transcode_proto_write_field_header(
    TranscodeCursor* cursor,
    uint8_t typeInfo,
    int16_t fieldId,
    int16_t /*prevFieldId*/) {
  if (cursor == nullptr) {
    return;
  }
  if (cursor->error != 0) {
    return;
  }
  if (typeInfo == 0 || fieldId == 0) {
    setCursorError(cursor, kMalformedProtobuf);
    return;
  }
  // typeInfo is wire_type + 1 (0 reserved for stop). Subtract 1 to get real
  // wire type for the tag.
  uint8_t wireType = typeInfo - 1;
  if (!isSupportedWireType(wireType)) {
    setCursorError(cursor, kMalformedProtobuf);
    return;
  }
  thrift_transcode_write_unsigned_varint(
      cursor, (numberForFieldId(fieldId) << 3) | wireType);
}

void thrift_transcode_proto_write_stop(TranscodeCursor* /*cursor*/) {
  // Protobuf messages don't have stop markers — no-op
}

void thrift_transcode_proto_skip_field(
    TranscodeCursor* cursor, uint8_t typeInfo) {
  if (cursor == nullptr) {
    return;
  }
  if (cursor->error != 0) {
    return;
  }
  if (typeInfo == 0) {
    setCursorError(cursor, kMalformedProtobuf);
    return;
  }
  // typeInfo is offset by 1 (0 = stop). Subtract 1 to get real wire type.
  uint8_t wireType = typeInfo - 1;
  switch (wireType) {
    case PB_WIRE_VARINT:
      thrift_transcode_read_unsigned_varint(cursor);
      break;
    case PB_WIRE_64BIT:
      thrift_transcode_read_fixed64_le_checked(cursor);
      break;
    case PB_WIRE_LENGTH_DELIMITED: {
      size_t len = 0;
      thrift_transcode_read_varint_prefixed(cursor, &len);
      break;
    }
    case PB_WIRE_32BIT:
      thrift_transcode_read_fixed32_le_checked(cursor);
      break;
    default:
      setCursorError(cursor, kMalformedProtobuf);
      break;
  }
}

TranscodePatchPoint thrift_transcode_proto_reserve_length(
    TranscodeCursor* cursor) {
  TranscodePatchPoint mark = thrift_transcode_cursor_mark(cursor);
  thrift_transcode_cursor_skip(cursor, kReservedLengthBytes);
  return mark;
}

void thrift_transcode_proto_patch_length(
    TranscodeCursor* cursor, TranscodePatchPoint mark) {
  if (cursor == nullptr || cursor->error != 0) {
    return;
  }
  const size_t bodyBytes =
      thrift_transcode_cursor_bytes_since_mark(cursor, mark) -
      kReservedLengthBytes;
  thrift_transcode_cursor_patch_varint(
      cursor, mark, bodyBytes, kReservedLengthBytes);
}

bool thrift_transcode_proto_read_next_occurrence(
    TranscodeCursor* cursor, int16_t fieldId, uint8_t typeInfo) {
  if (cursor == nullptr || cursor->error != 0) {
    return false;
  }
  const uint8_t* next = cursor->readPos;
  int16_t nextFieldId = 0;
  const uint8_t nextTypeInfo =
      thrift_transcode_proto_read_field_header(cursor, &nextFieldId, 0);
  if (cursor->error != 0) {
    return false;
  }
  if (nextTypeInfo == 0 || nextFieldId != fieldId) {
    cursor->readPos = next;
    return false;
  }
  if (nextTypeInfo != typeInfo) {
    setCursorError(cursor, kMalformedProtobuf);
    return false;
  }
  return true;
}

TranscodePatchPoint thrift_transcode_proto_begin_map_entry(
    TranscodeCursor* cursor, int16_t fieldId, uint8_t keyWireType) {
  thrift_transcode_proto_write_field_header(
      cursor, PB_WIRE_LENGTH_DELIMITED + 1, fieldId, 0);
  TranscodePatchPoint mark = thrift_transcode_proto_reserve_length(cursor);
  thrift_transcode_proto_write_field_header(
      cursor, static_cast<uint8_t>(keyWireType + 1), kMapEntryKeyField, 0);
  return mark;
}

void thrift_transcode_proto_write_map_value_header(
    TranscodeCursor* cursor, uint8_t valueWireType) {
  thrift_transcode_proto_write_field_header(
      cursor, static_cast<uint8_t>(valueWireType + 1), kMapEntryValueField, 0);
}

void thrift_transcode_proto_end_map_entry(
    TranscodeCursor* cursor, TranscodePatchPoint mark) {
  thrift_transcode_proto_patch_length(cursor, mark);
}

bool thrift_transcode_proto_enter_map_entry(
    TranscodeCursor* cursor,
    uint8_t keyWireType,
    uint8_t valueWireType,
    TranscodeProtoMapEntry* entry) {
  if (cursor == nullptr || cursor->error != 0) {
    return false;
  }
  if (entry == nullptr) {
    setCursorError(cursor, kMalformedProtobuf);
    return false;
  }
  *entry = {};
  const uint64_t len = thrift_transcode_read_unsigned_varint(cursor);
  if (cursor->error != 0) {
    return false;
  }
  if (cursor->readPos > cursor->readEnd ||
      len > static_cast<uint64_t>(cursor->readEnd - cursor->readPos)) {
    setCursorError(cursor, kMalformedProtobuf);
    return false;
  }
  entry->parentReadEnd = cursor->readEnd;
  entry->end = cursor->readPos + len;
  cursor->readEnd = entry->end;
  int16_t fieldId = 0;
  while (const uint8_t typeInfo =
             thrift_transcode_proto_read_field_header(cursor, &fieldId, 0)) {
    if (fieldId == kMapEntryKeyField) {
      if (typeInfo != keyWireType + 1) {
        setCursorError(cursor, kMalformedProtobuf);
        break;
      }
      entry->key = cursor->readPos;
    } else if (fieldId == kMapEntryValueField) {
      if (typeInfo != valueWireType + 1) {
        setCursorError(cursor, kMalformedProtobuf);
        break;
      }
      entry->value = cursor->readPos;
    }
    thrift_transcode_proto_skip_field(cursor, typeInfo);
  }
  if (cursor->error != 0) {
    cursor->readEnd = entry->parentReadEnd;
    return false;
  }
  return true;
}

void thrift_transcode_proto_leave_map_entry(
    TranscodeCursor* cursor, const TranscodeProtoMapEntry* entry) {
  if (cursor == nullptr || entry == nullptr) {
    return;
  }
  cursor->readPos = entry->end;
  cursor->readEnd = entry->parentReadEnd;
}

} // extern "C"
