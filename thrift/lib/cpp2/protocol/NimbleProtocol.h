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

#include <folly/io/Cursor.h>
#include <thrift/facebook/nimble/Decoder.h>
#include <thrift/facebook/nimble/Encoder.h>
#include <thrift/facebook/nimble/NimbleTypes.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp/util/BitwiseCast.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

namespace apache {
namespace thrift {

using folly::io::Cursor;

class NimbleProtocolReader;

class NimbleProtocolWriter {
 public:
  using ProtocolReader = NimbleProtocolReader;

  uint32_t writeMessageBegin(
      const std::string& name,
      MessageType messageType,
      int32_t seqid);
  uint32_t writeMessageEnd();
  uint32_t writeStructBegin(const char* name);
  uint32_t writeStructEnd();
  uint32_t writeFieldBegin(const char* name, TType fieldType, int16_t fieldId);
  uint32_t writeFieldEnd();
  uint32_t writeFieldStop();
  uint32_t writeMapBegin(TType keyType, TType valType, uint32_t size);
  uint32_t writeMapEnd();
  uint32_t writeListBegin(TType elemType, uint32_t size);
  uint32_t writeListEnd();
  uint32_t writeSetBegin(TType elemType, uint32_t size);
  uint32_t writeSetEnd();
  uint32_t writeBool(bool value);
  uint32_t writeByte(int8_t byte);
  uint32_t writeI16(int16_t i16);
  uint32_t writeI32(int32_t i32);
  uint32_t writeI64(int64_t i64);
  uint32_t writeDouble(double dub);
  uint32_t writeFloat(float flt);
  uint32_t writeString(folly::StringPiece str);
  uint32_t writeBinary(folly::StringPiece str);
  uint32_t writeBinary(folly::ByteRange str);
  uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& str);
  uint32_t writeBinary(const folly::IOBuf& str);
  uint32_t writeSerializedData(const std::unique_ptr<folly::IOBuf>& /*data*/);

  /**
   * Functions that return the serialized size
   */

  uint32_t serializedMessageSize(const std::string& name) const;
  uint32_t
  serializedFieldSize(const char* name, TType fieldType, int16_t fieldId) const;
  uint32_t serializedStructSize(const char* name) const;
  uint32_t serializedSizeMapBegin(TType keyType, TType valType, uint32_t size)
      const;
  uint32_t serializedSizeMapEnd() const;
  uint32_t serializedSizeListBegin(TType elemType, uint32_t size) const;
  uint32_t serializedSizeListEnd() const;
  uint32_t serializedSizeSetBegin(TType elemType, uint32_t size) const;
  uint32_t serializedSizeSetEnd() const;
  uint32_t serializedSizeStop() const;
  uint32_t serializedSizeBool(bool = false) const;
  uint32_t serializedSizeByte(int8_t = 0) const;
  uint32_t serializedSizeI16(int16_t = 0) const;
  uint32_t serializedSizeI32(int32_t = 0) const;
  uint32_t serializedSizeI64(int64_t = 0) const;
  uint32_t serializedSizeDouble(double = 0.0) const;
  uint32_t serializedSizeFloat(float = 0) const;
  uint32_t serializedSizeString(folly::StringPiece str) const;
  uint32_t serializedSizeBinary(folly::StringPiece str) const;
  uint32_t serializedSizeBinary(folly::ByteRange v) const;
  uint32_t serializedSizeBinary(std::unique_ptr<folly::IOBuf> const& v) const;
  uint32_t serializedSizeBinary(folly::IOBuf const& v) const;
  uint32_t serializedSizeZCBinary(folly::StringPiece str) const;
  uint32_t serializedSizeZCBinary(folly::ByteRange v) const;
  uint32_t serializedSizeZCBinary(
      std::unique_ptr<folly::IOBuf> const& /*v*/) const;
  uint32_t serializedSizeZCBinary(folly::IOBuf const& /*v*/) const;
  uint32_t serializedSizeSerializedData(
      std::unique_ptr<folly::IOBuf> const& /*data*/) const;

  std::unique_ptr<folly::IOBuf> finalize() {
    return encoder_.finalize();
  }

 private:
  /*
   * The encoder that manipulates the underlying field and content streams.
   */
  detail::Encoder encoder_;

