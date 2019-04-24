/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <thrift/lib/cpp2/protocol/NimbleProtocol.h>
#include <folly/container/Array.h>
#include <folly/lang/Bits.h>

namespace apache {
namespace thrift {

void NimbleProtocolWriter::encode(bool input) {
  encoder_.encodeContentChunk(input);
}

void NimbleProtocolWriter::encode(int8_t input) {
  encoder_.encodeContentChunk(input);
}

void NimbleProtocolWriter::encode(int16_t input) {
  encoder_.encodeContentChunk(input);
}

void NimbleProtocolWriter::encode(int32_t input) {
  encoder_.encodeContentChunk(input);
}

void NimbleProtocolWriter::encode(int64_t input) {
  auto lower = static_cast<uint32_t>(input & 0xffffffff);
  auto higher = static_cast<uint32_t>(input >> 32);
  encoder_.encodeContentChunk(lower);
  encoder_.encodeContentChunk(higher);
}

void NimbleProtocolWriter::encode(uint8_t input) {
  encoder_.encodeContentChunk(input);
}

void NimbleProtocolWriter::encode(uint16_t input) {
  encoder_.encodeContentChunk(input);
}

void NimbleProtocolWriter::encode(uint32_t input) {
  encoder_.encodeContentChunk(input);
}

void NimbleProtocolWriter::encode(uint64_t input) {
  auto lower = static_cast<uint32_t>(input & 0xffffffff);
  auto higher = static_cast<uint32_t>(input >> 32);
  encoder_.encodeContentChunk(lower);
  encoder_.encodeContentChunk(higher);
}
void NimbleProtocolWriter::encode(double input) {
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  uint64_t bits = bitwise_cast<uint64_t>(input);
  encode(bits);
}

void NimbleProtocolWriter::encode(float input) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = bitwise_cast<uint32_t>(input);
  encode(bits);
}

void NimbleProtocolWriter::encode(folly::StringPiece input) {
  encoder_.encodeContentChunk(input.size());
  encoder_.encodeBinary(input.data(), input.size());
}

void NimbleProtocolWriter::encode(folly::ByteRange input) {
  // Note: this matches CompactProtocol in that it doesn't support input length
  // greater than UINT32_MAX
  // TODO: caller set string_limit
  encoder_.encodeContentChunk(input.size());
  encoder_.encodeBinary(input.data(), input.size());
}

void NimbleProtocolWriter::encodeStop() {
  encoder_.encodeFieldChunk(0);
}

void NimbleProtocolReader::decode(bool& value) {
  value = static_cast<bool>(decoder_.nextContentChunk());
}

void NimbleProtocolReader::decode(int8_t& value) {
  value = static_cast<int8_t>(decoder_.nextContentChunk());
}

void NimbleProtocolReader::decode(int16_t& value) {
  value = static_cast<int16_t>(decoder_.nextContentChunk());
}

void NimbleProtocolReader::decode(int32_t& value) {
  value = static_cast<int32_t>(decoder_.nextContentChunk());
}

void NimbleProtocolReader::decode(int64_t& value) {
  auto lower = decoder_.nextContentChunk();
  auto higher = decoder_.nextContentChunk();
  value = static_cast<int64_t>(higher) << 32 | lower;
}

void NimbleProtocolReader::decode(uint8_t& value) {
  value = static_cast<uint8_t>(decoder_.nextContentChunk());
}

void NimbleProtocolReader::decode(uint16_t& value) {
  value = static_cast<uint16_t>(decoder_.nextContentChunk());
}

void NimbleProtocolReader::decode(uint32_t& value) {
  value = static_cast<uint32_t>(decoder_.nextContentChunk());
}

void NimbleProtocolReader::decode(uint64_t& value) {
  auto lower = decoder_.nextContentChunk();
  auto higher = decoder_.nextContentChunk();
  value = static_cast<uint64_t>(higher) << 32 | lower;
}

void NimbleProtocolReader::decode(double& value) {
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  auto lower = decoder_.nextContentChunk();
  auto higher = decoder_.nextContentChunk();
  uint64_t bits = static_cast<uint64_t>(higher) << 32 | lower;
  value = bitwise_cast<double>(bits);
}

void NimbleProtocolReader::decode(float& value) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = decoder_.nextContentChunk();
  value = bitwise_cast<float>(bits);
}
} // namespace thrift
} // namespace apache
