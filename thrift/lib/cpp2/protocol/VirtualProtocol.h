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

#ifndef CPP2_PROTOCOL_VIRTUALPROTOCOL_H_
#define CPP2_PROTOCOL_VIRTUALPROTOCOL_H_ 1

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <folly/FBString.h>
#include <folly/io/Cursor.h>
#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

namespace apache {
namespace thrift {

class VirtualReaderBase {
 public:
  VirtualReaderBase() {}
  virtual ~VirtualReaderBase() {}

  virtual ProtocolType protocolType() const = 0;
  virtual bool kUsesFieldNames() const = 0;
  virtual bool kOmitsContainerSizes() const = 0;

  virtual void setInput(const folly::io::Cursor& cursor) = 0;
  void setInput(const folly::IOBuf* buf) {
    setInput(folly::io::Cursor(buf));
  }

  virtual void readMessageBegin(
      std::string& name,
      MessageType& messageType,
      int32_t& seqid) = 0;
  virtual void readMessageEnd() = 0;
  virtual void readStructBegin(std::string& name) = 0;
  virtual void readStructEnd() = 0;
  virtual void
  readFieldBegin(std::string& name, TType& fieldType, int16_t& fieldId) = 0;
  virtual void readFieldEnd() = 0;
  virtual void readMapBegin(TType& keyType, TType& valType, uint32_t& size) = 0;
  virtual void readMapEnd() = 0;
  virtual void readListBegin(TType& elemType, uint32_t& size) = 0;
  virtual void readListEnd() = 0;
  virtual void readSetBegin(TType& elemType, uint32_t& size) = 0;
  virtual void readSetEnd() = 0;
  virtual void readBool(bool& value) = 0;
  virtual void readBool(std::vector<bool>::reference value) = 0;
  virtual void readByte(int8_t& byte) = 0;
  virtual void readI16(int16_t& i16) = 0;
  virtual void readI32(int32_t& i32) = 0;
  virtual void readI64(int64_t& i64) = 0;
  virtual void readDouble(double& dub) = 0;
  virtual void readFloat(float& flt) = 0;
  virtual void readString(std::string& str) = 0;
  virtual void readString(folly::fbstring& str) = 0;
  virtual void readBinary(std::string& str) = 0;
  virtual void readBinary(folly::fbstring& str) = 0;
  virtual void readBinary(detail::SkipNoopString& str) = 0;
  virtual void readBinary(std::unique_ptr<folly::IOBuf>& str) = 0;
  virtual void readBinary(folly::IOBuf& str) = 0;
  virtual void skip(TType type) = 0;
  virtual const folly::io::Cursor& getCursor() const = 0;
  virtual size_t getCursorPosition() const = 0;
  virtual uint32_t readFromPositionAndAppend(
      folly::io::Cursor& cursor,
      std::unique_ptr<folly::IOBuf>& ser) = 0;
  virtual bool peekMap() {
    return false;
  }
  virtual bool peekSet() {
    return false;
  }
  virtual bool peekList() {
    return false;
  }
};

class VirtualWriterBase {
 public:
  VirtualWriterBase() {}
  virtual ~VirtualWriterBase() {}

  virtual ProtocolType protocolType() const = 0;

  virtual bool kSortKeys() const = 0;

  virtual void setOutput(
      folly::IOBufQueue* queue,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) = 0;

  virtual void setOutput(folly::io::QueueAppender&& output) = 0;

  virtual uint32_t writeMessageBegin(
      const std::string& name,
      MessageType messageType,
      int32_t seqid) = 0;
  virtual uint32_t writeMessageEnd() = 0;
  virtual uint32_t writeStructBegin(const char* name) = 0;
  virtual uint32_t writeStructEnd() = 0;
  virtual uint32_t
  writeFieldBegin(const char* name, TType fieldType, int16_t fieldId) = 0;
  virtual uint32_t writeFieldEnd() = 0;
  virtual uint32_t writeFieldStop() = 0;
  virtual uint32_t
  writeMapBegin(TType keyType, TType valType, uint32_t size) = 0;
  virtual uint32_t writeMapEnd() = 0;
  virtual uint32_t writeListBegin(TType elemType, uint32_t size) = 0;
  virtual uint32_t writeListEnd() = 0;
  virtual uint32_t writeSetBegin(TType elemType, uint32_t size) = 0;
  virtual uint32_t writeSetEnd() = 0;
  virtual uint32_t writeBool(bool value) = 0;
  virtual uint32_t writeByte(int8_t byte) = 0;
  virtual uint32_t writeI16(int16_t i16) = 0;
  virtual uint32_t writeI32(int32_t i32) = 0;
  virtual uint32_t writeI64(int64_t i64) = 0;
  virtual uint32_t writeDouble(double dub) = 0;
  virtual uint32_t writeFloat(float flt) = 0;
  virtual uint32_t writeString(folly::StringPiece str) = 0;
  virtual uint32_t writeBinary(folly::StringPiece str) = 0;
  virtual uint32_t writeBinary(folly::ByteRange v) = 0;
  virtual uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& str) = 0;
  virtual uint32_t writeBinary(const folly::IOBuf& str) = 0;

