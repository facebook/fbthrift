/*
 * Copyright 2004-present Facebook, Inc.
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

namespace apache {
namespace thrift {

uint32_t Frozen2ProtocolWriter::writeMessageBegin(
    const std::string& name,
    MessageType messageType,
    int32_t seqid) {
  uint32_t wsize = 0;
  wsize += writeByte(detail::FROZEN_PROTOCOL_ID);
  wsize += writeByte(detail::FROZEN_PROTOCOL_VERSION);
  wsize += writeI16(static_cast<int16_t>(messageType));
  wsize += writeString(name);
  wsize += writeI32(seqid);
  return wsize;
}

uint32_t Frozen2ProtocolWriter::writeMessageEnd() {
  return 0;
}

template <typename T>
uint32_t Frozen2ProtocolWriter::writeObject(T& o) {
  std::string str;
  frozen::freezeToString(o, str);
  return writeString(str);
}

uint32_t Frozen2ProtocolWriter::writeByte(int8_t byte) {
  out_.write(byte);
  return sizeof(byte);
}

uint32_t Frozen2ProtocolWriter::writeI16(int16_t i16) {
  out_.writeBE(i16);
  return sizeof(int16_t);
}

uint32_t Frozen2ProtocolWriter::writeI32(int32_t i32) {
  out_.writeBE(i32);
  return sizeof(int32_t);
}

uint32_t Frozen2ProtocolWriter::writeString(folly::StringPiece str) {
  folly::ByteRange range(str);
  uint32_t size = range.size();
  if (size > std::numeric_limits<int32_t>::max()) {
    TProtocolException::throwExceededSizeLimit();
  }
  uint32_t result = writeI32(static_cast<int32_t>(size));
  out_.push(range.data(), size);
  return result + size;
}

uint32_t Frozen2ProtocolWriter::serializedMessageSize(
    const std::string& name) const {
  return 2 * serializedSizeI32() + serializedSizeString(name);
}

uint32_t Frozen2ProtocolWriter::serializedSizeI16() const {
  return 2;
}

uint32_t Frozen2ProtocolWriter::serializedSizeI32() const {
  return 4;
}

uint32_t Frozen2ProtocolWriter::serializedSizeString(
    folly::StringPiece str) const {
  return serializedSizeI32() + str.size();
}

template <typename T>
uint32_t Frozen2ProtocolWriter::serializedObjectSize(T& o) const {
  return frozen::frozenSize(o);
}

uint32_t Frozen2ProtocolReader::readMessageBegin(
    std::string& name,
    MessageType& messageType,
    int32_t& seqid) {
  uint32_t result = 0;
  int32_t sz;
  result += readI32(sz);
  int8_t protId = static_cast<int8_t>(sz >> 24);
  if (protId != detail::FROZEN_PROTOCOL_ID) {
    throwBadProtocolId(sz);
  }

  int8_t version = static_cast<int8_t>(sz >> 16);
  if (version != detail::FROZEN_PROTOCOL_VERSION) {
    throwBadProtocolVersion(sz);
  }
  messageType = static_cast<MessageType>(static_cast<uint16_t>(sz));
  result += readString(name);
  result += readI32(seqid);

  return result;
}

uint32_t Frozen2ProtocolReader::readMessageEnd() {
  return 0;
}

template <typename T>
uint32_t Frozen2ProtocolReader::readObject(T& o) {
  std::string str;
  frozen::Layout<T> layout;
  uint32_t len = readString(str);
  folly::ByteRange range((folly::StringPiece(str)));
  frozen::deserializeRootLayout(range, layout);
  layout.thaw({range.begin(), 0}, o);
  return len;
}

uint32_t Frozen2ProtocolReader::readI32(int32_t& i32) {
  i32 = in_.readBE<int32_t>();
  return 4;
}

uint32_t Frozen2ProtocolReader::readString(std::string& str) {
  uint32_t result;
  int32_t size;
  result = readI32(size);
  return result + readStringBody(str, size);
}

uint32_t Frozen2ProtocolReader::readStringBody(std::string& str, int32_t size) {
  str.reserve(size);
  str.clear();
  size_t sizeLeft = size;
  while (sizeLeft > 0) {
    auto data = in_.peekBytes();
    auto data_avail = std::min(data.size(), sizeLeft);
    if (data.empty()) {
      TProtocolException::throwExceededSizeLimit();
    }

    str.append(reinterpret_cast<const char*>(data.data()), data_avail);
    sizeLeft -= data_avail;
    in_.skip(data_avail);
  }
  return static_cast<uint32_t>(size);
}
}
} // apache::thrift
