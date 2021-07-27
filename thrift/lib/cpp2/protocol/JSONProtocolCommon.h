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

#pragma once

#include <array>
#include <limits>
#include <list>

#include <folly/Conv.h>
#include <folly/Range.h>
#include <folly/Traits.h>
#include <folly/dynamic.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/json.h>
#include <thrift/lib/cpp/protocol/TBase64Utils.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

namespace apache {
namespace thrift {

namespace detail {

template <typename T>
using value_type_of = typename T::value_type;

template <typename T>
using is_string = std::is_same<char, folly::detected_t<value_type_of, T>>;

namespace json {
constexpr uint8_t kJSONObjectStart = '{';
constexpr uint8_t kJSONObjectEnd = '}';
constexpr uint8_t kJSONArrayStart = '[';
constexpr uint8_t kJSONArrayEnd = ']';
constexpr uint8_t kJSONPairSeparator = ':';
constexpr uint8_t kJSONElemSeparator = ',';
constexpr uint8_t kJSONBackslash = '\\';
constexpr uint8_t kJSONStringDelimiter = '"';
constexpr uint8_t kJSONZeroChar = '0';
constexpr uint8_t kJSONEscapeChar = 'u';
constexpr uint8_t kJSONSpace = ' ';
constexpr uint8_t kJSONNewline = '\n';
constexpr uint8_t kJSONTab = '\t';
constexpr uint8_t kJSONCarriageReturn = '\r';
constexpr uint32_t kThriftVersion1 = 1;
constexpr folly::StringPiece kJSONEscapePrefix("\\u00");
constexpr folly::StringPiece kJSONTrue("true");
constexpr folly::StringPiece kJSONFalse("false");
constexpr folly::StringPiece kThriftNan("NaN");
constexpr folly::StringPiece kThriftNegativeNan("-NaN");
constexpr folly::StringPiece kThriftInfinity("Infinity");
constexpr folly::StringPiece kThriftNegativeInfinity("-Infinity");
} // namespace json
} // namespace detail

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

  inline void setOutput(folly::io::QueueAppender&& output) {
    out_ = std::move(output);
  }

  //  These writers are common to both json and simple-json protocols.
  inline uint32_t writeMessageBegin(
      const std::string& name, MessageType messageType, int32_t seqid);
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

  //  These sizes are common to both json and simple-json protocols.
  inline uint32_t serializedSizeByte(int8_t = 0) const;
  inline uint32_t serializedSizeI16(int16_t = 0) const;
  inline uint32_t serializedSizeI32(int32_t = 0) const;
  inline uint32_t serializedSizeI64(int64_t = 0) const;
  inline uint32_t serializedSizeDouble(double = 0.0) const;
  inline uint32_t serializedSizeFloat(float = 0) const;
  inline uint32_t serializedSizeString(folly::StringPiece) const;
  inline uint32_t serializedSizeBinary(folly::StringPiece str) const;
  inline uint32_t serializedSizeBinary(folly::ByteRange v) const;
  inline uint32_t serializedSizeBinary(
      const std::unique_ptr<folly::IOBuf>& v) const;
  inline uint32_t serializedSizeBinary(const folly::IOBuf& v) const;
  inline uint32_t serializedSizeZCBinary(folly::StringPiece str) const;
  inline uint32_t serializedSizeZCBinary(folly::ByteRange v) const;
  inline uint32_t serializedSizeZCBinary(
      const std::unique_ptr<folly::IOBuf>& /*v*/) const;
  inline uint32_t serializedSizeZCBinary(const folly::IOBuf& /*v*/) const;

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
  template <typename T>
  uint32_t writeJSONDouble(T dbl);

  static const uint8_t kJSONCharTable[0x30];
  static inline uint8_t hexChar(uint8_t val);

  void base64_encode(const uint8_t* in, uint32_t len, uint8_t* buf) {
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

 private:
  uint32_t writeJSONDoubleInternal(double dbl);
  uint32_t writeJSONDoubleInternal(float flt);
  uint32_t writeJSONIntInternal(int64_t num);
};

class JSONProtocolReaderCommon {
 public:
  explicit JSONProtocolReaderCommon(
      ExternalBufferSharing /*sharing*/ = COPY_EXTERNAL_BUFFER /* ignored */) {}

  inline void setAllowDecodeUTF8(bool val) { allowDecodeUTF8_ = val; }

