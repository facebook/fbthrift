/*
 * Copyright 2016 Facebook, Inc.
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

#include <array>
#include <limits>
#include <list>
#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/Cursor.h>
#include <thrift/lib/cpp/protocol/TBase64Utils.h>
#include <thrift/lib/cpp/protocol/TJSONProtocol.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

namespace apache { namespace thrift {

class JSONProtocolWriterCommon {

 public:

  explicit JSONProtocolWriterCommon(
      ExternalBufferSharing /*sharing*/ = COPY_EXTERNAL_BUFFER /* ignored */) {}

  /**
   * The IOBuf itself is managed by the caller.
   * It must exist for the life of the protocol as well,
   * or until the output is reset with setOutput/Input(nullptr), or
   * set to some other buffer.
   */
  inline void setOutput(
      folly::IOBufQueue* queue,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) {
    // Allocate 16KB at a time; leave some room for the IOBuf overhead
    constexpr size_t kDesiredGrowth = (1 << 14) - 64;
    out_.reset(queue, std::min(maxGrowth, kDesiredGrowth));
  }

  //  These writers are common to both json and simple-json protocols.
  inline uint32_t writeMessageBegin(const std::string& name,
                                    MessageType messageType,
                                    int32_t seqid);
  inline uint32_t writeMessageEnd();
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

  //  These sizes are common to both json and simple-json protocols.
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

  using TJSONProtocol = protocol::TJSONProtocol;

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

  static const uint8_t kJSONCharTable[0x30];
  static inline uint8_t hexChar(uint8_t val);

  void base64_encode(const uint8_t *in, uint32_t len, uint8_t *buf) {
    protocol::base64_encode(in, len, buf);
  }

  /**
   * Cursor to write the data out to.
   */
  folly::io::QueueAppender out_{nullptr, 0};

  struct Context {
    ContextType type;
    int meta;
  };

  std::list<Context> context;
};

class JSONProtocolReaderCommon {

 public:

  explicit JSONProtocolReaderCommon(
      ExternalBufferSharing /*sharing*/ = COPY_EXTERNAL_BUFFER /* ignored */) {}

  inline void setAllowDecodeUTF8(bool val) {
    allowDecodeUTF8_ = val;
  }

  /**
   * The IOBuf itself is managed by the caller.
   * It must exist for the life of the SimpleJSONProtocol as well,
   * or until the output is reset with setOutput/Input(NULL), or
   * set to some other buffer.
   */
  inline void setInput(const folly::IOBuf* buf) {
    in_.reset(buf);
  }

  inline uint32_t readMessageBegin(std::string& name,
                                   MessageType& messageType,
                                   int32_t& seqid);
  inline uint32_t readMessageEnd();
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

  inline uint32_t skip(TType type);

  folly::io::Cursor getCurrentPosition() const {
    return in_;
  }

  inline uint32_t readFromPositionAndAppend(folly::io::Cursor& cursor,
                                            std::unique_ptr<folly::IOBuf>& ser);

 protected:

  using TJSONProtocol = protocol::TJSONProtocol;

  enum class ContextType { MAP, ARRAY };

  template <typename Str>
  using is_string = std::is_same<typename Str::value_type, char>;

  // skip over whitespace so that we can peek, and store number of bytes
  // skipped
  inline void skipWhitespace();
  // skip over whitespace *and* return the number whitespace bytes skipped
  inline uint32_t readWhitespace();
  inline uint32_t ensureCharNoWhitespace(char expected);
  inline uint32_t ensureChar(char expected);
  // this is similar to skipWhitespace and readWhitespace.  The skip-version
  // skips over context so that we can peek, and stores the number of bytes
  // skipped.  The read-version calls the skip-version, and returns the number
  // of bytes skipped.  Calling skip a second (or third...) time in a row
  // without calling read has no effect.
  inline void ensureAndSkipContext();
  inline uint32_t ensureAndReadContext(bool& keyish);
  inline uint32_t beginContext(ContextType type);
  inline uint32_t ensureAndBeginContext(ContextType type);
  inline uint32_t endContext();

  template <typename T>
  static T castIntegral(folly::StringPiece val);
  template <typename T>
  uint32_t readInContext(T& val);
  inline uint32_t readJSONKey(std::string& key);
  inline uint32_t readJSONKey(folly::fbstring& key);
  inline uint32_t readJSONKey(bool key);
  template <typename T>
  uint32_t readJSONKey(T& key);
  template <typename T>
  uint32_t readJSONIntegral(T& val);
  inline uint32_t readNumericalChars(std::string& val);
  inline uint32_t readJSONVal(int8_t& val);
  inline uint32_t readJSONVal(int16_t& val);
  inline uint32_t readJSONVal(int32_t& val);
  inline uint32_t readJSONVal(int64_t& val);
  inline uint32_t readJSONVal(double& val);
  inline uint32_t readJSONVal(float& val);
  template <typename Str>
  inline typename std::enable_if<is_string<Str>::value, uint32_t>::type
  readJSONVal(Str& val);
  inline bool JSONtoBool(const std::string& s);
  inline uint32_t readJSONVal(bool& val);
  inline uint32_t readJSONNull();
  inline uint32_t readJSONKeyword(std::string& kw);
  inline uint32_t readJSONEscapeChar(uint8_t& out);
  template <typename StrType>
  uint32_t readJSONString(StrType& val);
  template <typename StrType>
  uint32_t readJSONBase64(StrType& s);

  static const std::string kEscapeChars;
  static const uint8_t kEscapeCharVals[8];
  static inline uint8_t hexVal(uint8_t ch);

  void base64_decode(uint8_t *buf, uint32_t len) {
    protocol::base64_decode(buf, len);
  }

  //  Rewrite in subclasses.
  std::array<folly::StringPiece, 2> bools_{{"", ""}};

  /**
   * Cursor to manipulate the buffer to read from.  Throws an exception if
   * there is not enough data tor ead the whole struct.
   */
  folly::io::Cursor in_{nullptr};

  struct Context {
    ContextType type;
    int meta;
  };

  std::list<Context> context;

  bool keyish_{false};
  // we sometimes consume whitespace while peeking
  uint32_t skippedWhitespace_{0};
  // we sometimes consume chars while peeking at context
  uint32_t skippedChars_{0};
  bool skippedIsUnread_{false};
  bool allowDecodeUTF8_{true};

};

}} // apache::thrift

#include "JSONProtocolCommon.tcc"
