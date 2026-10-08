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
#include <thrift/lib/cpp2/transcode/InterpreterInternal.h>
#include <thrift/lib/cpp2/transcode/TranscodeInput.h>
#include <thrift/lib/cpp2/transcode/TranscodePlan.h>

#include <folly/Range.h>

// JSON names struct fields instead of numbering them and carries no container
// counts. These steps read and write JSON for the interpreter.
namespace apache::thrift::transcode::detail {

// A JSON source may end with whitespace after its value, but nothing else.
void validateInputConsumed(TranscodeCursor* c, const TranscodePlan& plan);

void execJsonBytesScalar(TranscodeCursor* c, const ScalarOp& op);

void execJsonSeq(TranscodeCursor* c, const SeqOp& op);
void execJsonSeqTarget(TranscodeCursor* c, const SeqOp& op);

void execJsonMap(TranscodeCursor* c, const MapOp& op);
void execJsonMapTarget(TranscodeCursor* c, const MapOp& op);

// `ignoredMember` is the tag of an internally tagged union, which shares the
// arm's JSON object without being one of the arm's fields.
void execJsonStruct(
    TranscodeCursor* c,
    const StructOp& op,
    ScalarFieldOverrides fieldOverrides = {},
    folly::ByteRange ignoredMember = {});
void execJsonStructFlattened(TranscodeCursor* c, const StructOp& op);
void execIdStructToJson(
    TranscodeCursor* c,
    const StructOp& op,
    FieldProto readProto,
    const Framing& rf,
    ScalarFieldOverrides fieldOverrides);

} // namespace apache::thrift::transcode::detail
