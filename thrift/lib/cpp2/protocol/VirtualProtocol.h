/*
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
  virtual uint32_t writeString(const std::string& str) = 0;
  virtual uint32_t writeString(const folly::fbstring& str) = 0;
  virtual uint32_t writeBinary(const std::string& str) = 0;
  virtual uint32_t writeBinary(const folly::fbstring& str) = 0;
  virtual uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& str) = 0;

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
  virtual uint32_t serializedSizeString(const std::string& str) = 0;
  virtual uint32_t serializedSizeString(const folly::fbstring& str) = 0;
  virtual uint32_t serializedSizeBinary(const std::string& v) = 0;
  virtual uint32_t serializedSizeBinary(const folly::fbstring& v) = 0;
  virtual uint32_t serializedSizeBinary(
      const std::unique_ptr<folly::IOBuf>& v) = 0;
  virtual uint32_t serializedSizeZCBinary(const std::string& v) = 0;
  virtual uint32_t serializedSizeZCBinary(const folly::fbstring& v) = 0;
  virtual uint32_t serializedSizeZCBinary(
      const std::unique_ptr<folly::IOBuf>& v) = 0;
};

std::unique_ptr<VirtualReaderBase> makeVirtualReader(ProtocolType type);
std::unique_ptr<VirtualWriterBase> makeVirtualWriter(ProtocolType type);

template <class ProtocolT>
class VirtualReader : public VirtualReaderBase {
 public:
  template <typename ...Args,
            typename std::enable_if<
              std::is_constructible<ProtocolT, Args...>::value,
              bool>::type = false>
  explicit VirtualReader(Args&& ...args)
    : protocol_(std::forward<Args>(args)...) {}
  ~VirtualReader() {}

  ProtocolType protocolType() const {
    return protocol_.protocolType();
  }

  void setInput(const folly::IOBuf* buf) {
    protocol_.setInput(buf);
  }

  uint32_t readMessageBegin(std::string& name,
                            MessageType& messageType,
                            int32_t& seqid) {
    return protocol_.readMessageBegin(name, messageType, seqid);
  }
  uint32_t readMessageEnd() {
    return protocol_.readMessageEnd();
  }
  uint32_t readStructBegin(std::string& name) {
    return protocol_.readStructBegin(name);
  }
  uint32_t readStructEnd() {
    return protocol_.readStructEnd();
  }
  uint32_t readFieldBegin(std::string& name,
                          TType& fieldType,
                          int16_t& fieldId) {
    return protocol_.readFieldBegin(name, fieldType, fieldId);
  }
  uint32_t readFieldEnd() {
    return protocol_.readFieldEnd();
  }
  uint32_t readMapBegin(TType& keyType,
                        TType& valType,
                        uint32_t& size) {
    return protocol_.readMapBegin(keyType, valType, size);
  }
  uint32_t readMapEnd() {
    return protocol_.readMapEnd();
  }
  uint32_t readListBegin(TType& elemType, uint32_t& size) {
    return protocol_.readListBegin(elemType, size);
  }
  uint32_t readListEnd() {
    return protocol_.readListEnd();
  }
  uint32_t readSetBegin(TType& elemType, uint32_t& size) {
    return protocol_.readSetBegin(elemType, size);
  }
  uint32_t readSetEnd() {
    return protocol_.readSetEnd();
  }
  uint32_t readBool(bool& value) {
    return protocol_.readBool(value);
  }
  uint32_t readBool(std::vector<bool>::reference value) {
    return protocol_.readBool(value);
  }
  uint32_t readByte(int8_t& byte) {
    return protocol_.readByte(byte);
  }
  uint32_t readI16(int16_t& i16) {
    return protocol_.readI16(i16);
  }
  uint32_t readI32(int32_t& i32) {
    return protocol_.readI32(i32);
  }
  uint32_t readI64(int64_t& i64) {
    return protocol_.readI64(i64);
  }
  uint32_t readDouble(double& dub) {
    return protocol_.readDouble(dub);
  }
  uint32_t readFloat(float& flt) {
    return protocol_.readFloat(flt);
  }
  uint32_t readString(std::string& str) {
    return protocol_.readString(str);
  }
  uint32_t readString(folly::fbstring& str) {
    return protocol_.readString(str);
  }
  uint32_t readBinary(std::string& str) {
    return protocol_.readBinary(str);
  }
  uint32_t readBinary(folly::fbstring& str) {
    return protocol_.readBinary(str);
  }
  uint32_t readBinary(std::unique_ptr<folly::IOBuf>& str) {
    return protocol_.readBinary(str);
  }
  uint32_t readBinary(folly::IOBuf& str) {
    return protocol_.readBinary(str);
  }
  uint32_t skip(TType type) {
    return protocol_.skip(type);
  }
  folly::io::Cursor getCurrentPosition() const {
    return protocol_.getCurrentPosition();
  }
  uint32_t readFromPositionAndAppend(
    folly::io::Cursor& cursor,
    std::unique_ptr<folly::IOBuf>& ser) {
    return protocol_.readFromPositionAndAppend(cursor, ser);
  }

 private:
  ProtocolT protocol_;
};

template <class ProtocolT>
class VirtualWriter : public VirtualWriterBase {
 public:
  template <typename ...Args,
            typename std::enable_if<
              std::is_constructible<ProtocolT, Args...>::value,
              bool>::type = false>
  explicit VirtualWriter(Args&& ...args)
    : protocol_(std::forward<Args>(args)...) {}
  ~VirtualWriter() {}

  ProtocolType protocolType() const {
    return protocol_.protocolType();
  }

  void setOutput(
      folly::IOBufQueue* queue,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) {
    protocol_.setOutput(queue, maxGrowth);
  }

  uint32_t writeMessageBegin(const std::string& name,
                             MessageType messageType,
                             int32_t seqid) {
    return protocol_.writeMessageBegin(name, messageType, seqid);
  }
  uint32_t writeMessageEnd() {
    return protocol_.writeMessageEnd();
  }
  uint32_t writeStructBegin(const char* name) {
    return protocol_.writeStructBegin(name);
  }
  uint32_t writeStructEnd() {
    return protocol_.writeStructEnd();
  }
  uint32_t writeFieldBegin(const char* name,
                           TType fieldType,
                           int16_t fieldId) {
    return protocol_.writeFieldBegin(name, fieldType, fieldId);
  }
  uint32_t writeFieldEnd() {
    return protocol_.writeFieldEnd();
  }
  uint32_t writeFieldStop() {
    return protocol_.writeFieldStop();
  }
  uint32_t writeMapBegin(TType keyType,
                         TType valType,
                         uint32_t size) {
    return protocol_.writeMapBegin(keyType, valType, size);
  }
  uint32_t writeMapEnd() {
    return protocol_.writeMapEnd();
  }
  uint32_t writeListBegin(TType elemType, uint32_t size) {
    return protocol_.writeListBegin(elemType, size);
  }
  uint32_t writeListEnd() {
    return protocol_.writeListEnd();
  }
  uint32_t writeSetBegin(TType elemType, uint32_t size) {
    return protocol_.writeSetBegin(elemType, size);
  }
  uint32_t writeSetEnd() {
    return protocol_.writeSetEnd();
  }
  uint32_t writeBool(bool value) {
    return protocol_.writeBool(value);
  }
  uint32_t writeByte(int8_t byte) {
    return protocol_.writeByte(byte);
  }
  uint32_t writeI16(int16_t i16) {
    return protocol_.writeI16(i16);
  }
  uint32_t writeI32(int32_t i32) {
    return protocol_.writeI32(i32);
  }
  uint32_t writeI64(int64_t i64) {
    return protocol_.writeI64(i64);
  }
  uint32_t writeDouble(double dub) {
    return protocol_.writeDouble(dub);
  }
  uint32_t writeFloat(float flt) {
    return protocol_.writeFloat(flt);
  }
  uint32_t writeString(const std::string& str) {
    return protocol_.writeString(str);
  }
  uint32_t writeString(const folly::fbstring& str) {
    return protocol_.writeString(str);
  }
  uint32_t writeBinary(const std::string& str) {
    return protocol_.writeBinary(str);
  }
  uint32_t writeBinary(const folly::fbstring& str) {
    return protocol_.writeBinary(str);
  }
  uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& str) {
    return protocol_.writeBinary(str);
  }

  uint32_t serializedMessageSize(const std::string& name) {
    return protocol_.serializedMessageSize(name);
  }
  uint32_t serializedFieldSize(const char* name,
                               TType fieldType,
                               int16_t fieldId) {
    return protocol_.serializedFieldSize(name, fieldType, fieldId);
  }
  uint32_t serializedStructSize(const char* name) {
    return protocol_.serializedStructSize(name);
  }
  uint32_t serializedSizeMapBegin(TType keyType,
                                  TType valType,
                                  uint32_t size) {
    return protocol_.serializedSizeMapBegin(keyType, valType, size);
  }
  uint32_t serializedSizeMapEnd() {
    return protocol_.serializedSizeMapEnd();
  }
  uint32_t serializedSizeListBegin(TType elemType,
                                   uint32_t size) {
    return protocol_.serializedSizeListBegin(elemType, size);
  }
  uint32_t serializedSizeListEnd() {
    return protocol_.serializedSizeListEnd();
  }
  uint32_t serializedSizeSetBegin(TType elemType,
                                  uint32_t size) {
    return protocol_.serializedSizeSetBegin(elemType, size);
  }
  uint32_t serializedSizeSetEnd() {
    return protocol_.serializedSizeSetEnd();
  }
  uint32_t serializedSizeStop() {
    return protocol_.serializedSizeStop();
  }
  uint32_t serializedSizeBool(bool value) {
    return protocol_.serializedSizeBool(value);
  }
  uint32_t serializedSizeByte(int8_t value) {
    return protocol_.serializedSizeByte(value);
  }
  uint32_t serializedSizeI16(int16_t value) {
    return protocol_.serializedSizeI16(value);
  }
  uint32_t serializedSizeI32(int32_t value) {
    return protocol_.serializedSizeI32(value);
  }
  uint32_t serializedSizeI64(int64_t value) {
    return protocol_.serializedSizeI64(value);
  }
  uint32_t serializedSizeDouble(double dub) {
    return protocol_.serializedSizeDouble(dub);
  }
  uint32_t serializedSizeFloat(float flt) {
    return protocol_.serializedSizeFloat(flt);
  }
  uint32_t serializedSizeString(const std::string& str) {
    return protocol_.serializedSizeString(str);
  }
  uint32_t serializedSizeString(const folly::fbstring& str) {
    return protocol_.serializedSizeString(str);
  }
  uint32_t serializedSizeBinary(const std::string& v) {
    return protocol_.serializedSizeBinary(v);
  }
  uint32_t serializedSizeBinary(const folly::fbstring& v) {
    return protocol_.serializedSizeBinary(v);
  }
  uint32_t serializedSizeBinary(const std::unique_ptr<folly::IOBuf>& v) {
    return protocol_.serializedSizeBinary(v);
  }
  uint32_t serializedSizeZCBinary(const std::string& v) {
    return protocol_.serializedSizeZCBinary(v);
  }
  uint32_t serializedSizeZCBinary(const folly::fbstring& v) {
    return protocol_.serializedSizeZCBinary(v);
  }
  uint32_t serializedSizeZCBinary(const std::unique_ptr<folly::IOBuf>& v) {
    return protocol_.serializedSizeZCBinary(v);
  }
 private:
  ProtocolT protocol_;
};


}} // apache::thrift

#endif
