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

inline uint32_t NimbleProtocolWriter::writeStructBegin(const char* /*name*/) {
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeStructEnd() {
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
  // TODO: set container limit and check size within limit
  // TODO: handle map size greater than 2**24
  // To use the short size encoding, we need the high bit of the resulting
  // chunk to be 0, and so must fit in 31 bits, including the shift for the
  // metadata.
  if (size >= (1U << (31 - kStructyTypeMetadataBits))) {
    throw std::runtime_error("Not implemented yet");
  }

  StructyType structyType = getStructyTypeFromMap(keyType, valType);

  // The lowest two bits of this fieldChunk is a fieldChunkHint (11) meaning
  // that this is a complex type. The third lowest bit (0) denotes that this is
  // a structy type rather than a stringy type. The next 4 bits encode the
  // specific type of the structy field, e.g. map from 1-chunk items to 1-chunk
  // items, map from 2-chunk items to complex items, etc.
  encoder_.encodeFieldChunk(
      (size << kStructyTypeMetadataBits) |
      (structyType << kComplexMetadataBits) |
      (ComplexType::STRUCTY << kFieldChunkHintBits) |
      NimbleFieldChunkHint::COMPLEX_TYPE);
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
  // TODO: handle list size greater than 2**24
  // To use the short size encoding, we need the high bit of the resulting
  // chunk to be 0, and so must fit in 31 bits, including the shift for the
  // metadata.
  if (size >= (1U << (31 - kStructyTypeMetadataBits))) {
    throw std::runtime_error("Not implemented yet");
  }
  StructyType structyType = getStructyTypeFromListOrSet(elemType);
  // The lowest two bits of the fieldChunk is a fieldChunkHint (11) indicating
  // that this is a complex type. The thrid lowest bit (0) denotes a structy
  // type. The next 4 bits encode the specific type of the structy field.
  encoder_.encodeFieldChunk(
      size << kStructyTypeMetadataBits | structyType << kComplexMetadataBits |
      ComplexType::STRUCTY << kFieldChunkHintBits |
      NimbleFieldChunkHint::COMPLEX_TYPE);
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

inline uint32_t NimbleProtocolWriter::writeString(folly::StringPiece str) {
  encode(str);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeBinary(folly::StringPiece str) {
  encode(str);
  return 0;
}

inline uint32_t NimbleProtocolWriter::writeBinary(folly::ByteRange str) {
  encode(str);
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

inline void NimbleProtocolReader::readMapBegin(
    TType& /*keyType*/,
    TType& /*valType*/,
    uint32_t& size) {
  uint32_t nextFieldChunk = decoder_.nextFieldChunk();
  // sanity check
  if (UNLIKELY(
          (nextFieldChunk & 3) != NimbleFieldChunkHint::COMPLEX_TYPE ||
          (nextFieldChunk >> kFieldChunkHintBits & 1) !=
              ComplexType::STRUCTY)) {
    // TODO: handle (skip) malformed data
    throw std::runtime_error("Not implemented yet. Skip malformed data");
  }

  // TODO: the structy type is encoded in the 4-7th lowest bits
  // StructyType structyType =
  //     static_cast<StructyType>(nextFieldChunk >> kComplexMetadataBits & 0xf);

  // TODO: handle map size greater than 2**24
  if (nextFieldChunk & (1U << 31)) {
    throw std::runtime_error("Not implemented yet");
  }
  size = nextFieldChunk >> kStructyTypeMetadataBits;
}

inline void NimbleProtocolReader::readMapEnd() {
  // To consume the fieldChunk 0 which marks the end of the map
  uint32_t fieldChunk = decoder_.nextFieldChunk();
  if (UNLIKELY(fieldChunk != 0)) {
    // TODO: handle malformed data (skip?)
    throw std::runtime_error("Data is malformed");
  }
}

inline void NimbleProtocolReader::readListBegin(
    TType& /*elemType*/,
    uint32_t& size) {
  uint32_t nextFieldChunk = decoder_.nextFieldChunk();
  // sanity check
  if (UNLIKELY(
          (nextFieldChunk & 3) != NimbleFieldChunkHint::COMPLEX_TYPE ||
          (nextFieldChunk >> kFieldChunkHintBits & 1) !=
              ComplexType::STRUCTY)) {
    // TODO: handle (skip) malformated data
    return;
  }

  // TODO: handle map size greater than 2**24
  if (nextFieldChunk & (1U << 31)) {
    throw std::runtime_error("Not implemented yet");
  }
  size = nextFieldChunk >> kStructyTypeMetadataBits;
}

inline void NimbleProtocolReader::readListEnd() {
  // To consume the fieldChunk 0 which marks the end of the list
  uint32_t fieldChunk = decoder_.nextFieldChunk();
  if (UNLIKELY(fieldChunk != 0)) {
    // TODO: handle malformed data (skip?)
    throw std::runtime_error("Data is malformed");
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

template <typename StrType>
inline void NimbleProtocolReader::readString(StrType& str) {
  uint32_t nextFieldChunk = decoder_.nextFieldChunk();
  // sanity check
  if (UNLIKELY(
          (nextFieldChunk & 3) != NimbleFieldChunkHint::COMPLEX_TYPE ||
          (nextFieldChunk >> kFieldChunkHintBits & 1) !=
              ComplexType::STRINGY)) {
    // TODO: skip bad content field
    throw std::runtime_error("Not implemented yet (data is malformed)");
  }

  // TODO: handle string length greater than 2**28
  if (nextFieldChunk & 1U << 31) {
    throw std::runtime_error("Not implemented yet");
  }

  uint32_t size = nextFieldChunk >> kComplexMetadataBits;
  // TODO: the following is a temporary workaround to keep apache::thrift::skip
  // working. Need to implement nimble specific skipping logic.
  str.reserve(size);
  str.clear();
  if (size > 0) {
    auto buf = std::unique_ptr<char[]>(new char[size]);
    decoder_.nextBinary(buf.get(), size);
    str.append(buf.get(), size);
  }
}

template <typename StrType>
inline void NimbleProtocolReader::readBinary(StrType& str) {
  readString(str);
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
    case COMPLEX_METADATA:
      // TODO: handle these cases
      throw std::runtime_error("Not implemented yet");
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
  uint32_t fieldChunk = decoder_.nextFieldChunk();
  if (fieldChunk == 0) {
    state.fieldType = TType::T_STOP;
    return false;
  }

  auto fieldChunkHint = static_cast<NimbleFieldChunkHint>(fieldChunk & 3);
  // sanity check
  if (UNLIKELY(fieldChunkHint == NimbleFieldChunkHint::COMPLEX_TYPE)) {
    // TODO: data is malformed, handle this case
    throw std::runtime_error("Bad data encountered while deserializing");
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
