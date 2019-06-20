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
#include <folly/portability/GFlags.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp/util/BitwiseCast.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/protocol/nimble/Decoder.h>
#include <thrift/lib/cpp2/protocol/nimble/Encoder.h>
#include <thrift/lib/cpp2/protocol/nimble/NimbleTypes.h>

DECLARE_int32(thrift_cpp2_protocol_reader_string_limit);
DECLARE_int32(thrift_cpp2_protocol_reader_container_limit);

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
  uint32_t writeCollectionBegin(TType elemType, uint32_t size);
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
  void encodeComplexTypeMetadata(
      uint32_t size,
      detail::nimble::StructyType structyType =
          detail::nimble::StructyType::NONE);

  /*
   * The encoder that manipulates the underlying field and content streams.
   */
  detail::Encoder encoder_;
};

class NimbleProtocolReader {
 public:
  using ProtocolWriter = NimbleProtocolWriter;

  explicit NimbleProtocolReader(
      int32_t string_limit = FLAGS_thrift_cpp2_protocol_reader_string_limit,
      int32_t container_limit =
          FLAGS_thrift_cpp2_protocol_reader_container_limit)
      : string_limit_(string_limit), container_limit_(container_limit) {
    if (string_limit_ <= 0) {
      string_limit_ = INT32_MAX;
    }
    if (container_limit_ <= 0) {
      container_limit_ = INT32_MAX;
    }
  }

  static constexpr bool kUsesFieldNames() {
    return false;
  }

  static constexpr bool kOmitsContainerSizes() {
    return false;
  }

  static constexpr bool kOmitsContainerElemTypes() {
    return true;
  }

  void setInput(const folly::io::Cursor& cursor) {
    decoder_.setInput(cursor);
  }

  /**
   * Reading functions
   */
  void
  readMessageBegin(std::string& name, MessageType& messageType, int32_t& seqid);
  void readMessageEnd();
  void readStructBegin(const std::string& name);
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
  uint32_t readComplexTypeSize(detail::nimble::ComplexType complexType);
  void skip(TType /*type*/) {}
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

  struct StructReadState {
    int16_t fieldId;
    apache::thrift::protocol::TType fieldType;
    detail::nimble::NimbleFieldChunkHint fieldChunkHint;
    uint32_t fieldChunk;

    void readStructBegin(NimbleProtocolReader* iprot) {
      iprot->readStructBegin("");
    }

    void readStructEnd(NimbleProtocolReader* /*iprot*/) {}

    void readFieldBegin(NimbleProtocolReader* iprot) {
      // Right now, advanceToNextField parses its field chunk on the case where
      // it fails to match; this is the only place that parsing occurs. So we
      // call it with dummy data in order to initialize those fields. There's an
      // edge case where the parsing succeeds; in that case we fill in the data
      // we care about manually.
      //
      // This is probably not the right division of concerns; the match-failure
      // path should just store the field chunk into the read state, and let its
      // caller (who is on a slow, non-inlined path) deal with the parsing
      // logic. That's somewhat tricky to change though; in the short term, this
      // works.
      bool hitStop = iprot->advanceToNextField(0, 0, TType::T_STOP, *this);
      if (hitStop) {
        this->fieldType = TType::T_STOP;
      }
    }

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

    inline void skip(NimbleProtocolReader* iprot) {
      iprot->skip(*this);
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
  /* The actual method that does skip when there is a mismatch */
  void skip(StructReadState& state);

 private:
  void checkComplexFieldData(
      uint32_t fieldChunk,
      detail::nimble::ComplexType complexType);

  detail::Decoder decoder_;
  int32_t string_limit_;
  int32_t container_limit_;
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
