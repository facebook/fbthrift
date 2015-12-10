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

#ifndef CPP2_PROTOCOL_TSIMPLEJSONPROTOCOL_H_
#define CPP2_PROTOCOL_TSIMPLEJSONPROTOCOL_H_ 1

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/Cursor.h>
#include <list>
#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

namespace apache { namespace thrift {

using folly::IOBuf;
using folly::IOBufQueue;

typedef folly::io::RWPrivateCursor RWCursor;
using folly::io::Cursor;
using folly::io::QueueAppender;

class SimpleJSONProtocolWriter {

 public:
  static const int32_t VERSION_1 = 0x80010000;

  explicit SimpleJSONProtocolWriter(
      ExternalBufferSharing /*sharing*/ = COPY_EXTERNAL_BUFFER /* ignored */)
    : out_(nullptr, 0) {}

  static constexpr ProtocolType protocolType() {
    return ProtocolType::T_SIMPLE_JSON_PROTOCOL;
  }

  /**
   * ...
   * The IOBuf itself is managed by the caller.
   * It must exist for the life of the SimpleJSONProtocol as well,
   * or until the output is reset with setOutput/Input(NULL), or
   * set to some other buffer.
   */
  inline void setOutput(
      IOBufQueue* queue,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) {
    // Allocate 16KB at a time; leave some room for the IOBuf overhead
    constexpr size_t kDesiredGrowth = (1 << 14) - 64;
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
  inline uint32_t writeString(folly::StringPiece str);
  inline uint32_t writeBinary(folly::StringPiece str);
  inline uint32_t writeBinary(folly::ByteRange v);
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
  inline uint32_t serializedSizeString(folly::StringPiece);
  inline uint32_t serializedSizeBinary(folly::StringPiece str);
  inline uint32_t serializedSizeBinary(folly::ByteRange v);
  inline uint32_t serializedSizeBinary(const std::unique_ptr<folly::IOBuf>& v);
  inline uint32_t serializedSizeBinary(const folly::IOBuf& v);
  inline uint32_t serializedSizeZCBinary(folly::StringPiece str);
  inline uint32_t serializedSizeZCBinary(folly::ByteRange v);
  inline uint32_t serializedSizeZCBinary(
      const std::unique_ptr<folly::IOBuf>& /*v*/);
  inline uint32_t serializedSizeZCBinary(const folly::IOBuf& /*v*/);
  inline uint32_t serializedSizeSerializedData(
      const std::unique_ptr<folly::IOBuf>& data);

 protected:
  enum class ContextType { MAP, ARRAY };
  inline uint32_t beginContext(ContextType);
  inline uint32_t endContext();
  inline uint32_t writeContext();
  inline uint32_t writeJSONEscapeChar(uint8_t ch);
  inline uint32_t writeJSONChar(uint8_t ch);
  inline uint32_t writeJSONString(folly::StringPiece);
  inline uint32_t writeJSONBase64(folly::ByteRange);
  inline uint32_t writeJSONBool(bool val);
  inline uint32_t writeJSONInt(int64_t num);
  template<typename T>
  uint32_t writeJSONDouble(T dbl);

  /**
   * Cursor to write the data out to.
   */
  QueueAppender out_;

  struct Context {
    ContextType type;
    int meta;
  };

  std::list<Context> context;
};

class SimpleJSONProtocolReader {

 public:
  static const int32_t VERSION_MASK = 0xffff0000;
  static const int32_t VERSION_1 = 0x80010000;

  explicit SimpleJSONProtocolReader(
      ExternalBufferSharing /*sharing*/ = COPY_EXTERNAL_BUFFER /* ignored */)
    : in_(nullptr)
    , allowDecodeUTF8_(true)
    , skippedWhitespace_(0)
    , skippedChars_(0)
    , skippedIsUnread_(false) {}

  static constexpr ProtocolType protocolType() {
    return ProtocolType::T_SIMPLE_JSON_PROTOCOL;
  }

  /**
   * The IOBuf itself is managed by the caller.
   * It must exist for the life of the SimpleJSONProtocol as well,
   * or until the output is reset with setOutput/Input(NULL), or
   * set to some other buffer.
   */
  inline void setInput(const IOBuf* buf) {
    in_.reset(buf);
  }

  inline void setAllowDecodeUTF8(bool val) {
    allowDecodeUTF8_ = val;
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
  inline bool peekMap();
  inline bool peekList();
  inline bool peekSet();

  inline uint32_t skip(TType type);

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
  enum class ContextType { MAP, ARRAY };

  // skip over whitespace so that we can peek, and store number of bytes
  // skipped
  inline void skipWhitespace();

  // skip over whitespace *and* return the number whitespace bytes skipped
  inline uint32_t readWhitespace();

  inline uint32_t ensureChar(char expected);
  inline uint32_t ensureCharNoWhitespace(char expected);
  inline uint32_t beginContext(ContextType type);
  inline uint32_t ensureAndBeginContext(ContextType type);
  inline uint32_t endContext();

  // this is similar to skipWhitespace and readWhitespace.  The skip-version
  // skips over context so that we can peek, and stores the number of bytes
  // skipped.  The read-version calls the skip-version, and returns the number
  // of bytes skipped.  Calling skip a second (or third...) time in a row
  // without calling read has no effect.
  inline void ensureAndSkipContext();
  inline uint32_t ensureAndReadContext(bool& keyish);

  template <typename T>
  uint32_t readInContext(T& val);
  inline uint32_t readJSONKey(std::string& key);
  inline uint32_t readJSONKey(folly::fbstring& key);
  inline uint32_t readJSONKey(bool key);
  template <typename T>
  uint32_t readJSONKey(T& key);
  template <typename T>
  T castIntegral(const std::string& val);
  template <typename T>
  uint32_t readJSONIntegral(T& val);
  inline uint32_t readNumericalChars(std::string& val);
  inline uint32_t readJSONVal(int8_t& val);
  inline uint32_t readJSONVal(int16_t& val);
  inline uint32_t readJSONVal(int32_t& val);
  inline uint32_t readJSONVal(int64_t& val);
  inline uint32_t readJSONVal(double& val);
  inline uint32_t readJSONVal(float& val);
  inline uint32_t readJSONVal(std::string& val);
  inline bool JSONtoBool(const std::string& s);
  inline uint32_t readJSONVal(bool& val);
  inline uint32_t readJSONNull();
  inline uint32_t readJSONEscapeChar(uint8_t& out);
  template <typename StrType>
  uint32_t readJSONString(StrType& val);
  template <typename StrType>
  uint32_t readJSONBase64(StrType& s);

  /**
   * Cursor to manipulate the buffer to read from.  Throws an exception if
   * there is not enough data tor ead the whole struct.
   */
  Cursor in_;

  int32_t seqid_;

  struct Context {
    ContextType type;
    int meta;
  };

  std::list<Context> context;
  bool allowDecodeUTF8_;
  uint32_t skippedWhitespace_;  // we sometimes consume whitespace while peeking

  bool keyish_;
  uint32_t skippedChars_; // we sometimes consume chars while peeking at context
  bool skippedIsUnread_;
};

}} // apache::thrift

#include "SimpleJSONProtocol.tcc"

#endif // #ifndef CPP2_PROTOCOL_TSIMPLEJSONPROTOCOL_H_
