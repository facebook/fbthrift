/*
 * Copyright 2019-present Facebook, Inc.
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

#include <thrift/lib/cpp2/protocol/NimbleProtocol.h>

namespace apache {
namespace thrift {

inline uint32_t NimbleProtocolWriter::writeMessageBegin(
    const std::string& /*name*/,
    MessageType /*messageType*/,
    int32_t /*seqid*/) {
  throw std::logic_error{"Method not implemented."};
}

inline uint32_t NimbleProtocolWriter::writeMessageEnd() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeStructBegin(const char* /*name*/) {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeStructEnd() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeFieldBegin(
    const char* /*name*/,
    TType fieldType,
    int16_t fieldId) {
  auto contentSize = detail::nimble::ttypeToContentSize(fieldType, encodeSize_);
  encoder_.encodeFieldChunk(fieldId << 2 | contentSize);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeFieldEnd() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeFieldStop() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeBool(bool value) {
  encode(value);
  return 0;
}
inline uint32_t NimbleProtocolWriter::writeByte(int8_t byte) {
  encode(byte);
  return 0;
}
inline uint32_t NimbleProtocolWriter::writeI16(int16_t i16) {
  encode(i16);
  return 0;
}
inline uint32_t NimbleProtocolWriter::writeI32(int32_t i32) {
  encode(i32);
  return 0;
}
inline uint32_t NimbleProtocolWriter::writeI64(int64_t i64) {
  encode(i64);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeDouble(double dub) {
  encode(dub);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeFloat(float flt) {
  encode(flt);
  return 0;
}

/**
 * Functions that return the serialized size
 */

inline uint32_t NimbleProtocolWriter::serializedMessageSize(
    const std::string& /*name*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedFieldSize(
    const char* /*name*/,
    TType /*fieldType*/,
    int16_t /*fieldId*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedStructSize(
    const char* /*name*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeMapBegin(
    TType /*keyType*/,
    TType /*valType*/,
    uint32_t /*size*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeMapEnd() const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeListBegin(
    TType /*elemType*/,
    uint32_t /*size*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeListEnd() const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeSetBegin(
    TType /*elemType*/,
    uint32_t /*size*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeSetEnd() const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeStop() const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeBool(bool /*val*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeByte(int8_t /*val*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeI16(int16_t /*val*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeI32(int32_t /*val*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeI64(int64_t /*val*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeDouble(
    double /*val*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeFloat(float /*val*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeString(
    folly::StringPiece /*str*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeBinary(
    folly::StringPiece /*str*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeBinary(
    folly::ByteRange /*v*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeBinary(
    std::unique_ptr<folly::IOBuf> const& /*v*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeBinary(
    folly::IOBuf const& /*v*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeZCBinary(
    folly::StringPiece /*str*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeZCBinary(
    folly::ByteRange /*v*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeZCBinary(
    std::unique_ptr<folly::IOBuf> const& /*v*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeZCBinary(
    folly::IOBuf const& /*v*/) const {
  return 0;
}
inline uint32_t NimbleProtocolWriter::serializedSizeSerializedData(
    std::unique_ptr<folly::IOBuf> const& /*data*/) const {
  return 0;
}

/**
 * Reading functions
 */
inline void NimbleProtocolReader::readMessageBegin(
    std::string& /*name*/,
    MessageType& /*messageType*/,
    int32_t& /*seqid*/) {}

inline void NimbleProtocolReader::readMessageEnd() {}
inline void NimbleProtocolReader::readStructBegin(std::string& /*name*/) {}
inline void NimbleProtocolReader::readStructEnd() {}
inline void NimbleProtocolReader::readFieldBegin(
    std::string& /*name*/,
    TType& /*fieldType*/,
    int16_t& /*fieldId*/) {}
inline void NimbleProtocolReader::readFieldEnd() {}
inline void NimbleProtocolReader::readBool(bool& value) {
  decode(value);
}
inline void NimbleProtocolReader::readBool(
    std::vector<bool>::reference /*value*/) {}

inline void NimbleProtocolReader::readByte(int8_t& byte) {
  decode(byte);
}
inline void NimbleProtocolReader::readI16(int16_t& i16) {
  decode(i16);
}
inline void NimbleProtocolReader::readI32(int32_t& i32) {
  decode(i32);
}
inline void NimbleProtocolReader::readI64(int64_t& i64) {
  decode(i64);
}

inline void NimbleProtocolReader::readDouble(double& dub) {
  decode(dub);
}

inline void NimbleProtocolReader::readFloat(float& flt) {
  decode(flt);
}

inline void NimbleProtocolReader::skip(TType type) {
  auto contentSize = detail::nimble::ttypeToContentSize(type, encodeSize_);
  switch (contentSize) {
    case detail::nimble::ONE_CHUNK:
      decoder_.nextContentChunk();
      break;
    case detail::nimble::TWO_CHUNKS:
      for (int i = 0; i < 2; ++i) {
        decoder_.nextContentChunk();
      }
      break;
    case detail::nimble::VAR_BYTES:
    case detail::nimble::VAR_FIELDS:
      // TODO: handle these cases
      break;
    default:
      folly::assume_unreachable();
  }
}

bool NimbleProtocolReader::advanceToNextField(
    int32_t /*currFieldId*/,
    int32_t /*nextFieldId*/,
    TType /*nextFieldType*/,
    StructReadState& state) {
  auto fieldIdAndSize = decoder_.nextFieldChunk();
  if (fieldIdAndSize == 0) {
    state.fieldType = TType::T_STOP;
  } else {
    state.fieldId = fieldIdAndSize >> 2;
    state.contentSize =
        static_cast<detail::nimble::NimbleContentSize>(fieldIdAndSize & 3);
    state.fieldType = TType::T_VOID; // just to make fieldType non-zero
  }
  return false;
}

bool NimbleProtocolReader::isCompatibleWithType(
    TType expectedFieldType,
    StructReadState& state) {
  // store the expected TType in case we need to skip
  state.fieldType = expectedFieldType;
  auto expectedContentSize =
      detail::nimble::ttypeToContentSize(expectedFieldType, encodeSize_);
  return state.contentSize == expectedContentSize;
}

} // namespace thrift
} // namespace apache
