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

#include <thrift/lib/cpp2/protocol/Protocol.h>

#include <string>

namespace apache::thrift {

// This module allows us to take an arbitrary serialized Protocol stream
// and print it to human readable format, without needing the Thrift
// structure definition.
//
// In doing so, we won't be able to render the tag names, but we can
// still provide some useful debugging info, particularly for data
// that might be in an arbitrary table or the result of some computation, e.g.
//
//  toDebugString(protoReader) ->
//    struct {
//      1: string = "image.jpg"
//      2: i32 = 640
//      3: i32 = 480
//      4: float = 0.85
//      5 list<i64> = [
//        2342342343,
//        4353453453,
//      ]
//    }
//
// WARNING: you should not store data persistently in this text string format,
// since it's subject to change. Instead prefer the standard Compact/Binary/etc.
// Protocol formats.

// Converts serialized protocol data -> text format
//
// Usage example:
//   apache::thrift::CompactProtocolReader reader;
//   reader.setInput(iobufWithSerializedData);
//   std::cout << apache::thrift::toDebugString(reader);
//
template <ThriftProtocolReader ProtocolReader>
std::string toDebugString(ProtocolReader& inProtoReader);

} // namespace apache::thrift
