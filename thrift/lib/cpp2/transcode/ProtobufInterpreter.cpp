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

#include <thrift/lib/cpp2/transcode/ProtobufInterpreter.h>

#include <thrift/lib/cpp2/transcode/InterpreterInternal.h>
#include <thrift/lib/cpp2/transcode/Intrinsics.h>
#include <thrift/lib/cpp2/transcode/ReadHelpers.h>

#include <folly/CppAttributes.h>

#include <algorithm>
#include <variant>
#include <vector>

namespace apache::thrift::transcode::detail {
namespace {

constexpr int64_t kMalformedProtobuf = 1;

bool isThriftContainerFraming(ContainerFraming framing) {
  return framing == ContainerFraming::Compact ||
      framing == ContainerFraming::Binary;
}

void writeThriftDefault(
    TranscodeCursor* c, ContainerFraming framing, uint8_t type) {
  if (framing == ContainerFraming::Compact) {
    thrift_transcode_compact_write_default(c, type);
  } else {
    thrift_transcode_binary_write_default(c, type);
  }
}

// A part the entry leaves out takes its type's default, as protobuf parsers
// do.
void readMapEntryPart(
    TranscodeCursor* c,
    const Command& cmd,
    const uint8_t* FOLLY_NULLABLE start,
    uint8_t wireType,
    ContainerFraming framing,
    uint8_t type) {
  if (start == nullptr) {
    writeThriftDefault(c, framing, type);
    return;
  }
  c->readPos = start;
  execCommand(c, cmd, static_cast<uint8_t>(wireType + 1));
}

void readMapEntry(TranscodeCursor* c, const MapOp& op) {
  TranscodeProtoMapEntry entry{};
  if (!thrift_transcode_proto_enter_map_entry(
          c, op.readKeyWireType, op.readValueWireType, &entry)) {
    return;
  }
  readMapEntryPart(
      c,
      *op.key,
      entry.key,
      op.readKeyWireType,
      op.writeFraming,
      op.writeKeyType);
  if (!hasError(c)) {
    readMapEntryPart(
        c,
        *op.value,
        entry.value,
        op.readValueWireType,
        op.writeFraming,
        op.writeValueType);
  }
  thrift_transcode_proto_leave_map_entry(c, &entry);
}

} // namespace

bool writesProtobufOccurrences(const Command& cmd) {
  if (const auto* seq = std::get_if<SeqOp>(&cmd)) {
    return isThriftContainerFraming(seq->readFraming) &&
        seq->writeFraming == ContainerFraming::None;
  }
  if (const auto* map = std::get_if<MapOp>(&cmd)) {
    return isThriftContainerFraming(map->readFraming) &&
        map->writeEntryIsSubmessage;
  }
  return false;
}

bool readsProtobufOccurrences(const FieldEntry& field) {
  if (!field.isRepeated) {
    return false;
  }
  if (const auto* seq = std::get_if<SeqOp>(field.command.get())) {
    return seq->readFraming == ContainerFraming::None &&
        seq->readLoopKind == LoopKind::ByCount;
  }
  if (const auto* map = std::get_if<MapOp>(field.command.get())) {
    return map->readEntryIsSubmessage;
  }
  return false;
}

void writeProtobufOccurrences(
    TranscodeCursor* c, const Command& cmd, const FieldEntry& field) {
  const int16_t fieldId = targetFieldId(field);
  if (const auto* seq = std::get_if<SeqOp>(&cmd)) {
    const uint32_t count = readSeqCount(c, seq->readFraming, seq->readElemType);
    if (count == 0 || hasError(c)) {
      return;
    }
    if (seq->writeLoopKind == LoopKind::ByBytes) {
      thrift_transcode_proto_write_field_header(
          c, field.writeTypeInfo, fieldId, 0);
      const TranscodePatchPoint writeMark =
          thrift_transcode_proto_reserve_length(c);
      for (uint32_t i = 0; i < count && !hasError(c); ++i) {
        execCommand(c, *seq->element, 0);
      }
      thrift_transcode_proto_patch_length(c, writeMark);
      return;
    }
    for (uint32_t i = 0; i < count && !hasError(c); ++i) {
      thrift_transcode_proto_write_field_header(
          c, field.writeTypeInfo, fieldId, 0);
      execCommand(c, *seq->element, 0);
    }
    return;
  }
  const auto& map = std::get<MapOp>(cmd);
  const uint32_t count =
      readMapCount(c, map.readFraming, map.readKeyType, map.readValueType);
  for (uint32_t i = 0; i < count && !hasError(c); ++i) {
    const TranscodePatchPoint writeMark =
        thrift_transcode_proto_begin_map_entry(
            c, fieldId, map.writeKeyWireType);
    execCommand(c, *map.key, 0);
    thrift_transcode_proto_write_map_value_header(c, map.writeValueWireType);
    execCommand(c, *map.value, 0);
    thrift_transcode_proto_end_map_entry(c, writeMark);
  }
}

void readProtobufOccurrences(
    TranscodeCursor* c,
    const FieldEntry& field,
    uint8_t typeInfo,
    std::vector<int16_t>& fieldsRead) {
  if (std::find(fieldsRead.begin(), fieldsRead.end(), field.fieldId) !=
      fieldsRead.end()) {
    setError(c, kMalformedProtobuf);
    return;
  }
  fieldsRead.push_back(field.fieldId);
  const auto* seq = std::get_if<SeqOp>(field.command.get());
  const auto* map = std::get_if<MapOp>(field.command.get());
  const TranscodePatchPoint writeMark = seq != nullptr
      ? reserveSeqHeader(c, seq->writeFraming)
      : reserveNonEmptyMapHeader(c, map->writeFraming);
  uint32_t count = 0;
  do {
    if (seq != nullptr) {
      execCommand(c, *seq->element, typeInfo);
    } else {
      readMapEntry(c, *map);
    }
    ++count;
  } while (
      !hasError(c) &&
      thrift_transcode_proto_read_next_occurrence(c, field.fieldId, typeInfo));
  if (hasError(c)) {
    return;
  }
  if (seq != nullptr) {
    patchSeqHeader(c, writeMark, seq->writeFraming, count, seq->writeElemType);
  } else {
    patchNonEmptyMapHeader(
        c,
        writeMark,
        map->writeFraming,
        count,
        map->writeKeyType,
        map->writeValueType);
  }
}

} // namespace apache::thrift::transcode::detail
