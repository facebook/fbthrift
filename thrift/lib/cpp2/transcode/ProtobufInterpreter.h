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

#include <cstdint>
#include <vector>

// Protobuf repeated fields and maps have no container header: each element or
// entry is its own occurrence of the field. These steps convert between those
// occurrences and a Thrift list, set or map.
namespace apache::thrift::transcode::detail {

bool writesProtobufOccurrences(const Command& cmd);
bool readsProtobufOccurrences(const FieldEntry& field);

// Packed scalars go in one record, other elements and map entries in one
// record each. An empty container writes nothing, as protobuf encoders do.
void writeProtobufOccurrences(
    TranscodeCursor* c, const Command& cmd, const FieldEntry& field);

// Reads the consecutive occurrences of `field`, starting with the one whose
// header was just read. `fieldsRead` holds the repeated fields the struct has
// already read: a later occurrence would need a second Thrift container, so it
// is rejected.
void readProtobufOccurrences(
    TranscodeCursor* c,
    const FieldEntry& field,
    uint8_t typeInfo,
    std::vector<int16_t>& fieldsRead);

} // namespace apache::thrift::transcode::detail
