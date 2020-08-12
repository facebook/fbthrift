/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <thrift/lib/cpp2/protocol/NimbleProtocol.h>

#include <folly/Utility.h>

namespace apache {
namespace thrift {

using namespace detail::nimble;

inline uint32_t NimbleProtocolWriter::writeMessageBegin(
    const std::string& /*name*/,
    MessageType /*messageType*/,
    int32_t /*seqid*/) {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeMessageEnd() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeStructBegin(const char* /*name*/) {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeStructEnd() {
  encoder_.encodeFieldBytes(stopBytes());
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeFieldBegin(
    const char* /*name*/,
    TType fieldType,
    int16_t fieldId) {
  FieldBytes info = fieldBeginBytes(ttypeToNimbleType(fieldType), fieldId);
  encoder_.encodeFieldBytes(info);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeFieldEnd() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeFieldStop() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeMapBegin(
    TType keyType,
    TType valType,
    uint32_t size) {
  encoder_.encodeFieldBytes(
      mapBeginByte(ttypeToNimbleType(keyType), ttypeToNimbleType(valType)));
  encoder_.encodeSizeChunk(size);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeMapEnd() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeCollectionBegin(
    TType elemType,
    uint32_t size) {
  encoder_.encodeFieldBytes(listBeginByte(ttypeToNimbleType(elemType)));
  encoder_.encodeSizeChunk(size);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeListBegin(
    TType elemType,
    uint32_t size) {
  return writeCollectionBegin(elemType, size);
}

inline uint32_t NimbleProtocolWriter::writeListEnd() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeSetBegin(
    TType elemType,
    uint32_t size) {
  return writeCollectionBegin(elemType, size);
}

inline uint32_t NimbleProtocolWriter::writeSetEnd() {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeBool(bool value) {
  encoder_.encodeContentChunk(value);
  return 0;
}
inline uint32_t NimbleProtocolWriter::writeByte(int8_t byte) {
  encoder_.encodeContentChunk(byte);
  return 0;
}
inline uint32_t NimbleProtocolWriter::writeI16(int16_t i16) {
  encoder_.encodeContentChunk(i16);
  return 0;
}
inline uint32_t NimbleProtocolWriter::writeI32(int32_t i32) {
  encoder_.encodeContentChunk(i32);
  return 0;
}
inline uint32_t NimbleProtocolWriter::writeI64(int64_t i64) {
  auto lower = static_cast<uint32_t>(i64 & 0xffffffff);
  auto higher = static_cast<uint32_t>(i64 >> 32);
  encoder_.encodeContentChunk(lower);
  encoder_.encodeContentChunk(higher);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeDouble(double dub) {
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  return writeI64(folly::bit_cast<int64_t>(dub));
}

inline uint32_t NimbleProtocolWriter::writeFloat(float flt) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = folly::bit_cast<uint32_t>(flt);
  encoder_.encodeContentChunk(bits);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeString(folly::StringPiece str) {
  writeBinary(folly::ByteRange(str));
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeBinary(folly::StringPiece str) {
  writeBinary(folly::ByteRange(str));
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeBinary(folly::ByteRange str) {
  DCHECK_LE(str.size(), folly::to_unsigned(std::numeric_limits<int>::max()));
  encoder_.encodeSizeChunk(folly::to_narrow(str.size()));
  encoder_.encodeBinary(str.data(), str.size());
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeBinary(
    const std::unique_ptr<folly::IOBuf>& str) {
  if (!str) {
    encoder_.encodeSizeChunk(0);
    return 0;
  }
  return writeBinary(*str);
}

inline uint32_t NimbleProtocolWriter::writeBinary(const folly::IOBuf& str) {
  size_t size = str.computeChainDataLength();
  DCHECK_LE(size, folly::to_unsigned(std::numeric_limits<int>::max()));
  encoder_.encodeSizeChunk(folly::to_narrow(size));
  encoder_.encodeBinary(str.data(), size);
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

inline void NimbleProtocolReader::readStructBegin(const std::string& /*name*/) {
}

inline void NimbleProtocolReader::readStructEnd() {}

inline void NimbleProtocolReader::readFieldBegin(
    std::string& /*name*/,
    NimbleType& /*fieldType*/,
    int16_t& /*fieldId*/) {}

inline void NimbleProtocolReader::readFieldEnd() {}

inline void NimbleProtocolReader::readMapBegin(
    NimbleType& keyType,
    NimbleType& valType,
    uint32_t& size) {
  std::uint8_t byte = decoder_.nextFieldByte();
  mapTypesFromByte(byte, keyType, valType);

  size = decoder_.nextSizeChunk();
  if (size > static_cast<uint32_t>(container_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }
}

inline void NimbleProtocolReader::readMapEnd() {}

inline void NimbleProtocolReader::readListBegin(
    NimbleType& elemType,
    uint32_t& size) {
  std::uint8_t byte = decoder_.nextFieldByte();
  listTypeFromByte(byte, elemType);
  size = decoder_.nextSizeChunk();
  if (size > static_cast<uint32_t>(container_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }
}

inline void NimbleProtocolReader::readListEnd() {}

inline void NimbleProtocolReader::readSetBegin(
    NimbleType& elemType,
    uint32_t& size) {
  readListBegin(elemType, size);
}

inline void NimbleProtocolReader::readSetEnd() {
  readListEnd();
}

inline void NimbleProtocolReader::readBool(bool& value) {
  value = static_cast<bool>(decoder_.nextContentChunk());
}

inline void NimbleProtocolReader::readBoolWithContext(
    bool& value,
    StructReadState& srs) {
  value = static_cast<bool>(decoder_.nextContentChunk(srs.decoderState));
}

inline void NimbleProtocolReader::readBool(std::vector<bool>::reference value) {
  bool val;
  readBool(val);
  value = val;
}

inline void NimbleProtocolReader::readByte(int8_t& byte) {
  byte = static_cast<int8_t>(decoder_.nextContentChunk());
}

inline void NimbleProtocolReader::readByteWithContext(
    int8_t& byte,
    StructReadState& srs) {
  byte = static_cast<int8_t>(decoder_.nextContentChunk(srs.decoderState));
}

inline void NimbleProtocolReader::readI16(int16_t& i16) {
  i16 = static_cast<int16_t>(decoder_.nextContentChunk());
}

inline void NimbleProtocolReader::readI16WithContext(
    int16_t& i16,
    StructReadState& srs) {
  i16 = static_cast<int16_t>(decoder_.nextContentChunk(srs.decoderState));
}

inline void NimbleProtocolReader::readI32(int32_t& i32) {
  i32 = static_cast<int32_t>(decoder_.nextContentChunk());
}

inline void NimbleProtocolReader::readI32WithContext(
    int32_t& i32,
    StructReadState& srs) {
  i32 = static_cast<int32_t>(decoder_.nextContentChunk(srs.decoderState));
}

inline void NimbleProtocolReader::readI64(int64_t& i64) {
  auto lower = decoder_.nextContentChunk();
  auto higher = decoder_.nextContentChunk();
  i64 = static_cast<int64_t>(higher) << 32 | lower;
}

inline void NimbleProtocolReader::readI64WithContext(
    int64_t& i64,
    StructReadState& srs) {
  auto lower = decoder_.nextContentChunk(srs.decoderState);
  auto higher = decoder_.nextContentChunk(srs.decoderState);
  i64 = static_cast<int64_t>(higher) << 32 | lower;
}

inline void NimbleProtocolReader::readDouble(double& dub) {
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  int64_t bits = 0;
  readI64(bits);
  dub = folly::bit_cast<double>(bits);
}

inline void NimbleProtocolReader::readDoubleWithContext(
    double& dub,
    StructReadState& srs) {
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  int64_t bits = 0;
  readI64WithContext(bits, srs);
  dub = folly::bit_cast<double>(bits);
}

inline void NimbleProtocolReader::readFloat(float& flt) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = decoder_.nextContentChunk();
  flt = folly::bit_cast<float>(bits);
}

inline void NimbleProtocolReader::readFloatWithContext(
    float& flt,
    StructReadState& srs) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = decoder_.nextContentChunk(srs.decoderState);
  flt = folly::bit_cast<float>(bits);
}

template <typename StrType>
inline void NimbleProtocolReader::readString(StrType& str) {
  uint32_t size = decoder_.nextSizeChunk();
  if (size > static_cast<uint32_t>(string_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }

  decoder_.nextBinary(str, size);
}

template <typename StrType>
inline void NimbleProtocolReader::readStringWithContext(
    StrType& str,
    StructReadState& /* srs */) {
  uint32_t size = decoder_.nextSizeChunk();
  if (size > static_cast<uint32_t>(string_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }

  decoder_.nextBinary(str, size);
}

template <typename StrType>
inline void NimbleProtocolReader::readBinary(StrType& str) {
  readString(str);
}

template <typename StrType>
inline void NimbleProtocolReader::readBinaryWithContext(
    StrType& str,
    StructReadState& /* srs */) {
  readString(str);
}

inline void NimbleProtocolReader::readBinary(
    std::unique_ptr<folly::IOBuf>& str) {
  if (!str) {
    str = std::make_unique<folly::IOBuf>();
  }
  readBinary(*str);
}

inline void NimbleProtocolReader::readBinaryWithContext(
    std::unique_ptr<folly::IOBuf>& str,
    StructReadState& srs) {
  if (!str) {
    str = std::make_unique<folly::IOBuf>();
  }
  readBinaryWithContext(*str, srs);
}

inline void NimbleProtocolReader::readBinary(folly::IOBuf& str) {
  uint32_t size = decoder_.nextSizeChunk();

  if (size > static_cast<uint32_t>(string_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }
  decoder_.nextBinary(str, size);
}

inline void NimbleProtocolReader::readBinaryWithContext(
    folly::IOBuf& str,
    StructReadState& /* srs */) {
  uint32_t size = decoder_.nextSizeChunk();

  if (size > static_cast<uint32_t>(string_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }
  decoder_.nextBinary(str, size);
}

inline void NimbleProtocolReader::skip_n(
    std::uint32_t n,
    std::initializer_list<detail::nimble::NimbleType> types) {
  StructReadState state;
  state.borrowFieldPtrs(this);
  state.decoderState = borrowState();
  for (std::uint32_t i = 0; i < n; ++i) {
    for (auto type : types) {
      state.nimbleType = type;
      state = skip(std::move(state));
    }
  }
  state.returnFieldPtrs(this);
  returnState(std::move(state.decoderState));
}

inline NimbleProtocolReader::StructReadState NimbleProtocolReader::skip(
    StructReadState state) {
  // As a precondition, state has borrowed from the protocol. So, in cases where
  // we need to call a method that affects protocol state (like when skipping
  // containers), we need to return and borrow it again before recursing into a
  // skip call.
  switch (state.nimbleType) {
    case NimbleType::STOP: {
      TProtocolException::throwInvalidSkipType(protocol::T_STOP);
    }
    case NimbleType::ONE_CHUNK: {
      decoder_.nextContentChunk(state.decoderState);
      break;
    }
    case NimbleType::TWO_CHUNK: {
      decoder_.nextContentChunk(state.decoderState);
      decoder_.nextContentChunk(state.decoderState);
      break;
    }
    case NimbleType::STRING: {
      std::uint32_t size = decoder_.nextSizeChunk();
      decoder_.skipStringBytes(size);
      break;
    }
    case NimbleType::STRUCT: {
      while (true) {
        state.readFieldBegin(this);
        if (state.atStop()) {
          break;
        }
        state = skip(std::move(state));
      }
      break;
    }
    case NimbleType::LIST: {
      NimbleType elemType;
      std::uint32_t size;
      // Have to maintain borrowship precondition.
      state.returnFieldPtrs(this);
      readListBegin(elemType, size);
      state.borrowFieldPtrs(this);
      for (std::uint32_t i = 0; i < size; ++i) {
        state.nimbleType = elemType;
        state = skip(std::move(state));
      }
      break;
    }
    case NimbleType::MAP: {
      NimbleType keyType;
      NimbleType valueType;
      std::uint32_t size;
      // Have to maintain borrowship precondition.
      state.returnFieldPtrs(this);
      readMapBegin(keyType, valueType, size);
      state.borrowFieldPtrs(this);
      for (std::uint32_t i = 0; i < size; ++i) {
        state.nimbleType = keyType;
        state = skip(std::move(state));
        state.nimbleType = valueType;
        state = skip(std::move(state));
      }
      break;
    }
    case NimbleType::INVALID: {
      TProtocolException::throwInvalidSkipType(protocol::T_STOP);
    }
  }
  return state;
}

inline void NimbleProtocolReader::advanceToNextFieldSlow(
    StructReadState& state) {
  std::uint8_t firstByte = decoder_.nextFieldByte();
  if (isCompactMetadata(firstByte)) {
    // Short encoding
    state.nimbleType = nimbleTypeFromCompactMetadata(firstByte);
    state.fieldId = fieldIdFromCompactMetadata(firstByte);
  } else {
    state.nimbleType = nimbleTypeFromLargeMetadata(firstByte);
    if (isLargeMetadataTwoByte(firstByte)) {
      state.fieldId =
          fieldIdFromTwoByteMetadata(firstByte, decoder_.nextFieldByte());
    } else {
      state.fieldId =
          fieldIdFromThreeByteMetadata(firstByte, decoder_.nextFieldShort());
    }
  }
}

inline detail::BufferingNimbleDecoderState NimbleProtocolReader::borrowState() {
  return decoder_.borrowState();
}

inline void NimbleProtocolReader::returnState(
    detail::BufferingNimbleDecoderState state) {
  decoder_.returnState(std::move(state));
}

bool NimbleProtocolReader::isCompatibleWithType(
    TType expectedFieldType,
    StructReadState& state) {
  return ttypeToNimbleType(expectedFieldType) == state.nimbleType;
}
} // namespace thrift
} // namespace apache