  virtual uint32_t serializedMessageSize(const std::string& name) = 0;
  virtual uint32_t
  serializedFieldSize(const char* name, TType fieldType, int16_t fieldId) = 0;
  virtual uint32_t serializedStructSize(const char* name) = 0;
  virtual uint32_t
  serializedSizeMapBegin(TType keyType, TType valType, uint32_t size) = 0;
  virtual uint32_t serializedSizeMapEnd() = 0;
  virtual uint32_t serializedSizeListBegin(TType elemType, uint32_t size) = 0;
  virtual uint32_t serializedSizeListEnd() = 0;
  virtual uint32_t serializedSizeSetBegin(TType elemType, uint32_t size) = 0;
  virtual uint32_t serializedSizeSetEnd() = 0;
  virtual uint32_t serializedSizeStop() = 0;
  virtual uint32_t serializedSizeBool(bool value) = 0;
  virtual uint32_t serializedSizeByte(int8_t value) = 0;
  virtual uint32_t serializedSizeI16(int16_t value) = 0;
  virtual uint32_t serializedSizeI32(int32_t value) = 0;
  virtual uint32_t serializedSizeI64(int64_t value) = 0;
  virtual uint32_t serializedSizeDouble(double value) = 0;
  virtual uint32_t serializedSizeFloat(float value) = 0;
  virtual uint32_t serializedSizeString(folly::StringPiece str) = 0;
  virtual uint32_t serializedSizeBinary(folly::StringPiece str) = 0;
  virtual uint32_t serializedSizeBinary(folly::ByteRange v) = 0;
  virtual uint32_t serializedSizeBinary(
      const std::unique_ptr<folly::IOBuf>& v) = 0;
  virtual uint32_t serializedSizeBinary(const folly::IOBuf& v) = 0;
  virtual uint32_t serializedSizeZCBinary(folly::StringPiece v) = 0;
  virtual uint32_t serializedSizeZCBinary(folly::ByteRange v) = 0;
  virtual uint32_t serializedSizeZCBinary(
      const std::unique_ptr<folly::IOBuf>& v) = 0;
  virtual uint32_t serializedSizeZCBinary(const folly::IOBuf& v) = 0;
};

std::unique_ptr<VirtualReaderBase> makeVirtualReader(ProtocolType type);
std::unique_ptr<VirtualWriterBase> makeVirtualWriter(ProtocolType type);

template <class ProtocolT>
class VirtualWriter;

template <class ProtocolT>
class VirtualReader : public VirtualReaderBase {
 public:
  using ProtocolWriter = VirtualWriter<typename ProtocolT::ProtocolWriter>;

  template <
      typename... Args,
      typename std::enable_if<
          std::is_constructible<ProtocolT, Args...>::value,
          bool>::type = false>
  explicit VirtualReader(Args&&... args)
      : protocol_(std::forward<Args>(args)...) {}
  ~VirtualReader() override {}

  ProtocolType protocolType() const override {
    return protocol_.protocolType();
  }

  bool kUsesFieldNames() const override {
    return protocol_.kUsesFieldNames();
  }

  virtual bool kOmitsContainerSizes() const override {
    return protocol_.kOmitsContainerSizes();
  }

  using VirtualReaderBase::setInput;
  void setInput(const folly::io::Cursor& cursor) override {
    protocol_.setInput(cursor);
  }

