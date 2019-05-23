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

inline void NimbleProtocolWriter::encodeComplexTypeMetadata(
    uint32_t size,
    StructyType structyType) {
  int metadataBits = (structyType == StructyType::NONE)
      ? kComplexMetadataBits
      : kStructyTypeMetadataBits;
  ComplexType complexType = (structyType == StructyType::NONE)
      ? ComplexType::STRINGY
      : ComplexType::STRUCTY;
  // The lowest two bits of the fieldChunk indicates that this is a complex
  // type field. The third lowest bit denotes whether this field is a stringy
  // or structy type. The next 4 bits encode the specific type of the structy
  // field, if this is a structy type. Otherwise no effect.
  uint32_t metadata = (structyType << kComplexMetadataBits) |
      (complexType << kFieldChunkHintBits) |
      NimbleFieldChunkHint::COMPLEX_METADATA;
  // To use the short size encoding, we need the high bit of the resulting
  // chunk to be 0, and so must fit in 31 bits, including the shift for the
  // metadata.
  if (LIKELY(size < (1U << (31 - metadataBits)))) {
    encoder_.encodeFieldChunk((size << metadataBits) | metadata);
  } else {
    // long size encoding, takes two field chunks
    encoder_.encodeFieldChunk((1U << 31) | (size << metadataBits) | metadata);
    encoder_.encodeFieldChunk(
        (size >> (31 - metadataBits)) << kFieldChunkHintBits |
        NimbleFieldChunkHint::COMPLEX_METADATA);
  }
}

