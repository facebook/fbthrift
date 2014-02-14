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

#ifndef CPP2_PROTOCOL_TBINARYPROTOCOL_H_
#define CPP2_PROTOCOL_TBINARYPROTOCOL_H_ 1

#include "folly/io/IOBuf.h"
#include "folly/io/IOBufQueue.h"
#include "folly/io/Cursor.h"
#include "thrift/lib/cpp/protocol/TProtocol.h"
#include "thrift/lib/cpp2/protocol/Protocol.h"

using apache::thrift::protocol::TType;

namespace apache { namespace thrift {

using folly::IOBuf;
using folly::IOBufQueue;

typedef folly::io::RWPrivateCursor RWCursor;
using folly::io::Cursor;
using folly::io::QueueAppender;

/**
 * The default binary protocol for thrift. Writes all data in a very basic
 * binary format, essentially just spitting out the raw bytes.
 *
 */
class BinaryProtocolWriter {

 public:
  static const int32_t VERSION_1 = 0x80010000;

  BinaryProtocolWriter()
      : out_(NULL, 0) {}

  static inline ProtocolType protocolType() {
    return ProtocolType::T_BINARY_PROTOCOL;
  }

  /**
   * ...
   * The IOBuf itself is managed by the caller.
   * It must exist for the life of the BinaryProtocol as well,
   * or until the output is reset with setOutput/Input(NULL), or
   * set to some other buffer.
   */
  inline void setOutput(
      IOBufQueue* queue,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) {
    // Allocate 1MB at a time; leave some room for the IOBuf overhead
    constexpr size_t kDesiredGrowth = (1 << 20) - 64;
    out_.reset(queue, std::min(maxGrowth, kDesiredGrowth));
  }

  inline uint32_t writeMessageBegin(const std::string& name,
                                    MessageType messageType,
                                    int32_t seqid);
  inline uint32_t writeMessageEnd();
  inline uint32_t writeStructBegin(const char* name);
  inline uint32_t writeStructEnd();
  inline uint32_t writeFieldBegin(const char* name,
                                  TType fieldType,
                                  int16_t fieldId);
  inline uint32_t writeFieldEnd();
  inline uint32_t writeFieldStop();
  inline uint32_t writeMapBegin(TType keyType,
                                TType valType,
                                uint32_t size);
  inline uint32_t writeMapEnd();
  inline uint32_t writeListBegin(TType elemType, uint32_t size);
  inline uint32_t writeListEnd();
  inline uint32_t writeSetBegin(TType elemType, uint32_t size);
  inline uint32_t writeSetEnd();
  inline uint32_t writeBool(bool value);
  inline uint32_t writeByte(int8_t byte);
  inline uint32_t writeI16(int16_t i16);
  inline uint32_t writeI32(int32_t i32);
  inline uint32_t writeI64(int64_t i64);
  inline uint32_t writeDouble(double dub);
  inline uint32_t writeFloat(float flt);
  template <typename StrType>
  inline uint32_t writeString(const StrType& str);
  template <typename StrType>
  inline uint32_t writeBinary(const StrType& str);
  inline uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& str);
  inline uint32_t writeBinary(const folly::IOBuf& str);
  inline uint32_t writeSerializedData(
      const std::unique_ptr<folly::IOBuf>& data);

  /**
   * Functions that return the serialized size
   */

  inline uint32_t serializedMessageSize(const std::string& name);
  inline uint32_t serializedFieldSize(const char* name,
                                      TType fieldType,
                                      int16_t fieldId);
  inline uint32_t serializedStructSize(const char* name);
  inline uint32_t serializedSizeMapBegin(TType keyType,
                                         TType valType,
                                         uint32_t size);
  inline uint32_t serializedSizeMapEnd();
  inline uint32_t serializedSizeListBegin(TType elemType,
                                            uint32_t size);
  inline uint32_t serializedSizeListEnd();
  inline uint32_t serializedSizeSetBegin(TType elemType,
                                           uint32_t size);
  inline uint32_t serializedSizeSetEnd();
  inline uint32_t serializedSizeStop();
  inline uint32_t serializedSizeBool(bool = false);
  inline uint32_t serializedSizeByte(int8_t = 0);
  inline uint32_t serializedSizeI16(int16_t = 0);
  inline uint32_t serializedSizeI32(int32_t = 0);
  inline uint32_t serializedSizeI64(int64_t = 0);
  inline uint32_t serializedSizeDouble(double = 0.0);
  inline uint32_t serializedSizeFloat(float = 0);
  template <typename StrType>
  inline uint32_t serializedSizeString(const StrType&);
  template <typename StrType>
  uint32_t serializedSizeBinary(const StrType& v) {
    return serializedSizeString(v);
  }
  inline uint32_t serializedSizeBinary(const std::unique_ptr<folly::IOBuf>& v);
  inline uint32_t serializedSizeBinary(const folly::IOBuf& v);
  template <typename StrType>
  uint32_t serializedSizeZCBinary(const StrType& v) {
    return serializedSizeBinary(v);
  }
  uint32_t serializedSizeZCBinary(const std::unique_ptr<folly::IOBuf>& v) {
    // size only
    return serializedSizeI32();
  }
  uint32_t serializedSizeZCBinary(const folly::IOBuf& v) {
    // size only
    return serializedSizeI32();
  }
  inline uint32_t serializedSizeSerializedData(
      const std::unique_ptr<folly::IOBuf>& data);

