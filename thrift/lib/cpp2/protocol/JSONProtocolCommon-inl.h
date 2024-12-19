/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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

#include <cuchar>
#include <iostream>
#include <folly/Unicode.h>
#include <folly/Utility.h>
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif /* __ARM_NEON */

namespace apache {
namespace thrift {

// Return the hex character representing the integer val. The value is masked
// to make sure it is in the correct range.
inline uint8_t JSONProtocolWriterCommon::hexChar(uint8_t val) {
  val &= 0x0F;
  if (val < 10) {
    return val + '0';
  } else {
    return val - 10 + 'a';
  }
}

/*
 * Public writing methods
 */

inline uint32_t JSONProtocolWriterCommon::writeMessageBegin(
    const std::string& name, MessageType messageType, int32_t seqid) {
  auto ret = beginContext(ContextType::ARRAY);
  ret += writeI32(apache::thrift::detail::json::kThriftVersion1);
  ret += writeString(name);
  ret += writeI32(static_cast<int32_t>(messageType));
  ret += writeI32(seqid);
  return ret;
}

inline uint32_t JSONProtocolWriterCommon::writeMessageEnd() {
  return endContext();
}

inline uint32_t JSONProtocolWriterCommon::writeByte(int8_t byte) {
  auto ret = writeContext();
  return ret + writeJSONInt(byte);
}

inline uint32_t JSONProtocolWriterCommon::writeI16(int16_t i16) {
  auto ret = writeContext();
  return ret + writeJSONInt(i16);
}

inline uint32_t JSONProtocolWriterCommon::writeI32(int32_t i32) {
  auto ret = writeContext();
  return ret + writeJSONInt(i32);
}

inline uint32_t JSONProtocolWriterCommon::writeI64(int64_t i64) {
  auto ret = writeContext();
  return ret + writeJSONInt(i64);
}

inline uint32_t JSONProtocolWriterCommon::writeDouble(double dub) {
  auto ret = writeContext();
  return ret + writeJSONDouble(dub);
}

inline uint32_t JSONProtocolWriterCommon::writeFloat(float flt) {
  auto ret = writeContext();
  return ret + writeJSONDouble(flt);
}

inline uint32_t JSONProtocolWriterCommon::writeString(folly::StringPiece str) {
  auto ret = writeContext();
  return ret + writeJSONString(str);
}

inline uint32_t JSONProtocolWriterCommon::writeBinary(folly::StringPiece str) {
  return writeBinary(folly::ByteRange(str));
}

inline uint32_t JSONProtocolWriterCommon::writeBinary(folly::ByteRange v) {
  auto ret = writeContext();
  return ret + writeJSONBase64(v);
}

inline uint32_t JSONProtocolWriterCommon::writeBinary(
    const std::unique_ptr<folly::IOBuf>& str) {
  DCHECK(str);
  if (!str) {
    auto ret = writeContext();
    return ret + writeJSONString(folly::StringPiece());
  }
  return writeBinary(*str);
}

inline uint32_t JSONProtocolWriterCommon::writeBinary(const folly::IOBuf& str) {
  auto ret = writeContext();
  return ret + writeJSONBase64(str.clone()->coalesce());
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeByte(
    int8_t /*val*/) const {
  // 3 bytes for serialized, plus it might be a key, plus context
  return 6;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeI16(
    int16_t /*val*/) const {
  return 8;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeI32(
    int32_t /*val*/) const {
  return 13;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeI64(
    int64_t /*val*/) const {
  return 25;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeDouble(
    double /*val*/) const {
  return 25;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeFloat(
    float /*val*/) const {
  return 25;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeString(
    folly::StringPiece str) const {
  return static_cast<uint32_t>(str.size()) * 6 + 3;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeBinary(
    folly::StringPiece str) const {
  return serializedSizeBinary(folly::ByteRange(str));
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeBinary(
    folly::ByteRange v) const {
  return static_cast<uint32_t>(v.size()) * 6 + 3;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeBinary(
    const std::unique_ptr<folly::IOBuf>& v) const {
  return (v ? serializedSizeBinary(*v) * 6 : 0) + 3;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeBinary(
    const folly::IOBuf& v) const {
  size_t size = v.computeChainDataLength();
  uint32_t limit = std::numeric_limits<uint32_t>::max() - serializedSizeI32();
  if (size > limit) {
    TProtocolException::throwExceededSizeLimit(size, limit);
  }
  return static_cast<uint32_t>(size) * 6 + 3;
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeZCBinary(
    folly::StringPiece str) const {
  return serializedSizeZCBinary(folly::ByteRange(str));
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeZCBinary(
    folly::ByteRange v) const {
  return serializedSizeBinary(v);
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeZCBinary(
    const std::unique_ptr<folly::IOBuf>&) const {
  // size only
  return serializedSizeI32();
}

inline uint32_t JSONProtocolWriterCommon::serializedSizeZCBinary(
    const folly::IOBuf&) const {
  // size only
  return serializedSizeI32();
}

/**
 * Protected writing methods
 */

inline uint32_t JSONProtocolWriterCommon::beginContext(ContextType type) {
  context.push_back({type, 0});
  switch (type) {
    case ContextType::MAP:
      out_.write(apache::thrift::detail::json::kJSONObjectStart);
      return 1;
    case ContextType::ARRAY:
      out_.write(apache::thrift::detail::json::kJSONArrayStart);
      return 1;
  }
  CHECK(false);
  return 0;
}

inline uint32_t JSONProtocolWriterCommon::endContext() {
  DCHECK(!context.empty());
  switch (context.back().type) {
    case ContextType::MAP:
      out_.write(apache::thrift::detail::json::kJSONObjectEnd);
      break;
    case ContextType::ARRAY:
      out_.write(apache::thrift::detail::json::kJSONArrayEnd);
      break;
  }
  context.pop_back();
  return 1;
}

inline uint32_t JSONProtocolWriterCommon::writeContext() {
  if (context.empty()) {
    return 0;
  }
  auto& ctx = context.back();
  auto meta = ctx.meta++;
  switch (ctx.type) {
    case ContextType::MAP:
      if (meta == 0) {
        return 0;
      } else if (meta % 2 == 0) {
        out_.write(apache::thrift::detail::json::kJSONElemSeparator);
      } else {
        out_.write(apache::thrift::detail::json::kJSONPairSeparator);
      }
      return 1;
    case ContextType::ARRAY:
      if (meta != 0) {
        out_.write(apache::thrift::detail::json::kJSONElemSeparator);
        return 1;
      }
      return 0;
  }
  CHECK(false);
  return 0;
}

inline uint32_t JSONProtocolWriterCommon::writeJSONEscapeChar(uint8_t ch) {
  DCHECK(apache::thrift::detail::json::kJSONEscapePrefix.size() == 4);
  out_.push(
      (const uint8_t*)apache::thrift::detail::json::kJSONEscapePrefix.data(),
      4);
  out_.write(hexChar(ch >> 4));
  out_.write(hexChar(ch));
  return 6;
}

inline uint32_t JSONProtocolWriterCommon::writeJSONChar(uint8_t ch) {
  if (ch >= 32) {
    // Only special character >= 32 is '\' and '='
    if (ch == apache::thrift::detail::json::kJSONStringDelimiter) {
      constexpr uint16_t res = apache::thrift::detail::json::kJSONBackslash |
        ((uint16_t)apache::thrift::detail::json::kJSONStringDelimiter << 8);
      out_.write(res);
    } else if (ch == apache::thrift::detail::json::kJSONBackslash) {
      constexpr uint16_t res = apache::thrift::detail::json::kJSONBackslash |
          ((uint16_t)apache::thrift::detail::json::kJSONBackslash << 8);
      out_.write(res);
      return 2;
    } else {
      out_.write(ch);
      return 1;
    }
  } else {
    uint8_t outCh = kJSONCharTable[ch];
    // Check if regular character, backslash escaped, or JSON escaped
    if (outCh != 0) {
      uint16_t res{0};
      res |= apache::thrift::detail::json::kJSONBackslash;
      res |= ((uint16_t)outCh << 8);
      out_.write(res);
      return 2;
    } else {
      return writeJSONEscapeChar(ch);
    }
  }
}

inline uint32_t JSONProtocolWriterCommon::writeJSONString(
    folly::StringPiece str) {
  uint32_t ret = 2;
#ifdef __ARM_NEON
  if (str.empty()) {
    // for an empty string
    constexpr uint16_t res =
        apache::thrift::detail::json::kJSONStringDelimiter |
        ((uint16_t)apache::thrift::detail::json::kJSONStringDelimiter << 8);
    out_.write(res);
    return ret;
  }
  out_.write(apache::thrift::detail::json::kJSONStringDelimiter);
  int i = 0;

  static const uint8x16_t backslashx16 = vdupq_n_u8('\\');
  static const uint8x16_t specialCharsx16 = vdupq_n_u8(0x30);
  static const uint8x8_t backslashx8 = vdup_n_u8('\\');
  static const uint8x8_t specialCharsx8 = vdup_n_u8(0x30);
  for (; i + 15 < str.size(); i += 16) {
    // load 16 bytes per chunk, if available
    uint8x16_t val = vld1q_u8((uint8_t*)&str[i]);

    uint8x16_t lteMask = vcleq_u8(val, specialCharsx16); // non-zero for every char that is below 0x30 / '0'.
    uint8x16_t backslashMask = vceqq_u8(val, backslashx16); // non-zero for every char that equals to '\'
    uint8x16_t mask = vorrq_u8(lteMask, backslashMask); // non-zero for every char that requires escape check
    uint64_t lowEscaped = vgetq_lane_u64(vreinterpretq_u64_u8(mask), 0); // if true, any char in the lower half (byte 0..7) needs escape check
    uint64_t highEscaped = vgetq_lane_u64(vreinterpretq_u64_u8(mask), 1); // if true, any char in the upper half (byte 8..15) needs escape check
    if (FOLLY_UNLIKELY(lowEscaped || highEscaped)) {

      if (lowEscaped) {
        for (int j = i; j < i + 8; ++j) {
          ret += writeJSONChar(str[j]);
        }
      } else {
        out_.push((const uint8_t*)&val, sizeof(uint8x8_t));
        ret += 8;
      }

      if (highEscaped) {
        for (int j = i + 8; j < i + 16; ++j) {
          ret += writeJSONChar(str[j]);
        }
      } else {
        out_.push((const uint8_t*)&val + sizeof(uint8x8_t), sizeof(uint8x8_t));
        ret += 8;
      }

    } else {
      out_.push((const uint8_t*)&val, sizeof(uint8x16_t));
      ret += 16;
    }
  } // end 16 byte per iteration loop

  for (; i + 7 < str.size(); i += 8) {
    // load 16 bytes per chunk, if available
    uint8x8_t val = vld1_u8((uint8_t*)&str[i]);

    uint8x8_t lteMask = vcle_u8(val, specialCharsx8); // non-zero for every char that is below 0x30 / '0'.
    uint8x8_t backslashMask = vceq_u8(val, backslashx8); // non-zero for every char that equals to '\'
    uint8x8_t mask = vorr_u8(lteMask, backslashMask); // non-zero for every char that requires escape check
    uint64_t escaped = vget_lane_u64(vreinterpret_u64_u8(mask), 0); // if true, any char needs escape check
    if (FOLLY_UNLIKELY(escaped)) {
      auto firstEscapeIdx = __builtin_ctzl(escaped) / 8;
      auto lastEscapeIdx = 7 - __builtin_clzl(escaped) / 8;
      for (int j = i; j < i + firstEscapeIdx; ++j) {
        out_.write(str[j]);
      }
      for (int j = i + firstEscapeIdx; j <= i + lastEscapeIdx; ++j) {
        ret += writeJSONChar(str[j]);
      }
      for (int j = i + lastEscapeIdx + 1; j < i + 8; ++j) {
        out_.write(str[j]);
      }
    } else {
      out_.push((const uint8_t*)&val, sizeof(uint8x8_t));
      ret += 8;
    }
  } // end 8 byte per iteration loop

  // remainder loop
#pragma unroll
  for (; i < str.size(); ++i) {
    ret += writeJSONChar(str[i]);
  }
  out_.write(apache::thrift::detail::json::kJSONStringDelimiter);
#else // __ARM_NEON
  out_.write(apache::thrift::detail::json::kJSONStringDelimiter);
  for (uint8_t ch : str) {
    // Only special characters >= 32 are '\' and '"'
    if (ch == apache::thrift::detail::json::kJSONBackslash ||
        ch == apache::thrift::detail::json::kJSONStringDelimiter) {
      out_.write(apache::thrift::detail::json::kJSONBackslash);
      ret += 1;
    }
    if (ch >= 32) {
      out_.write(ch);
      ret += 1;
    } else {
      uint8_t outCh = kJSONCharTable[ch];
      // Check if regular character, backslash escaped, or JSON escaped
      if (outCh != 0) {
        out_.write(apache::thrift::detail::json::kJSONBackslash);
        out_.write(outCh);
        ret += 2;
      } else {
        ret += writeJSONEscapeChar(ch);
      }
    }
  }
  out_.write(apache::thrift::detail::json::kJSONStringDelimiter);
#endif
  return ret;
}

inline uint64_t pack_uint64(const uint32_t i0, const uint32_t i1) {
  uint64_t ret{0};
  ret |= ((uint64_t)i1) << 32;
  ret |= i0;
  return ret;
}

inline uint32_t pack_uint32(
    const uint8_t c0, const uint8_t c1, const uint8_t c2, const uint8_t c3) {
  uint32_t ret{0};
  ret |= c3 << 24;
  ret |= c2 << 16;
  ret |= c1 << 8;
  ret |= c0;
  return ret;
}

inline uint16_t pack_uint16(const uint8_t c0, const uint8_t c1) {
  uint16_t ret{0};
  ret |= c1 << 8;
  ret |= c0;
  return ret;
}

static const uint8_t* kBase64EncodeTable =
   reinterpret_cast<const uint8_t*>("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

inline uint32_t base64_encode_3_inline(
    const uint8_t in0, const uint8_t in1, const uint8_t in2) {
  return pack_uint32(
      // 6 bits of in0
      kBase64EncodeTable[(in0 >> 2) & 0x3f],
      // 2 bits of in0 and 4 bits of in1
      kBase64EncodeTable[((in0 << 4) & 0x30) | ((in1 >> 4) & 0x0f)],
      // 4 bits of in 1 and 2 bits of in2
      kBase64EncodeTable[((in1 << 2) & 0x3c) | ((in2 >> 6) & 0x03)],
      // 6 bits of in2
      kBase64EncodeTable[in2 & 0x3f]);
}

inline uint32_t JSONProtocolWriterCommon::writeJSONBase64(folly::ByteRange v) {
  uint32_t ret = 2;

  out_.write(apache::thrift::detail::json::kJSONStringDelimiter);
  auto bytes = v.data();
  uint32_t len = folly::to_narrow(v.size());
  while (len >= 6) {
    // encode 6 bytes at a time
    out_.write(pack_uint64(
        base64_encode_3_inline(bytes[0], bytes[1], bytes[2]),
        base64_encode_3_inline(bytes[3], bytes[4], bytes[5])));
    ret += 8;
    bytes += 6;
    len -= 6;
  }
  while (len >= 3) {
    // Encode 3 bytes at a time
    out_.write(base64_encode_3_inline(bytes[0], bytes[1], bytes[2]));
    ret += 4;
    bytes += 3;
    len -= 3;
  }
  if (len == 2) {
    DCHECK_LE(len, folly::to_unsigned(std::numeric_limits<int>::max()));
    out_.write(pack_uint32(
        kBase64EncodeTable[(bytes[0] >> 2) & 0x3f],
        kBase64EncodeTable[((bytes[0] << 4) & 0x30) | ((bytes[1] >> 4) & 0x0f)],
        kBase64EncodeTable[(bytes[1] << 2) & 0x3c],
        apache::thrift::detail::json::kJSONStringDelimiter));
    return ret + 3;
  } else if (len == 1) {
    DCHECK_LE(len, folly::to_unsigned(std::numeric_limits<int>::max()));
    out_.write(pack_uint16(
        kBase64EncodeTable[(bytes[0] >> 2) & 0x3f],
        kBase64EncodeTable[(bytes[0] << 4) & 0x30]));
    ret += 2;
  }
  out_.write(apache::thrift::detail::json::kJSONStringDelimiter);

  return ret;
}

inline uint32_t JSONProtocolWriterCommon::writeJSONBool(bool val) {
  return writeJSONBoolInternal(val);
}

inline uint32_t JSONProtocolWriterCommon::writeJSONInt(int64_t num) {
  return writeJSONIntInternal(num);
}

template <typename T>
inline uint32_t JSONProtocolWriterCommon::writeJSONDouble(T dbl) {
  if (dbl == std::numeric_limits<T>::infinity()) {
    return writeJSONString(apache::thrift::detail::json::kThriftInfinity);
  } else if (dbl == -std::numeric_limits<T>::infinity()) {
    return writeJSONString(
        apache::thrift::detail::json::kThriftNegativeInfinity);
  } else if (std::isnan(dbl)) {
    return writeJSONString(apache::thrift::detail::json::kThriftNan);
  } else {
    return writeJSONDoubleInternal(dbl);
  }
}

/**
 * Public reading functions
 */

inline void JSONProtocolReaderCommon::readMessageBegin(
    std::string& name, MessageType& messageType, int32_t& seqid) {
  ensureAndBeginContext(ContextType::ARRAY);
  int64_t tmpVal;
  readI64(tmpVal);
  if (tmpVal != apache::thrift::detail::json::kThriftVersion1) {
    throwBadVersion();
  }
  readString(name);
  readI64(tmpVal);
  messageType = (MessageType)tmpVal;
  readI32(seqid);
}

inline void JSONProtocolReaderCommon::readMessageEnd() {
  endContext();
}

inline void JSONProtocolReaderCommon::readByte(int8_t& byte) {
  readInContext<int8_t>(byte);
}

inline void JSONProtocolReaderCommon::readI16(int16_t& i16) {
  readInContext<int16_t>(i16);
}

inline void JSONProtocolReaderCommon::readI32(int32_t& i32) {
  readInContext<int32_t>(i32);
}

inline void JSONProtocolReaderCommon::readI64(int64_t& i64) {
  readInContext<int64_t>(i64);
}

inline void JSONProtocolReaderCommon::readDouble(double& dub) {
  readInContext<double>(dub);
}

inline void JSONProtocolReaderCommon::readFloat(float& flt) {
  readInContext<float>(flt);
}

template <typename StrType>
inline void JSONProtocolReaderCommon::readString(StrType& str) {
  bool keyish;
  ensureAndReadContext(keyish);
  readJSONString(str);
}

template <typename StrType>
inline void JSONProtocolReaderCommon::readBinary(StrType& str) {
  bool keyish;
  ensureAndReadContext(keyish);
  readJSONBase64(str);
}

inline void JSONProtocolReaderCommon::readBinary(
    std::unique_ptr<folly::IOBuf>& str) {
  folly::IOBufQueue queue;
  folly::io::QueueAppender a(&queue, 1000);
  bool keyish;
  ensureAndReadContext(keyish);
  readJSONBase64(a);
  str = queue.move();
}

inline void JSONProtocolReaderCommon::readBinary(folly::IOBuf& str) {
  folly::IOBufQueue queue;
  folly::io::QueueAppender a(&queue, 1000);
  bool keyish;
  ensureAndReadContext(keyish);
  readJSONBase64(a);
  str.appendChain(queue.move());
}

/**
 * Protected reading functions
 */

inline void JSONProtocolReaderCommon::skipWhitespace() {
  for (auto peek = in_.peekBytes(); !peek.empty(); peek = in_.peekBytes()) {
    uint32_t size = 0;
    for (char ch : peek) {
      if (ch != apache::thrift::detail::json::kJSONSpace &&
          ch != apache::thrift::detail::json::kJSONNewline &&
          ch != apache::thrift::detail::json::kJSONTab &&
          ch != apache::thrift::detail::json::kJSONCarriageReturn) {
        in_.skip(size);
        return;
      }
      ++skippedWhitespace_;
      ++size;
    }
    in_.skip(size);
  }
}

inline uint32_t JSONProtocolReaderCommon::readWhitespace() {
  skipWhitespace();
  auto ret = skippedWhitespace_;
  skippedWhitespace_ = 0;
  return ret;
}

inline uint32_t JSONProtocolReaderCommon::ensureCharNoWhitespace(
    char expected) {
  auto actual = in_.read<int8_t>();
  if (actual != expected) {
    throwUnexpectedChar(actual, expected);
  }
  return 1;
}

inline uint32_t JSONProtocolReaderCommon::ensureChar(char expected) {
  auto ret = readWhitespace();
  return ret + ensureCharNoWhitespace(expected);
}

inline void JSONProtocolReaderCommon::ensureAndSkipContext() {
  if (skippedIsUnread_) {
    return;
  }
  skippedIsUnread_ = true;
  keyish_ = false;
  if (!context.empty()) {
    auto meta = context.back().meta++;
    switch (context.back().type) {
      case ContextType::MAP:
        if (meta % 2 == 0) {
          if (meta != 0) {
            skippedChars_ +=
                ensureChar(apache::thrift::detail::json::kJSONElemSeparator);
          }
          keyish_ = true;
        } else {
          skippedChars_ +=
              ensureChar(apache::thrift::detail::json::kJSONPairSeparator);
        }
        break;
      case ContextType::ARRAY:
        if (meta != 0) {
          skippedChars_ +=
              ensureChar(apache::thrift::detail::json::kJSONElemSeparator);
        }
        break;
    }
  }
}

inline void JSONProtocolReaderCommon::ensureAndReadContext(bool& keyish) {
  ensureAndSkipContext();
  keyish = keyish_;
  skippedChars_ = 0;
  skippedIsUnread_ = false;
}

inline void JSONProtocolReaderCommon::ensureAndBeginContext(ContextType type) {
  bool keyish;
  ensureAndReadContext(keyish);
  // perhaps handle keyish == true?  I think for backwards compat we want to
  // be able to handle non-string keys, even if it isn't valid JSON
  beginContext(type);
}

inline void JSONProtocolReaderCommon::beginContext(ContextType type) {
  context.push_back({type, 0});
  switch (type) {
    case ContextType::MAP:
      ensureChar(apache::thrift::detail::json::kJSONObjectStart);
      return;
    case ContextType::ARRAY:
      ensureChar(apache::thrift::detail::json::kJSONArrayStart);
      return;
  }
  CHECK(false);
}

inline void JSONProtocolReaderCommon::endContext() {
  DCHECK(!context.empty());

  auto type = context.back().type;
  context.pop_back();
  switch (type) {
    case ContextType::MAP:
      ensureChar(apache::thrift::detail::json::kJSONObjectEnd);
      return;
    case ContextType::ARRAY:
      ensureChar(apache::thrift::detail::json::kJSONArrayEnd);
      return;
  }
  CHECK(false);
}

template <typename T>
T JSONProtocolReaderCommon::castIntegral(folly::StringPiece val) {
  return folly::tryTo<T>(val).thenOrThrow(
      [](auto x) { return x; },
      [&](folly::ConversionCode) {
        throwUnrecognizableAsIntegral(val, folly::pretty_name<T>());
      });
}

template <typename T>
void JSONProtocolReaderCommon::readInContext(T& val) {
  static_assert(
      !apache::thrift::detail::is_string<T>::value,
      "Strings are strings in any context, use readJSONString");

  bool keyish;
  ensureAndReadContext(keyish);
  if (keyish) {
    readJSONKey(val);
  } else {
    readJSONVal(val);
  }
}

inline void JSONProtocolReaderCommon::readJSONKey(bool& key) {
  std::string s;
  readJSONString(s);
  key = JSONtoBool(s);
}

template <typename T>
void JSONProtocolReaderCommon::readJSONKey(T& key) {
  std::string s;
  readJSONString(s);
  key = castIntegral<T>(s);
}

template <typename T>
void JSONProtocolReaderCommon::readJSONIntegral(T& val) {
  std::string serialized;
  readNumericalChars(serialized);
  val = castIntegral<T>(serialized);
}

inline void JSONProtocolReaderCommon::readNumericalChars(std::string& val) {
  readWhitespace();
  readWhile(
      [](uint8_t ch) {
        return (ch >= '0' && ch <= '9') || ch == '+' || ch == '-' ||
            ch == '.' || ch == 'E' || ch == 'e';
      },
      val);
}

inline void JSONProtocolReaderCommon::readJSONVal(int8_t& val) {
  readJSONIntegral<int8_t>(val);
}

inline void JSONProtocolReaderCommon::readJSONVal(int16_t& val) {
  readJSONIntegral<int16_t>(val);
}

inline void JSONProtocolReaderCommon::readJSONVal(int32_t& val) {
  readJSONIntegral<int32_t>(val);
}

inline void JSONProtocolReaderCommon::readJSONVal(int64_t& val) {
  readJSONIntegral<int64_t>(val);
}

template <typename Floating>
inline typename std::enable_if_t<std::is_floating_point_v<Floating>>
JSONProtocolReaderCommon::readJSONVal(Floating& val) {
  static_assert(
      std::numeric_limits<Floating>::is_iec559,
      "Parameter type must fulfill IEEE 754 floating-point standard");

  readWhitespace();
  if (peekCharSafe() == apache::thrift::detail::json::kJSONStringDelimiter) {
    std::string str;
    readJSONString(str);
    if (str == apache::thrift::detail::json::kThriftNan) {
      val = std::numeric_limits<Floating>::quiet_NaN();
    } else if (str == apache::thrift::detail::json::kThriftNegativeNan) {
      val = -std::numeric_limits<Floating>::quiet_NaN();
    } else if (str == apache::thrift::detail::json::kThriftInfinity) {
      val = std::numeric_limits<Floating>::infinity();
    } else if (str == apache::thrift::detail::json::kThriftNegativeInfinity) {
      val = -std::numeric_limits<Floating>::infinity();
    } else {
      throwUnrecognizableAsFloatingPoint(str);
    }
    return;
  }
  std::string s;
  readNumericalChars(s);
  try {
    val = folly::to<Floating>(s);
  } catch (const std::exception&) {
    throwUnrecognizableAsFloatingPoint(s);
  }
}

template <typename Str>
inline typename std::enable_if_t<apache::thrift::detail::is_string<Str>::value>
JSONProtocolReaderCommon::readJSONVal(Str& val) {
  readJSONString(val);
}

inline bool JSONProtocolReaderCommon::JSONtoBool(const std::string& s) {
  if (s == "true") {
    return true;
  } else if (s == "false") {
    return false;
  } else {
    throwUnrecognizableAsBoolean(s);
  }
  return false;
}

inline void JSONProtocolReaderCommon::readJSONVal(bool& val) {
  std::string s;
  readJSONKeyword(s);
  val = JSONtoBool(s);
}

inline void JSONProtocolReaderCommon::readJSONNull() {
  std::string s;
  readJSONKeyword(s);
  if (s != "null") {
    throwUnrecognizableAsAny(s);
  }
}

inline void JSONProtocolReaderCommon::readJSONKeyword(std::string& kw) {
  readWhitespace();
  readWhile([](int8_t ch) { return ch >= 'a' && ch <= 'z'; }, kw);
}

inline void JSONProtocolReaderCommon::readJSONEscapeChar(uint8_t& out) {
  uint8_t b1, b2;
  ensureCharNoWhitespace(apache::thrift::detail::json::kJSONZeroChar);
  ensureCharNoWhitespace(apache::thrift::detail::json::kJSONZeroChar);
  b1 = in_.read<uint8_t>();
  b2 = in_.read<uint8_t>();
  out = static_cast<uint8_t>((hexVal(b1) << 4) + hexVal(b2));
}

#ifdef __ARM_NEON
static inline uint8_t hexVal_inl(uint8_t ch) {
  if ((ch >= '0') && (ch <= '9')) {
    return ch - '0';
  } else if ((ch >= 'a') && (ch <= 'f')) {
    return ch - 'a' + 10;
  } else if ((ch >= 'A') && (ch <= 'F')) {
    return ch - 'A' + 10;
  } else {
    return '\0';
  }
}

inline uint8_t readOrFallback(
    const folly::ByteRange& input,
    folly::io::Cursor& fallBackInput,
    unsigned& inIdx,
    unsigned& skipped) {
  if (inIdx < input.size()) {
    ++skipped;
    return input[inIdx++];
  } else {
    if (skipped > 0) {
      fallBackInput.skip(skipped);
      skipped = 0;
    }
    return fallBackInput.read<uint8_t>();
  }
}

inline char lookupJSONEscapeChar(char ch) {
  switch (ch) {
    case '"': [[fallthrough]];
    case '\\': [[fallthrough]];
    case '/':
      return ch;
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    default:
      throw TProtocolException(
          TProtocolException::INVALID_DATA,
          "Invalid escaped char " + std::to_string(ch));
  }
}

inline char16_t hexToChar16(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3) {
  uint16_t result = 0;

  // Convert each character to its numerical equivalent
  result |= hexVal_inl(c0) << 12;
  result |= hexVal_inl(c1) << 8;
  result |= hexVal_inl(c2) << 4;
  result |= hexVal_inl(c3);

  return result;
}

template <typename StrType>
inline void appendChar32ToString(StrType& s, char32_t wc) {
  if (wc < 0x80) {
    s += static_cast<char>(wc);
  } else if (wc < 0x800) {
    s += static_cast<char>((wc >> 6) | 0xC0);
    s += static_cast<char>((wc & 0x3F) | 0x80);
  } else if (wc < 0x10000) {
    s += static_cast<char>((wc >> 12) | 0xE0);
    s += static_cast<char>(((wc >> 6) & 0x3F) | 0x80);
    s += static_cast<char>((wc & 0x3F) | 0x80);
  } else {
    s += static_cast<char>((wc >> 18) | 0xF0);
    s += static_cast<char>(((wc >> 12) & 0x3F) | 0x80);
    s += static_cast<char>(((wc >> 6) & 0x3F) | 0x80);
    s += static_cast<char>((wc & 0x3F) | 0x80);
  }
}

template <bool allowDecodeUTF8, typename StrType>
inline bool decodeJSONStringSequentially(
    const folly::ByteRange& input,
    folly::io::Cursor& fallBackInput,
    StrType& output,
    unsigned& inIdx,
    unsigned& skipped,
    unsigned limit,
    char16_t& highSurrogate) {
  unsigned t = inIdx + limit;
  while ((limit == 0 || inIdx < t) && inIdx < input.size()) {
    uint8_t ch = readOrFallback(input, fallBackInput, inIdx, skipped);
    if (ch == '"') {
      return true;
    }
    if (ch == '\\') {
      ch = readOrFallback(input, fallBackInput, inIdx, skipped);
      if (ch == 'u') {
        // skip first to chars (expected to be zero) and parse hex chars into
        // single char
        const uint8_t c0 = readOrFallback(input, fallBackInput, inIdx, skipped);
        if constexpr (!allowDecodeUTF8) {
          if (c0 != '0') {
            throw TProtocolException(
                TProtocolException::INVALID_DATA,
                "Expected ASCII char but got unicode char");
          }
        }
        const uint8_t c1 = readOrFallback(input, fallBackInput, inIdx, skipped);
        if constexpr (!allowDecodeUTF8) {
          if (c1 != '0') {
            throw TProtocolException(
                TProtocolException::INVALID_DATA,
                "Expected ASCII char but got unicode char");
          }
        }
        const uint8_t c2 = readOrFallback(input, fallBackInput, inIdx, skipped);
        const uint8_t c3 = readOrFallback(input, fallBackInput, inIdx, skipped);
        // do not inline this lookup into the function call, evaluation order of
        // args is undefined
        const char16_t ch1 = hexToChar16(c0, c1, c2, c3);

        if constexpr (allowDecodeUTF8) {
          if (highSurrogate != 0 &&
              folly::utf16_code_unit_is_low_surrogate(ch1)) {
            appendChar32ToString(
                output,
                folly::unicode_code_point_from_utf16_surrogate_pair(
                    highSurrogate, ch1));
            highSurrogate = 0;
          } else if (
              folly::utf16_code_unit_is_high_surrogate(ch1) &&
              inIdx < input.size()) {
            highSurrogate = ch1;
          } else {
            appendChar32ToString(output, ch1);
          }
        } else {
          appendChar32ToString(output, ch1);
        }
        continue;
      } else {
        ch = lookupJSONEscapeChar(ch);
      }
    }
    if constexpr (allowDecodeUTF8) {
      if (highSurrogate != 0) {
        appendChar32ToString(output, highSurrogate);
        highSurrogate = 0;
      }
    }
    output += (char)ch;
  }
  return false;
}

template <bool allowDecodeUTF8, typename StrType>
inline void decodeJSONStringRemainder(
    folly::io::Cursor& fallBackInput,
    StrType& output,
    char16_t& highSurrogate) {
  while (true) {
    auto ch = fallBackInput.read<uint8_t>();
    if (ch == '"') {
      return;
    }
    if (ch == '\\') {
      ch = fallBackInput.read<uint8_t>();
      if (ch == 'u') {
        auto peek = fallBackInput.peek();
        if (peek.size() < 4) {
          throw TProtocolException(
              TProtocolException::INVALID_DATA,
              "Read beginning of escaped unicode char, expected unicode hex but reached end of stream instead");
        }
        if constexpr (!allowDecodeUTF8) {
          if (peek[0] != '0' || peek[1] != '0') {
            throw TProtocolException(
                TProtocolException::INVALID_DATA,
                "Expected ASCII char but got unicode char");
          }
        }
        char16_t ch1 = hexToChar16(peek[0], peek[1], peek[2], peek[3]);
        fallBackInput.skip(4);

        if constexpr (allowDecodeUTF8) {
          if (highSurrogate != 0 &&
              folly::utf16_code_unit_is_low_surrogate(ch1)) {
            appendChar32ToString(
                output,
                folly::unicode_code_point_from_utf16_surrogate_pair(
                    highSurrogate, ch1));
            highSurrogate = 0;
          } else if (folly::utf16_code_unit_is_high_surrogate(ch1)) {
            if (highSurrogate != 0) {
              appendChar32ToString(output, highSurrogate);
            }
            highSurrogate = ch1;
          } else {
            appendChar32ToString(output, ch1);
          }
        } else {
          appendChar32ToString(output, ch1);
        }
        continue;
      } else {
        ch = lookupJSONEscapeChar(ch);
      }
    }
    if constexpr (allowDecodeUTF8) {
      if (highSurrogate != 0) {
        appendChar32ToString(output, highSurrogate);
        highSurrogate = 0;
      }
    }
    output += (char)ch;
  }
}

template <bool allowDecodeUTF8, typename StrType>
bool decodeJSONStringPeek(
    const folly::ByteRange& input,
    folly::io::Cursor& fallBackInput,
    StrType& output,
    bool& dataConsumed,
    char16_t& highSurrogate) {
  if (input.empty()) {
    throw TProtocolException(
        TProtocolException::INVALID_DATA,
        "Got empty input stream while decoding JSON String");
  }

  unsigned int i = 0;
  unsigned int skipped = 0;

  static const uint8x16_t endMask = vdupq_n_u8('"');
  static const uint8x16_t escapeMask = vdupq_n_u8('\\');
  uint8_t buf[16] ;

  bool stringTerminated{false};

  while (i + 15 < input.size()) {
    unsigned int j = 0;
    // load 16 consecutive chars into a vector register
    uint8x16_t val = vld1q_u8((uint8_t*)&input[i]);
    // check each char for equality to termination character
    uint8x16_t endRes = vceqq_u8(val, endMask); // holds 0 or 1 for each char
    // check each char for equality to the escape character
    uint8x16_t escRes = vceqq_u8(val, escapeMask);

    uint8x16_t res = vorrq_u8(endRes, escRes);
    uint64x2_t cmp = vreinterpretq_u64_u8(res);


    bool firstHalfRequiresSeqProcessing =
        vgetq_lane_u64(cmp, 0) != 0;
    bool secondHalfRequiresSeqProcessing =
        vgetq_lane_u64(cmp, 1) != 0;
    if constexpr (allowDecodeUTF8) {
      if (highSurrogate != 0 && !firstHalfRequiresSeqProcessing) {
        appendChar32ToString(output, highSurrogate);
        highSurrogate = 0;
      }
    }
    if (FOLLY_UNLIKELY(firstHalfRequiresSeqProcessing || secondHalfRequiresSeqProcessing)) {
      // the 16 char contains a JSON escape or termination character - we cannot
      // copy the input
      // If the first lane of the uint64x2_t is zero, the first 8 chars do not
      // contain a termination
      uint8_t limit = 16;
      if (!firstHalfRequiresSeqProcessing) {
        // Only the second half requires special processing, we can trivially
        // copy the first half and process the second half sequentially
        vst1_u8(buf, vget_low_u8(val));
        output.append((const char*)buf, 8);
        i += 8;
        skipped += 8;
        limit = 8;
      }
      stringTerminated |= decodeJSONStringSequentially<allowDecodeUTF8>(
          input, fallBackInput, output, i, skipped, limit, highSurrogate);
      if (stringTerminated) {
        goto done;
      }
    } else {
      // no escaping required. just copy the input

      /*
       * vst1q_u8(buf, val);
       * output.append((char*)&buf, 16);
       */
      output.append((char*)&val, 16);

      i += 16;
      skipped += 16;
    }
  }

done:
  if (skipped > 0) {
    fallBackInput.skip(skipped);
    skipped = 0;
    dataConsumed = true;
  } else {
    dataConsumed = false;
  }
  return stringTerminated;
}

template <bool allowDecodeUTF8, typename StrType>
inline void readJSONStringNeon(folly::io::Cursor& in_, StrType& out) {
  try {
    bool dataConsumed{true};
    char16_t highSurrogate{0};
    for (auto peek = in_.peekBytes(); !peek.empty() && dataConsumed;
         peek = in_.peekBytes()) {
      if (decodeJSONStringPeek<allowDecodeUTF8>(
          peek, in_, out, dataConsumed, highSurrogate)) {
        return;
      }
    }
    decodeJSONStringRemainder<allowDecodeUTF8>(in_, out, highSurrogate);
  } catch (std::out_of_range& ex) {
    throw TProtocolException(
        TProtocolException::INVALID_DATA,
        "Reached preliminary end of input stream");
  }
}
#endif // __ARM_NEON

template <typename StrType>
inline void JSONProtocolReaderCommon::readJSONString(StrType& val) {
  ensureChar(apache::thrift::detail::json::kJSONStringDelimiter);
  val.clear();
#ifdef __ARM_NEON
  if (allowDecodeUTF8_) {
    readJSONStringNeon<true>(in_, val);
  } else {
    readJSONStringNeon<false>(in_, val);
  }
#else // __ARM_NEON
  std::string json = "\"";
  bool fullDecodeRequired = false;
  while (true) {
    auto ch = in_.read<uint8_t>();
    if (ch == apache::thrift::detail::json::kJSONStringDelimiter) {
      break;
    }
    if (ch == apache::thrift::detail::json::kJSONBackslash) {
      ch = in_.read<uint8_t>();
      if (ch == apache::thrift::detail::json::kJSONEscapeChar) {
        if (allowDecodeUTF8_) {
          json += "\\u";
          fullDecodeRequired = true;
          continue;
        } else {
          readJSONEscapeChar(ch);
        }
      } else {
        size_t pos = kEscapeChars().find_first_of(ch);
        if (pos == std::string::npos) {
          throwInvalidEscapeChar(ch);
        }
        if (fullDecodeRequired) {
          json += "\\";
          json += ch;
          continue;
        } else {
          ch = kEscapeCharVals[pos];
        }
      }
    }

    if (fullDecodeRequired) {
      json += ch;
    } else {
      val += ch;
    }
  }

  if (fullDecodeRequired) {
    json += "\"";
    try {
      folly::dynamic parsed = folly::parseJson(json);
      val += parsed.getString();
    } catch (const std::exception& e) {
      throwUnrecognizableAsString(json, e);
    }
  }
#endif // __ARM_NEON
}

inline void JSONProtocolReaderCommon::readJSONBase64(
    folly::io::QueueAppender& s) {
#ifdef __ARM_NEON
  readJSONBase64Neon(s);
#else // __ARM_NEON
  std::string tmp;
  readJSONString(tmp);

  uint8_t* b = (uint8_t*)tmp.c_str();
  uint32_t len = folly::to_narrow(tmp.length());

  // Allow optional trailing '=' as padding
  while (len > 0 && b[len - 1] == '=') {
    --len;
  }

  while (len >= 4) {
    base64_decode(b, 4);
    s.push(b, 3);
    b += 4;
    len -= 4;
  }
  // Don't decode if we hit the end or got a single leftover byte (invalid
  // base64 but legal for skip of regular string type)
  if (len > 1) {
    base64_decode(b, len);
    s.push(b, len - 1);
  }
#endif
}

// Return the integer value of a hex character ch.
// Throw a protocol exception if the character is not [0-9a-f].
inline uint8_t JSONProtocolReaderCommon::hexVal(uint8_t ch) {
  if ((ch >= '0') && (ch <= '9')) {
    return ch - '0';
  } else if ((ch >= 'a') && (ch <= 'f')) {
    return ch - 'a' + 10;
  } else {
    throwInvalidHexChar(ch);
  }
}

template <class Predicate>
uint32_t JSONProtocolReaderCommon::readWhile(
    const Predicate& pred, std::string& out) {
  uint32_t ret = 0;
  for (auto peek = in_.peekBytes(); !peek.empty(); peek = in_.peekBytes()) {
    uint32_t size = 0;
    for (uint8_t ch : peek) {
      if (!pred(ch)) {
        out.append(peek.begin(), peek.begin() + size);
        in_.skip(size);
        return ret + size;
      }
      ++size;
    }
    out.append(peek.begin(), peek.end());
    ret += size;
    in_.skip(size);
  }
  return ret;
}

inline int8_t JSONProtocolReaderCommon::peekCharSafe() {
  auto peek = in_.peekBytes();
  return peek.empty() ? 0 : *peek.data();
}

} // namespace thrift
} // namespace apache