  void readMessageBegin(
      std::string& name,
      MessageType& messageType,
      int32_t& seqid) override {
    protocol_.readMessageBegin(name, messageType, seqid);
  }
  void readMessageEnd() override {
    protocol_.readMessageEnd();
  }
  void readStructBegin(std::string& name) override {
    protocol_.readStructBegin(name);
  }
  void readStructEnd() override {
    protocol_.readStructEnd();
  }
  void readFieldBegin(std::string& name, TType& fieldType, int16_t& fieldId)
      override {
    protocol_.readFieldBegin(name, fieldType, fieldId);
  }
  void readFieldEnd() override {
    protocol_.readFieldEnd();
  }
  void readMapBegin(TType& keyType, TType& valType, uint32_t& size) override {
    protocol_.readMapBegin(keyType, valType, size);
  }
  void readMapEnd() override {
    protocol_.readMapEnd();
  }
  void readListBegin(TType& elemType, uint32_t& size) override {
    protocol_.readListBegin(elemType, size);
  }
  void readListEnd() override {
    protocol_.readListEnd();
  }
  void readSetBegin(TType& elemType, uint32_t& size) override {
    protocol_.readSetBegin(elemType, size);
  }
  void readSetEnd() override {
    protocol_.readSetEnd();
  }
  void readBool(bool& value) override {
    protocol_.readBool(value);
  }
  void readBool(std::vector<bool>::reference value) override {
    protocol_.readBool(value);
  }
  void readByte(int8_t& byte) override {
    protocol_.readByte(byte);
  }
  void readI16(int16_t& i16) override {
    protocol_.readI16(i16);
  }
  void readI32(int32_t& i32) override {
    protocol_.readI32(i32);
  }
  void readI64(int64_t& i64) override {
    protocol_.readI64(i64);
  }
  void readDouble(double& dub) override {
    protocol_.readDouble(dub);
  }
  void readFloat(float& flt) override {
    protocol_.readFloat(flt);
  }
  void readString(std::string& str) override {
    protocol_.readString(str);
  }
  void readString(folly::fbstring& str) override {
    protocol_.readString(str);
  }
  void readBinary(std::string& str) override {
    protocol_.readBinary(str);
  }
  void readBinary(folly::fbstring& str) override {
    protocol_.readBinary(str);
  }
  virtual void readBinary(detail::SkipNoopString& str) override {
    protocol_.readBinary(str);
  }
  void readBinary(std::unique_ptr<folly::IOBuf>& str) override {
    protocol_.readBinary(str);
  }
  void readBinary(folly::IOBuf& str) override {
    protocol_.readBinary(str);
  }
  void skip(TType type) override {
    protocol_.skip(type);
  }
  const folly::io::Cursor& getCursor() const override {
    return protocol_.getCursor();
  }
  size_t getCursorPosition() const override {
    return protocol_.getCursorPosition();
  }
  uint32_t readFromPositionAndAppend(
      folly::io::Cursor& cursor,
      std::unique_ptr<folly::IOBuf>& ser) override {
    return protocol_.readFromPositionAndAppend(cursor, ser);
  }

 protected:
  ProtocolT protocol_;
};

template <class ProtocolT>
class VirtualWriter : public VirtualWriterBase {
 public:
  using ProtocolReader = typename std::conditional<
      std::is_same<void, typename ProtocolT::ProtocolReader>::value,
      void,
      VirtualReader<typename ProtocolT::ProtocolReader>>::type;

  template <
      typename... Args,
      typename std::enable_if<
          std::is_constructible<ProtocolT, Args...>::value,
          bool>::type = false>
  explicit VirtualWriter(Args&&... args)
      : protocol_(std::forward<Args>(args)...) {}
  ~VirtualWriter() override {}

  ProtocolType protocolType() const override {
    return protocol_.protocolType();
  }

  bool kSortKeys() const override {
    return protocol_.kSortKeys();
  }

  void setOutput(
      folly::IOBufQueue* queue,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) override {
    protocol_.setOutput(queue, maxGrowth);
  }

  void setOutput(folly::io::QueueAppender&& output) override {
    protocol_.setOutput(std::move(output));
  }

  uint32_t writeMessageBegin(
      const std::string& name,
      MessageType messageType,
      int32_t seqid) override {
    return protocol_.writeMessageBegin(name, messageType, seqid);
  }
  uint32_t writeMessageEnd() override {
    return protocol_.writeMessageEnd();
  }
  uint32_t writeStructBegin(const char* name) override {
    return protocol_.writeStructBegin(name);
  }
  uint32_t writeStructEnd() override {
    return protocol_.writeStructEnd();
  }
  uint32_t writeFieldBegin(const char* name, TType fieldType, int16_t fieldId)
      override {
    return protocol_.writeFieldBegin(name, fieldType, fieldId);
  }
  uint32_t writeFieldEnd() override {
    return protocol_.writeFieldEnd();
  }
  uint32_t writeFieldStop() override {
    return protocol_.writeFieldStop();
  }
  uint32_t writeMapBegin(TType keyType, TType valType, uint32_t size) override {
    return protocol_.writeMapBegin(keyType, valType, size);
  }
  uint32_t writeMapEnd() override {
    return protocol_.writeMapEnd();
  }
  uint32_t writeListBegin(TType elemType, uint32_t size) override {
    return protocol_.writeListBegin(elemType, size);
  }
  uint32_t writeListEnd() override {
    return protocol_.writeListEnd();
  }
  uint32_t writeSetBegin(TType elemType, uint32_t size) override {
    return protocol_.writeSetBegin(elemType, size);
  }
  uint32_t writeSetEnd() override {
    return protocol_.writeSetEnd();
  }
  uint32_t writeBool(bool value) override {
    return protocol_.writeBool(value);
  }
  uint32_t writeByte(int8_t byte) override {
    return protocol_.writeByte(byte);
  }
  uint32_t writeI16(int16_t i16) override {
    return protocol_.writeI16(i16);
  }
  uint32_t writeI32(int32_t i32) override {
    return protocol_.writeI32(i32);
  }
  uint32_t writeI64(int64_t i64) override {
    return protocol_.writeI64(i64);
  }
  uint32_t writeDouble(double dub) override {
    return protocol_.writeDouble(dub);
  }
  uint32_t writeFloat(float flt) override {
    return protocol_.writeFloat(flt);
  }
  uint32_t writeString(folly::StringPiece str) override {
    return protocol_.writeString(str);
  }
  uint32_t writeBinary(folly::StringPiece str) override {
    return protocol_.writeBinary(str);
  }
  uint32_t writeBinary(folly::ByteRange v) override {
    return protocol_.writeBinary(v);
  }
  uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& str) override {
    return protocol_.writeBinary(str);
  }
  uint32_t writeBinary(const folly::IOBuf& str) override {
    return protocol_.writeBinary(str);
  }