 protected:
  /**
   * Cursor to write the data out to.
   */
  QueueAppender out_;
};

class BinaryProtocolReader {

 public:
  static const int32_t VERSION_MASK = 0xffff0000;
  static const int32_t VERSION_1 = 0x80010000;

  BinaryProtocolReader()
    : string_limit_(0)
    , container_limit_(0)
    , strict_read_(true)
    , in_(NULL) {}

  BinaryProtocolReader(int32_t string_limit,
                  int32_t container_limit)
    : string_limit_(string_limit)
    , container_limit_(container_limit)
    , strict_read_(true)
    , in_(NULL) {}

  static inline ProtocolType protocolType() {
    return ProtocolType::T_BINARY_PROTOCOL;
  }

  void setStringSizeLimit(int32_t string_limit) {
    string_limit_ = string_limit;
  }

  void setContainerSizeLimit(int32_t container_limit) {
    container_limit_ = container_limit;
  }

  void setStrict(bool strict_read = true) {
    strict_read_ = strict_read;
  }

  /**
   * The IOBuf itself is managed by the caller.
   * It must exist for the life of the BinaryProtocol as well,
   * or until the output is reset with setOutput/Input(NULL), or
   * set to some other buffer.
   */
  inline void setInput(const IOBuf* buf) {
    in_.reset(buf);
  }

  /**
   * Reading functions
   */
  inline uint32_t readMessageBegin(std::string& name,
                                   MessageType& messageType,
                                   int32_t& seqid);
  inline uint32_t readMessageEnd();
  inline uint32_t readStructBegin(std::string& name);
  inline uint32_t readStructEnd();
  inline uint32_t readFieldBegin(std::string& name,
                                 TType& fieldType,
                                 int16_t& fieldId);
  inline uint32_t readFieldEnd();
  inline uint32_t readMapBegin(TType& keyType,
                               TType& valType,
                               uint32_t& size);
  inline uint32_t readMapEnd();
  inline uint32_t readListBegin(TType& elemType, uint32_t& size);
  inline uint32_t readListEnd();
  inline uint32_t readSetBegin(TType& elemType, uint32_t& size);
  inline uint32_t readSetEnd();
  inline uint32_t readBool(bool& value);
  inline uint32_t readBool(std::vector<bool>::reference value);
  inline uint32_t readByte(int8_t& byte);
  inline uint32_t readI16(int16_t& i16);
  inline uint32_t readI32(int32_t& i32);
  inline uint32_t readI64(int64_t& i64);
  inline uint32_t readDouble(double& dub);
  inline uint32_t readFloat(float& flt);
  template<typename StrType>
  inline uint32_t readString(StrType& str);
  template <typename StrType>
  inline uint32_t readBinary(StrType& str);
  inline uint32_t readBinary(std::unique_ptr<folly::IOBuf>& str);
  inline uint32_t readBinary(folly::IOBuf& str);

  uint32_t skip(TType type) {
    return apache::thrift::skip(*this, type);
  }

  Cursor getCurrentPosition() const {
    return in_;
  }
  inline uint32_t readFromPositionAndAppend(Cursor& cursor,
                                            std::unique_ptr<folly::IOBuf>& ser);

  // Returns the last read sequence ID.  Used in servers
  // for backwards compatibility with thrift1.
  int32_t getSeqId() {
    return seqid_;
  }

 protected:
  template<typename StrType>
  inline uint32_t readStringBody(StrType& str, int32_t sz);

  inline void checkStringSize(int32_t size);

  int32_t string_limit_;
  int32_t container_limit_;

  // Enforce presence of version identifier
  bool strict_read_;

  /**
   * Cursor to manipulate the buffer to read from.  Throws an exception if
   * there is not enough data tor ead the whole struct.
   */
  Cursor in_;

  int32_t seqid_;
};

}} // apache::thrift

#include "BinaryProtocol.tcc"

#endif // #ifndef CPP2_PROTOCOL_TBINARYPROTOCOL_H_
