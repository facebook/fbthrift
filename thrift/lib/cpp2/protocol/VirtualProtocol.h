/*
 * Copyright 2014 Facebook, Inc.
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

#ifndef CPP2_PROTOCOL_VIRTUALPROTOCOL_H_
#define CPP2_PROTOCOL_VIRTUALPROTOCOL_H_ 1

#include <folly/FBString.h>
#include <folly/io/Cursor.h>

#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace apache { namespace thrift {

class VirtualReaderBase {
 public:
  VirtualReaderBase() {}
  virtual ~VirtualReaderBase() {}

  virtual ProtocolType protocolType() const = 0;

  virtual void setInput(const folly::IOBuf* buf) = 0;

  virtual uint32_t readMessageBegin(std::string& name,
                                   MessageType& messageType,
                                   int32_t& seqid) = 0;
  virtual uint32_t readMessageEnd() = 0;
  virtual uint32_t readStructBegin(std::string& name) = 0;
  virtual uint32_t readStructEnd() = 0;
  virtual uint32_t readFieldBegin(std::string& name,
                                 TType& fieldType,
                                 int16_t& fieldId) = 0;
  virtual uint32_t readFieldEnd() = 0;
  virtual uint32_t readMapBegin(TType& keyType,
                               TType& valType,
                               uint32_t& size) = 0;
  virtual uint32_t readMapEnd() = 0;
  virtual uint32_t readListBegin(TType& elemType, uint32_t& size) = 0;
  virtual uint32_t readListEnd() = 0;
  virtual uint32_t readSetBegin(TType& elemType, uint32_t& size) = 0;
  virtual uint32_t readSetEnd() = 0;
  virtual uint32_t readBool(bool& value) = 0;
  virtual uint32_t readBool(std::vector<bool>::reference value) = 0;
  virtual uint32_t readByte(int8_t& byte) = 0;
  virtual uint32_t readI16(int16_t& i16) = 0;
  virtual uint32_t readI32(int32_t& i32) = 0;
  virtual uint32_t readI64(int64_t& i64) = 0;
  virtual uint32_t readDouble(double& dub) = 0;
  virtual uint32_t readFloat(float& flt) = 0;
  virtual uint32_t readString(std::string& str) = 0;
  virtual uint32_t readString(folly::fbstring& str) = 0;
  virtual uint32_t readBinary(std::string& str) = 0;
  virtual uint32_t readBinary(folly::fbstring& str) = 0;
  virtual uint32_t readBinary(std::unique_ptr<folly::IOBuf>& str) = 0;
  virtual uint32_t readBinary(folly::IOBuf& str) = 0;
  virtual uint32_t skip(TType type) = 0;
  virtual folly::io::Cursor getCurrentPosition() const = 0;
  virtual uint32_t readFromPositionAndAppend(
    folly::io::Cursor& cursor,
    std::unique_ptr<folly::IOBuf>& ser) = 0;
  virtual bool peekMap() { return false; }
  virtual bool peekSet() { return false; }
  virtual bool peekList() { return false; }
};

class VirtualWriterBase {
 public:
  VirtualWriterBase() {}
  virtual ~VirtualWriterBase() {}

  virtual ProtocolType protocolType() const = 0;

  virtual void setOutput(
      folly::IOBufQueue* queue,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) = 0;

  virtual uint32_t writeMessageBegin(const std::string& name,
                                    MessageType messageType,
                                    int32_t seqid) = 0;
  virtual uint32_t writeMessageEnd() = 0;
  virtual uint32_t writeStructBegin(const char* name) = 0;
  virtual uint32_t writeStructEnd() = 0;
  virtual uint32_t writeFieldBegin(const char* name,
                                  TType fieldType,
                                  int16_t fieldId) = 0;
  virtual uint32_t writeFieldEnd() = 0;
  virtual uint32_t writeFieldStop() = 0;
  virtual uint32_t writeMapBegin(TType keyType,
                                TType valType,
                                uint32_t size) = 0;
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
  virtual uint32_t serializedFieldSize(const char* name,
                                      TType fieldType,
                                      int16_t fieldId) = 0;
  virtual uint32_t serializedStructSize(const char* name) = 0;
  virtual uint32_t serializedSizeMapBegin(TType keyType,
                                         TType valType,
                                         uint32_t size) = 0;
  virtual uint32_t serializedSizeMapEnd() = 0;
  virtual uint32_t serializedSizeListBegin(TType elemType,
                                            uint32_t size) = 0;
  virtual uint32_t serializedSizeListEnd() = 0;
  virtual uint32_t serializedSizeSetBegin(TType elemType,
                                           uint32_t size) = 0;
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

  template <typename ...Args,
            typename std::enable_if<
              std::is_constructible<ProtocolT, Args...>::value,
              bool>::type = false>
  explicit VirtualReader(Args&& ...args)
    : protocol_(std::forward<Args>(args)...) {}
  ~VirtualReader() override {}

  ProtocolType protocolType() const override {
    return protocol_.protocolType();
  }

  void setInput(const folly::IOBuf* buf) override { protocol_.setInput(buf); }

  uint32_t readMessageBegin(std::string& name,
                            MessageType& messageType,
                            int32_t& seqid) override {
    return protocol_.readMessageBegin(name, messageType, seqid);
  }
  uint32_t readMessageEnd() override { return protocol_.readMessageEnd(); }
  uint32_t readStructBegin(std::string& name) override {
    return protocol_.readStructBegin(name);
  }
  uint32_t readStructEnd() override { return protocol_.readStructEnd(); }
  uint32_t readFieldBegin(std::string& name,
                          TType& fieldType,
                          int16_t& fieldId) override {
    return protocol_.readFieldBegin(name, fieldType, fieldId);
  }
  uint32_t readFieldEnd() override { return protocol_.readFieldEnd(); }
  uint32_t readMapBegin(TType& keyType,
                        TType& valType,
                        uint32_t& size) override {
    return protocol_.readMapBegin(keyType, valType, size);
  }
  uint32_t readMapEnd() override { return protocol_.readMapEnd(); }
  uint32_t readListBegin(TType& elemType, uint32_t& size) override {
    return protocol_.readListBegin(elemType, size);
  }
  uint32_t readListEnd() override { return protocol_.readListEnd(); }
  uint32_t readSetBegin(TType& elemType, uint32_t& size) override {
    return protocol_.readSetBegin(elemType, size);
  }
  uint32_t readSetEnd() override { return protocol_.readSetEnd(); }
  uint32_t readBool(bool& value) override { return protocol_.readBool(value); }
  uint32_t readBool(std::vector<bool>::reference value) override {
    return protocol_.readBool(value);
  }
  uint32_t readByte(int8_t& byte) override { return protocol_.readByte(byte); }
  uint32_t readI16(int16_t& i16) override { return protocol_.readI16(i16); }
  uint32_t readI32(int32_t& i32) override { return protocol_.readI32(i32); }
  uint32_t readI64(int64_t& i64) override { return protocol_.readI64(i64); }
  uint32_t readDouble(double& dub) override {
    return protocol_.readDouble(dub);
  }
  uint32_t readFloat(float& flt) override { return protocol_.readFloat(flt); }
  uint32_t readString(std::string& str) override {
    return protocol_.readString(str);
  }
  uint32_t readString(folly::fbstring& str) override {
    return protocol_.readString(str);
  }
  uint32_t readBinary(std::string& str) override {
    return protocol_.readBinary(str);
  }
  uint32_t readBinary(folly::fbstring& str) override {
    return protocol_.readBinary(str);
  }
  uint32_t readBinary(std::unique_ptr<folly::IOBuf>& str) override {
    return protocol_.readBinary(str);
  }
  uint32_t readBinary(folly::IOBuf& str) override {
    return protocol_.readBinary(str);
  }
  uint32_t skip(TType type) override { return protocol_.skip(type); }
  folly::io::Cursor getCurrentPosition() const override {
    return protocol_.getCurrentPosition();
  }
  uint32_t readFromPositionAndAppend(
      folly::io::Cursor& cursor, std::unique_ptr<folly::IOBuf>& ser) override {
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
    void, VirtualReader<typename ProtocolT::ProtocolReader>>::type;

  template <typename ...Args,
            typename std::enable_if<
              std::is_constructible<ProtocolT, Args...>::value,
              bool>::type = false>
  explicit VirtualWriter(Args&& ...args)
    : protocol_(std::forward<Args>(args)...) {}
  ~VirtualWriter() override {}

  ProtocolType protocolType() const override {
    return protocol_.protocolType();
  }

  void setOutput(
      folly::IOBufQueue* queue,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) override {
    protocol_.setOutput(queue, maxGrowth);
  }

  uint32_t writeMessageBegin(const std::string& name,
                             MessageType messageType,
                             int32_t seqid) override {
    return protocol_.writeMessageBegin(name, messageType, seqid);
  }
  uint32_t writeMessageEnd() override { return protocol_.writeMessageEnd(); }
  uint32_t writeStructBegin(const char* name) override {
    return protocol_.writeStructBegin(name);
  }
  uint32_t writeStructEnd() override { return protocol_.writeStructEnd(); }
  uint32_t writeFieldBegin(const char* name,
                           TType fieldType,
                           int16_t fieldId) override {
    return protocol_.writeFieldBegin(name, fieldType, fieldId);
  }
  uint32_t writeFieldEnd() override { return protocol_.writeFieldEnd(); }
  uint32_t writeFieldStop() override { return protocol_.writeFieldStop(); }
  uint32_t writeMapBegin(TType keyType, TType valType, uint32_t size) override {
    return protocol_.writeMapBegin(keyType, valType, size);
  }
  uint32_t writeMapEnd() override { return protocol_.writeMapEnd(); }
  uint32_t writeListBegin(TType elemType, uint32_t size) override {
    return protocol_.writeListBegin(elemType, size);
  }
  uint32_t writeListEnd() override { return protocol_.writeListEnd(); }
  uint32_t writeSetBegin(TType elemType, uint32_t size) override {
    return protocol_.writeSetBegin(elemType, size);
  }
  uint32_t writeSetEnd() override { return protocol_.writeSetEnd(); }
  uint32_t writeBool(bool value) override { return protocol_.writeBool(value); }
  uint32_t writeByte(int8_t byte) override { return protocol_.writeByte(byte); }
  uint32_t writeI16(int16_t i16) override { return protocol_.writeI16(i16); }
  uint32_t writeI32(int32_t i32) override { return protocol_.writeI32(i32); }
  uint32_t writeI64(int64_t i64) override { return protocol_.writeI64(i64); }
  uint32_t writeDouble(double dub) override {
    return protocol_.writeDouble(dub);
  }
  uint32_t writeFloat(float flt) override { return protocol_.writeFloat(flt); }
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
  uint32_t serializedFieldSize(const char* name,
                               TType fieldType,
                               int16_t fieldId) override {
    return protocol_.serializedFieldSize(name, fieldType, fieldId);
  }
  uint32_t serializedStructSize(const char* name) override {
    return protocol_.serializedStructSize(name);
  }
  uint32_t serializedSizeMapBegin(TType keyType,
                                  TType valType,
                                  uint32_t size) override {
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


}} // apache::thrift

#endif