  uint32_t serializedMessageSize(const std::string& name) override {
    return protocol_.serializedMessageSize(name);
  }
  uint32_t serializedFieldSize(
      const char* name,
      TType fieldType,
      int16_t fieldId) override {
    return protocol_.serializedFieldSize(name, fieldType, fieldId);
  }
  uint32_t serializedStructSize(const char* name) override {
    return protocol_.serializedStructSize(name);
  }
  uint32_t serializedSizeMapBegin(TType keyType, TType valType, uint32_t size)
      override {
    return protocol_.serializedSizeMapBegin(keyType, valType, size);
  }
  uint32_t serializedSizeMapEnd() override {
    return protocol_.serializedSizeMapEnd();
  }
  uint32_t serializedSizeListBegin(TType elemType, uint32_t size) override {
    return protocol_.serializedSizeListBegin(elemType, size);
  }
  uint32_t serializedSizeListEnd() override {
    return protocol_.serializedSizeListEnd();
  }
  uint32_t serializedSizeSetBegin(TType elemType, uint32_t size) override {
    return protocol_.serializedSizeSetBegin(elemType, size);
  }
  uint32_t serializedSizeSetEnd() override {
    return protocol_.serializedSizeSetEnd();
  }
  uint32_t serializedSizeStop() override {
    return protocol_.serializedSizeStop();
  }
  uint32_t serializedSizeBool(bool value) override {
    return protocol_.serializedSizeBool(value);
  }
  uint32_t serializedSizeByte(int8_t value) override {
    return protocol_.serializedSizeByte(value);
  }
  uint32_t serializedSizeI16(int16_t value) override {
    return protocol_.serializedSizeI16(value);
  }
  uint32_t serializedSizeI32(int32_t value) override {
    return protocol_.serializedSizeI32(value);
  }
  uint32_t serializedSizeI64(int64_t value) override {
    return protocol_.serializedSizeI64(value);
  }
  uint32_t serializedSizeDouble(double dub) override {
    return protocol_.serializedSizeDouble(dub);
  }
  uint32_t serializedSizeFloat(float flt) override {
    return protocol_.serializedSizeFloat(flt);
  }
  uint32_t serializedSizeString(folly::StringPiece str) override {
    return protocol_.serializedSizeString(str);
  }
  uint32_t serializedSizeBinary(folly::StringPiece str) override {
    return protocol_.serializedSizeBinary(str);
  }
  uint32_t serializedSizeBinary(folly::ByteRange v) override {
    return protocol_.serializedSizeBinary(v);
  }
  uint32_t serializedSizeBinary(
      const std::unique_ptr<folly::IOBuf>& v) override {
    return protocol_.serializedSizeBinary(v);
  }
  uint32_t serializedSizeBinary(const folly::IOBuf& v) override {
    return protocol_.serializedSizeBinary(v);
  }
  uint32_t serializedSizeZCBinary(folly::StringPiece v) override {
    return protocol_.serializedSizeZCBinary(v);
  }
  uint32_t serializedSizeZCBinary(folly::ByteRange v) override {
    return protocol_.serializedSizeZCBinary(v);
  }
  uint32_t serializedSizeZCBinary(
      const std::unique_ptr<folly::IOBuf>& v) override {
    return protocol_.serializedSizeZCBinary(v);
  }
  uint32_t serializedSizeZCBinary(const folly::IOBuf& v) override {
    return protocol_.serializedSizeZCBinary(v);
  }

 private:
  ProtocolT protocol_;
};

} // namespace thrift
} // namespace apache

#endif