  void encode(bool input);
  void encode(int8_t input);
  void encode(int16_t input);
  void encode(int32_t input);
  void encode(int64_t input);
  void encode(uint8_t input);
  void encode(uint16_t input);
  void encode(uint32_t input);
  void encode(uint64_t input);
  void encode(double input);
  void encode(float input);
  void encode(folly::StringPiece input);
  void encode(folly::ByteRange input);
};

class NimbleProtocolReader {
 public:
  using ProtocolWriter = NimbleProtocolWriter;

  explicit NimbleProtocolReader(std::unique_ptr<folly::IOBuf> buf)
      : decoder_(std::move(buf)) {}

  static constexpr bool kUsesFieldNames() {
    return false;
  }

  static constexpr bool kOmitsContainerSizes() {
    return false;
  }

  static constexpr bool kOmitsContainerElemTypes() {
    return true;
  }

  /**
   * Reading functions
   */
  void
  readMessageBegin(std::string& name, MessageType& messageType, int32_t& seqid);
  void readMessageEnd();
  void readStructBegin(std::string& name);
  void readStructEnd();
  void readFieldBegin(std::string& name, TType& fieldType, int16_t& fieldId);
  void readFieldEnd();
  void readMapBegin(TType& keyType, TType& valType, uint32_t& size);
  void readMapEnd();
  void readListBegin(TType& elemType, uint32_t& size);
  void readListEnd();
  void readSetBegin(TType& elemType, uint32_t& size);
  void readSetEnd();
  void readBool(bool& value);
  void readBool(std::vector<bool>::reference value);
  void readByte(int8_t& byte);
  void readI16(int16_t& i16);
  void readI32(int32_t& i32);
  void readI64(int64_t& i64);
  void readDouble(double& dub);
  void readFloat(float& flt);
  template <typename StrType>
  void readString(StrType& str);
  template <typename StrType>
  void readBinary(StrType& str);
  void readBinary(std::unique_ptr<folly::IOBuf>& str);
  void readBinary(folly::IOBuf& str);
  void skip(TType type);
  bool peekMap() {
    return false;
  }
  bool peekSet() {
    return false;
  }
  bool peekList() {
    return false;
  }

  const Cursor& getCursor() const {
    throw std::logic_error(
        "NimbleProtocolReader doesn't expose the underlying cursor.");
  }

  size_t getCursorPosition() const {
    return 0;
  }

  void decode(bool& value);
  void decode(int8_t& value);
  void decode(int16_t& value);
  void decode(int32_t& value);
  void decode(int64_t& value);
  void decode(uint8_t& value);
  void decode(uint16_t& value);
  void decode(uint32_t& value);
  void decode(uint64_t& value);
  void decode(double& value);
  void decode(float& value);

  struct StructReadState {
    int16_t fieldId;
    apache::thrift::protocol::TType fieldType;
    detail::nimble::NimbleFieldChunkHint fieldChunkHint;

    void readStructBegin(NimbleProtocolReader* /*iprot*/) {}

    void readStructEnd(NimbleProtocolReader* /*iprot*/) {}

    void readFieldBegin(NimbleProtocolReader* /*iprot*/) {}

    FOLLY_NOINLINE void readFieldBeginNoInline(
        NimbleProtocolReader* /*iprot*/) {}

    void readFieldEnd(NimbleProtocolReader* /*iprot*/) {}

    FOLLY_ALWAYS_INLINE bool advanceToNextField(
        NimbleProtocolReader* iprot,
        int32_t currFieldId,
        int32_t nextFieldId,
        TType nextFieldType) {
      return iprot->advanceToNextField(
          currFieldId, nextFieldId, nextFieldType, *this);
    }

    FOLLY_ALWAYS_INLINE bool isCompatibleWithType(
        NimbleProtocolReader* iprot,
        TType expectedFieldType) {
      return iprot->isCompatibleWithType(expectedFieldType, *this);
    }

    std::string& fieldName() {
      throw std::logic_error(
          "NimbleProtocolReader doesn't support field names");
    }
  };

 protected:
  FOLLY_ALWAYS_INLINE bool advanceToNextField(
      int32_t currFieldId,
      int32_t nextFieldId,
      TType type,
      StructReadState& state);

  FOLLY_ALWAYS_INLINE bool isCompatibleWithType(
      TType expectedFieldType,
      StructReadState& state);

 private:
  detail::Decoder decoder_;
};

namespace detail {

template <class Protocol>
struct ProtocolReaderStructReadState;

template <>
struct ProtocolReaderStructReadState<NimbleProtocolReader>
    : NimbleProtocolReader::StructReadState {};

} // namespace detail
} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/protocol/NimbleProtocol.tcc>
