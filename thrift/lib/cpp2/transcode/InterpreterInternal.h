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

#include <thrift/lib/cpp2/transcode/Codec.h>
#include <thrift/lib/cpp2/transcode/Cursor.h>
#include <thrift/lib/cpp2/transcode/TranscodeInput.h>

#include <folly/CPortability.h>
#include <folly/CppAttributes.h>
#include <folly/Likely.h>

#include <cstddef>
#include <cstdint>

// Interpreter steps the protocol-specific interpreter files build on.
namespace apache::thrift::transcode::detail {

inline constexpr int64_t kMalformedFieldType = 1;
inline constexpr int64_t kUnsupportedProtocol = 90;

FOLLY_ALWAYS_INLINE bool hasError(const TranscodeCursor* c) {
  return FOLLY_UNLIKELY(c->error != 0);
}

struct Framing {
  uint8_t (*readHeader)(TranscodeCursor*, int16_t*, int16_t) = nullptr;
  void (*writeHeader)(TranscodeCursor*, uint8_t, int16_t, int16_t) = nullptr;
  void (*writeStop)(TranscodeCursor*) = nullptr;
  void (*skip)(TranscodeCursor*, uint8_t) = nullptr;
};

Framing framingFor(FieldProto p);

void execCommand(TranscodeCursor* c, const Command& cmd, uint8_t fieldTypeInfo);
void execScalar(TranscodeCursor* c, const ScalarOp& op, uint8_t fieldTypeInfo);

bool intFits(ValueKind kind, int64_t v);
bool writeScalarInt(TranscodeCursor* c, const ScalarOp& op, int64_t v);
bool writeScalarBytes(
    TranscodeCursor* c, WriteFn fn, const uint8_t* data, size_t len);
bool writeScalarValue(
    TranscodeCursor* c, const ScalarOp& op, const ScalarOverrideValue& value);

void writeMapHeader(
    TranscodeCursor* c,
    ContainerFraming framing,
    uint32_t count,
    uint8_t keyType,
    uint8_t valueType);
TranscodePatchPoint reserveNonEmptyMapHeader(
    TranscodeCursor* c, ContainerFraming framing);
void patchNonEmptyMapHeader(
    TranscodeCursor* c,
    TranscodePatchPoint writeMark,
    ContainerFraming framing,
    uint32_t count,
    uint8_t keyType,
    uint8_t valueType);

int16_t targetFieldId(const FieldEntry& field);

bool isUnion(const StructOp& op);
bool noteSingleField(TranscodeCursor* c, bool& fieldSeen);
bool finishSingleField(TranscodeCursor* c, bool fieldSeen);
bool noteUnionMember(
    TranscodeCursor* c, bool unionStruct, bool& unionMemberSeen);
bool finishUnion(TranscodeCursor* c, bool unionStruct, bool unionMemberSeen);

bool resolveScalarOverrideField(
    TranscodeCursor* c,
    const StructOp& op,
    const ScalarFieldOverride& override,
    const FieldEntry*& field,
    const ScalarOp*& scalar);
bool writeFieldFramedScalarValue(
    TranscodeCursor* c,
    const Framing& wf,
    const FieldEntry& field,
    const ScalarOp& scalar,
    const ScalarOverrideValue& value,
    int16_t& prevWrite);

struct IdFieldMatch {
  int16_t fieldId = 0;
  uint8_t typeInfo = 0;
  const FieldEntry* field = nullptr;
};

bool enterIdStructRead(
    TranscodeCursor* c, const StructOp& op, const uint8_t*& savedReadEnd);
void restoreIdStructReadEnd(
    TranscodeCursor* c,
    const StructOp& op,
    const uint8_t* FOLLY_NULLABLE savedReadEnd);
// A successful match has a command and a schema-compatible wire type.
bool readNextIdField(
    TranscodeCursor* c,
    const StructOp& op,
    FieldProto readProto,
    const Framing& rf,
    int16_t& prevRead,
    IdFieldMatch& match,
    UnknownFieldMode unknownFieldMode);

bool execFlattenedField(
    TranscodeCursor* c,
    const FieldEntry& field,
    uint8_t typeInfo,
    bool& fieldSeen);

} // namespace apache::thrift::transcode::detail