inline uint32_t NimbleProtocolWriter::writeStructBegin(const char* /*name*/) {
  // The lowest two bits of this fieldChunk is a fieldChunkHint (00) indicating
  // that this is a complex type. The third lowest bit (0) denotes that this is
  // a structy type rather than a stringy type. The next 4 bits encode the
  // specific type of the structy field, in this case a struct or a union.
  encoder_.encodeFieldChunk(
      StructyType::STRUCT << kComplexMetadataBits |
      ComplexType::STRUCTY << kFieldChunkHintBits |
      NimbleFieldChunkHint::COMPLEX_METADATA);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeStructEnd() {
  // field chunk 0 marks the end of a struct
  encoder_.encodeFieldChunk(0);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeFieldBegin(
    const char* /*name*/,
    TType fieldType,
    int16_t fieldId) {
  auto fieldChunkHint = ttypeToNimbleFieldChunkHint(fieldType);
  encoder_.encodeFieldChunk(fieldId << kFieldChunkHintBits | fieldChunkHint);
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
  StructyType structyType = getStructyTypeFromMap(keyType, valType);
  encodeComplexTypeMetadata(size, structyType);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeMapEnd() {
  // this 0 field chunk marks the end of the map
  encoder_.encodeFieldChunk(0);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeCollectionBegin(
    TType elemType,
    uint32_t size) {
  StructyType structyType = getStructyTypeFromListOrSet(elemType);
  encodeComplexTypeMetadata(size, structyType);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeListBegin(
    TType elemType,
    uint32_t size) {
  return writeCollectionBegin(elemType, size);
}

inline uint32_t NimbleProtocolWriter::writeListEnd() {
  // this 0 field chunk marks the end of the list
  encoder_.encodeFieldChunk(0);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeSetBegin(
    TType elemType,
    uint32_t size) {
  return writeCollectionBegin(elemType, size);
}

inline uint32_t NimbleProtocolWriter::writeSetEnd() {
  return writeListEnd();
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

  return writeI64(bitwise_cast<int64_t>(dub));
}

inline uint32_t NimbleProtocolWriter::writeFloat(float flt) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = bitwise_cast<uint32_t>(flt);
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
  encodeComplexTypeMetadata(str.size());
  encoder_.encodeBinary(str.data(), str.size());
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeBinary(
    const std::unique_ptr<folly::IOBuf>& str) {
  if (!str) {
    encodeComplexTypeMetadata(0);
    return 0;
  }
  return writeBinary(*str);
}

inline uint32_t NimbleProtocolWriter::writeBinary(const folly::IOBuf& str) {
  size_t size = str.computeChainDataLength();
  encodeComplexTypeMetadata(size);
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
  uint32_t nextFieldChunk = decoder_.nextFieldChunk();
  // sanity check
  checkComplexFieldData(nextFieldChunk, ComplexType::STRUCTY);
}

inline void NimbleProtocolReader::readStructEnd() {}

inline void NimbleProtocolReader::readFieldBegin(
    std::string& /*name*/,
    TType& /*fieldType*/,
    int16_t& /*fieldId*/) {}
inline void NimbleProtocolReader::readFieldEnd() {}

inline uint32_t NimbleProtocolReader::readComplexTypeSize(
    ComplexType complexType) {
  uint64_t metadataChunk = decoder_.nextMetadataChunks();

  int metadataBits = (complexType == ComplexType::STRINGY)
      ? kComplexMetadataBits
      : kStructyTypeMetadataBits;

  // sanity check
  checkComplexFieldData(
      static_cast<uint32_t>(metadataChunk & 0xffffffff), complexType);

  return static_cast<uint32_t>(metadataChunk >> metadataBits);
}

inline void NimbleProtocolReader::readMapBegin(
    TType& /*keyType*/,
    TType& /*valType*/,
    uint32_t& size) {
  size = readComplexTypeSize(ComplexType::STRUCTY);

  if (size > static_cast<uint32_t>(container_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }
}

inline void NimbleProtocolReader::readMapEnd() {
  // To consume the fieldChunk 0 which marks the end of the map
  uint32_t fieldChunk = decoder_.nextFieldChunk();
  if (UNLIKELY(fieldChunk != 0)) {
    TProtocolException::throwInvalidFieldData();
  }
}

inline void NimbleProtocolReader::readListBegin(
    TType& /*elemType*/,
    uint32_t& size) {
  size = readComplexTypeSize(ComplexType::STRUCTY);

  if (size > static_cast<uint32_t>(container_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }
}

inline void NimbleProtocolReader::readListEnd() {
  // To consume the fieldChunk 0 which marks the end of the list
  uint32_t fieldChunk = decoder_.nextFieldChunk();
  if (UNLIKELY(fieldChunk != 0)) {
    TProtocolException::throwInvalidFieldData();
  }
}

inline void NimbleProtocolReader::readSetBegin(
    TType& elemType,
    uint32_t& size) {
  readListBegin(elemType, size);
}

inline void NimbleProtocolReader::readSetEnd() {
  readListEnd();
}

inline void NimbleProtocolReader::readBool(bool& value) {
  value = static_cast<bool>(decoder_.nextContentChunk());
}
inline void NimbleProtocolReader::readBool(
    std::vector<bool>::reference /*value*/) {}

inline void NimbleProtocolReader::readByte(int8_t& byte) {
  byte = static_cast<int8_t>(decoder_.nextContentChunk());
}
inline void NimbleProtocolReader::readI16(int16_t& i16) {
  i16 = static_cast<int16_t>(decoder_.nextContentChunk());
}
inline void NimbleProtocolReader::readI32(int32_t& i32) {
  i32 = static_cast<int32_t>(decoder_.nextContentChunk());
}
inline void NimbleProtocolReader::readI64(int64_t& i64) {
  auto lower = decoder_.nextContentChunk();
  auto higher = decoder_.nextContentChunk();
  i64 = static_cast<int64_t>(higher) << 32 | lower;
}

inline void NimbleProtocolReader::readDouble(double& dub) {
  static_assert(sizeof(double) == sizeof(uint64_t), "");
  static_assert(std::numeric_limits<double>::is_iec559, "");

  int64_t bits = 0;
  readI64(bits);
  dub = bitwise_cast<double>(bits);
}

inline void NimbleProtocolReader::readFloat(float& flt) {
  static_assert(sizeof(float) == sizeof(uint32_t), "");
  static_assert(std::numeric_limits<float>::is_iec559, "");

  uint32_t bits = decoder_.nextContentChunk();
  flt = bitwise_cast<float>(bits);
}

template <typename StrType>
inline void NimbleProtocolReader::readString(StrType& str) {
  uint32_t size = readComplexTypeSize(ComplexType::STRINGY);
  if (size > static_cast<uint32_t>(string_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }

  decoder_.nextBinary(str, size);
}

template <typename StrType>
inline void NimbleProtocolReader::readBinary(StrType& str) {
  readString(str);
}

inline void NimbleProtocolReader::readBinary(
    std::unique_ptr<folly::IOBuf>& str) {
  if (!str) {
    str = std::make_unique<folly::IOBuf>();
  }
  readBinary(*str);
}

inline void NimbleProtocolReader::readBinary(folly::IOBuf& str) {
  uint32_t size = readComplexTypeSize(ComplexType::STRINGY);

  if (size > static_cast<uint32_t>(string_limit_)) {
    TProtocolException::throwExceededSizeLimit();
  }
  decoder_.nextBinary(str, size);
}

inline void NimbleProtocolReader::skip(TType type) {
  auto fieldChunkHint = ttypeToNimbleFieldChunkHint(type);
  switch (fieldChunkHint) {
    case ONE_CHUNK_TYPE:
      decoder_.nextContentChunk();
      break;
    case TWO_CHUNKS_TYPE:
      for (int i = 0; i < 2; ++i) {
        decoder_.nextContentChunk();
      }
      break;
    case COMPLEX_TYPE:
      // TODO: handle these cases
      throw std::runtime_error("Not implemented yet");
      break;
    default:
      folly::assume_unreachable();
  }
}

inline void NimbleProtocolReader::checkComplexFieldData(
    uint32_t fieldChunk,
    ComplexType complexType) {
  if (UNLIKELY(
          ((fieldChunk >> kFieldChunkHintBits) & 1) != complexType ||
          (fieldChunk & 3) != NimbleFieldChunkHint::COMPLEX_METADATA)) {
    TProtocolException::throwInvalidFieldData();
  }
}

bool NimbleProtocolReader::advanceToNextField(
    int32_t /*currFieldId*/,
    int32_t /*nextFieldId*/,
    TType /*nextFieldType*/,
    StructReadState& state) {
  uint32_t fieldChunk = decoder_.nextFieldChunk();
  if (fieldChunk == 0) {
    state.fieldType = TType::T_STOP;
    return false;
  }

  auto fieldChunkHint = static_cast<NimbleFieldChunkHint>(fieldChunk & 3);
  // sanity check
  if (UNLIKELY(fieldChunkHint == NimbleFieldChunkHint::COMPLEX_METADATA)) {
    // TODO: data is malformed, handle this case
    TProtocolException::throwInvalidFieldData();
  }
  state.fieldChunkHint = fieldChunkHint;
  state.fieldId = fieldChunk >> kFieldChunkHintBits;
  state.fieldType = TType::T_VOID; // just to make fieldType non-zero
  return false;
}

bool NimbleProtocolReader::isCompatibleWithType(
    TType expectedFieldType,
    StructReadState& state) {
  // store the expected TType in case we need to skip
  state.fieldType = expectedFieldType;
  auto expectedFieldChunkHint = ttypeToNimbleFieldChunkHint(expectedFieldType);
  return state.fieldChunkHint == expectedFieldChunkHint;
}
} // namespace thrift
} // namespace apache
