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

#include <cstdint>

#include <thrift/lib/cpp2/transcode/Cursor.h>

// Protobuf field framing intrinsics, exposed with the uniform field-header
// signature the codegen uses for every protocol. Field IDs are Thrift field
// IDs: positive IDs are their own protobuf field numbers, and negative IDs
// travel as `32767 - id` (32768 through 65535).

// The key and value thrift_transcode_proto_enter_map_entry finds in an entry.
struct TranscodeProtoMapEntry {
  // Where each part's payload starts, or null when the entry leaves it out.
  const uint8_t* key;
  const uint8_t* value;
  const uint8_t* end;
  const uint8_t* parentReadEnd;
};

extern "C" {

// Reads a protobuf tag and adapts to uniform signature:
//   Returns wire_type + 1 as the "type info" byte (offset by 1 so 0 stays
//   reserved for end of message). Sets *fieldId.
//   Returns 0 when cursor is at end of message.
uint8_t thrift_transcode_proto_read_field_header(
    TranscodeCursor* cursor, int16_t* fieldId, int16_t prevFieldId);

// Writes a protobuf tag from uniform signature:
//   typeInfo = wire_type + 1 (the readers' offset form; 1 is subtracted to
//   recover the wire type). prevFieldId is ignored.
void thrift_transcode_proto_write_field_header(
    TranscodeCursor* cursor,
    uint8_t typeInfo,
    int16_t fieldId,
    int16_t prevFieldId);

// No-op for protobuf (messages don't have stop markers).
void thrift_transcode_proto_write_stop(TranscodeCursor* cursor);

// Skips a protobuf field payload using the same typeInfo byte returned by
// thrift_transcode_proto_read_field_header: wire_type + 1, with 0 reserved for
// end of message. Callers should not pass the raw protobuf wire type.
void thrift_transcode_proto_skip_field(
    TranscodeCursor* cursor, uint8_t typeInfo);

// Reserves room for the length of the record written next. The length is only
// known once the body is written, so thrift_transcode_proto_patch_length fills
// it in as a five-byte varint, which protobuf readers accept.
TranscodePatchPoint thrift_transcode_proto_reserve_length(
    TranscodeCursor* cursor);
void thrift_transcode_proto_patch_length(
    TranscodeCursor* cursor, TranscodePatchPoint mark);

// Consumes the next field header only when it is another occurrence of
// `fieldId`, leaving any other field unread. The occurrence must have the
// same typeInfo as the first.
bool thrift_transcode_proto_read_next_occurrence(
    TranscodeCursor* cursor, int16_t fieldId, uint8_t typeInfo);

// A map entry is a length-delimited occurrence of the map's field holding the
// key in field 1 and the value in field 2. Writing one: begin writes the
// entry's header and the key's, the caller writes the key, the value header,
// then the value, and end fills in the entry's length.
TranscodePatchPoint thrift_transcode_proto_begin_map_entry(
    TranscodeCursor* cursor, int16_t fieldId, uint8_t keyWireType);
void thrift_transcode_proto_write_map_value_header(
    TranscodeCursor* cursor, uint8_t valueWireType);
void thrift_transcode_proto_end_map_entry(
    TranscodeCursor* cursor, TranscodePatchPoint mark);

// Reads the length of the map entry at the cursor and finds its key and value,
// which protobuf lets an entry carry in either order or leave out. Unknown
// entry fields are skipped. Narrows the read window to the entry until
// thrift_transcode_proto_leave_map_entry.
bool thrift_transcode_proto_enter_map_entry(
    TranscodeCursor* cursor,
    uint8_t keyWireType,
    uint8_t valueWireType,
    TranscodeProtoMapEntry* entry);
void thrift_transcode_proto_leave_map_entry(
    TranscodeCursor* cursor, const TranscodeProtoMapEntry* entry);

} // extern "C"