  /**
   * The IOBuf itself is managed by the caller.
   * It must exist for the life of the SimpleJSONProtocol as well,
   * or until the output is reset with setOutput/Input(NULL), or
   * set to some other buffer.
   */
  void setInput(const folly::io::Cursor& cursor) { in_ = cursor; }
  void setInput(const folly::IOBuf* buf) { in_.reset(buf); }

  inline void readMessageBegin(
      std::string& name, MessageType& messageType, int32_t& seqid);
  inline void readMessageEnd();
  inline void readByte(int8_t& byte);
  inline void readI16(int16_t& i16);
  inline void readI32(int32_t& i32);
  inline void readI64(int64_t& i64);
  inline void readDouble(double& dub);
  inline void readFloat(float& flt);
  template <typename StrType>
  inline void readString(StrType& str);
  template <typename StrType>
  inline void readBinary(StrType& str);
  inline void readBinary(std::unique_ptr<folly::IOBuf>& str);
  inline void readBinary(folly::IOBuf& str);

  const folly::io::Cursor& getCursor() const { return in_; }

  size_t getCursorPosition() const { return in_.getCurrentPosition(); }

  inline uint32_t readFromPositionAndAppend(
      folly::io::Cursor& cursor, std::unique_ptr<folly::IOBuf>& ser);

  static constexpr std::size_t fixedSizeInContainer(TType) { return 0; }
  void skipBytes(size_t bytes) { in_.skip(bytes); }

 protected:
  enum class ContextType { MAP, ARRAY };

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
  inline void ensureAndReadContext(bool& keyish);
  inline void beginContext(ContextType type);
  inline void ensureAndBeginContext(ContextType type);
  inline void endContext();

  template <typename T>
  static T castIntegral(folly::StringPiece val);
  template <typename T>
  void readInContext(T& val);
  inline void readJSONKey(bool& key);
  template <typename T>
  void readJSONKey(T& key);
  template <typename T>
  void readJSONIntegral(T& val);
  inline void readNumericalChars(std::string& val);
  inline void readJSONVal(int8_t& val);
  inline void readJSONVal(int16_t& val);
  inline void readJSONVal(int32_t& val);
  inline void readJSONVal(int64_t& val);
  template <typename Floating>
  inline typename std::enable_if<std::is_floating_point<Floating>::value>::type
  readJSONVal(Floating& val);
  template <typename Str>
  inline typename std::enable_if<
      apache::thrift::detail::is_string<Str>::value>::type
  readJSONVal(Str& val);
  inline bool JSONtoBool(const std::string& s);
  inline void readJSONVal(bool& val);
  inline void readJSONNull();
  inline void readJSONKeyword(std::string& kw);
  inline void readJSONEscapeChar(uint8_t& out);
  template <typename StrType>
  void readJSONString(StrType& val);
  template <typename StrType>
  void readJSONBase64(StrType& s);

  // This string's characters must match up with the elements in kEscapeCharVals
  // I don't have '/' on this list even though it appears on www.json.org --
  // it is not in the RFC
  static constexpr folly::StringPiece kEscapeChars() { return "\"\\/bfnrt"; }

  static const uint8_t kEscapeCharVals[8];
  static inline uint8_t hexVal(uint8_t ch);

  void base64_decode(uint8_t* buf, uint32_t len) {
    protocol::base64_decode(buf, len);
  }

  template <class Predicate>
  uint32_t readWhile(const Predicate& pred, std::string& out);

  // Returns next character, or \0 if at the end.
  inline int8_t peekCharSafe();

  [[noreturn]] static void throwBadVersion();
  [[noreturn]] static void throwUnrecognizableAsBoolean(std::string const& s);
  [[noreturn]] static void throwUnrecognizableAsIntegral(
      folly::StringPiece s, folly::StringPiece typeName);
  [[noreturn]] static void throwUnrecognizableAsFloatingPoint(
      std::string const& s);
  [[noreturn]] static void throwUnrecognizableAsString(
      std::string const& s, std::exception const& e);
  [[noreturn]] static void throwUnrecognizableAsAny(std::string const& s);
  [[noreturn]] static void throwInvalidFieldStart(char ch);
  [[noreturn]] static void throwUnexpectedChar(char ch, char expected);
  [[noreturn]] static void throwInvalidEscapeChar(char ch);
  [[noreturn]] static void throwInvalidHexChar(char ch);

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

} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/protocol/JSONProtocolCommon-inl.h>
