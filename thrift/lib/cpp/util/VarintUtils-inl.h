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

#ifdef __BMI2__
#include <immintrin.h>
#endif

#include <array>

#include <folly/Utility.h>
#include <folly/io/Cursor.h>
#include <folly/lang/Bits.h>

namespace apache {
namespace thrift {

namespace util {

namespace detail {

template <
    class T,
    class CursorT,
    typename std::enable_if<
        std::is_constructible<folly::io::Cursor, const CursorT&>::value,
        bool>::type = false>
void readVarintSlow(CursorT& c, T& value) {
  // ceil(sizeof(T) * 8) / 7
  static const size_t maxSize = (8 * sizeof(T) + 6) / 7;
  T retVal = 0;
  uint8_t shift = 0;
  uint8_t rsize = 0;
  while (true) {
    uint8_t byte = c.template read<uint8_t>();
    rsize++;
    retVal |= (uint64_t)(byte & 0x7f) << shift;
    shift += 7;
    if (!(byte & 0x80)) {
      value = retVal;
      return;
    }
    if (rsize >= maxSize) {
      // Too big for return type
      throw std::out_of_range("invalid varint read");
    }
  }
}

// This is a simple function that just throws an exception. It is defined out
// line to make the caller (readVarint) smaller and simpler (assembly-wise),
// which gives us 5% perf win (even when the exception is not actually thrown).
[[noreturn]] void throwInvalidVarint();

template <
    class T,
    class CursorT,
    typename std::enable_if<
        std::is_constructible<folly::io::Cursor, const CursorT&>::value,
        bool>::type = false>
void readVarintMediumSlow(CursorT& c, T& value, const uint8_t* p, size_t len) {
  enum { maxSize = (8 * sizeof(T) + 6) / 7 };

  // check that the available data is more than the longest possible varint or
  // that the last available byte ends a varint
  if (LIKELY(len >= maxSize || (len > 0 && !(p[len - 1] & 0x80)))) {
    uint64_t result;
    const uint8_t* start = p;
    do {
      uint64_t byte; // byte is uint64_t so that all shifts are 64-bit
      // clang-format off
      byte = *p++; result  = (byte & 0x7f);       if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) <<  7; if (!(byte & 0x80)) break;
      if (sizeof(T) <= 1) throwInvalidVarint();
      byte = *p++; result |= (byte & 0x7f) << 14; if (!(byte & 0x80)) break;
      if (sizeof(T) <= 2) throwInvalidVarint();
      byte = *p++; result |= (byte & 0x7f) << 21; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 28; if (!(byte & 0x80)) break;
      if (sizeof(T) <= 4) throwInvalidVarint();
      byte = *p++; result |= (byte & 0x7f) << 35; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 42; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 49; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 56; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 63; if (!(byte & 0x80)) break;
      // clang-format on
      throwInvalidVarint();
    } while (false);
    value = static_cast<T>(result);
    c.skipNoAdvance(p - start);
  } else {
    readVarintSlow<T, CursorT>(c, value);
  }
}

} // namespace detail

template <
    class T,
    class CursorT,
    typename std::enable_if<
        std::is_constructible<folly::io::Cursor, const CursorT&>::value,
        bool>::type = false>
void readVarint(CursorT& c, T& value) {
  const uint8_t* p = c.data();
  size_t len = c.length();
  if (len > 0 && !(*p & 0x80)) {
    value = static_cast<T>(*p);
    c.skipNoAdvance(1);
  } else {
    apache::thrift::util::detail::readVarintMediumSlow<T, CursorT>(
        c, value, p, len);
  }
}

template <
    class T,
    class CursorT,
    typename std::enable_if<
        std::is_constructible<folly::io::Cursor, const CursorT&>::value,
        bool>::type = false>
T readVarint(CursorT& c) {
  T value;
  readVarint<T, CursorT>(c, value);
  return value;
}

namespace detail {

// Cursor class must have ensure() and append() (e.g. QueueAppender)
template <class Cursor, class T>
uint8_t writeVarintSlow(Cursor& c, T value) {
  enum { maxSize = (8 * sizeof(T) + 6) / 7 };
  typedef typename std::make_unsigned<T>::type un_type;
  un_type unval = static_cast<un_type>(value);

  c.ensure(maxSize);

  uint8_t* p = c.writableData();
  uint8_t* orig_p = p;
  // precondition: (value & ~0x7f) != 0
  do {
    // clang-format off
    *p++ = ((unval & 0x7f) | 0x80); unval = unval >> 7; if ((unval & ~0x7f) == 0) break;
    *p++ = ((unval & 0x7f) | 0x80); unval = unval >> 7; if ((unval & ~0x7f) == 0) break;
    *p++ = ((unval & 0x7f) | 0x80); unval = unval >> 7; if ((unval & ~0x7f) == 0) break;
    *p++ = ((unval & 0x7f) | 0x80); unval = unval >> 7; if ((unval & ~0x7f) == 0) break;
    *p++ = ((unval & 0x7f) | 0x80); unval = unval >> 7; if ((unval & ~0x7f) == 0) break;
    *p++ = ((unval & 0x7f) | 0x80); unval = unval >> 7; if ((unval & ~0x7f) == 0) break;
    *p++ = ((unval & 0x7f) | 0x80); unval = unval >> 7; if ((unval & ~0x7f) == 0) break;
    *p++ = ((unval & 0x7f) | 0x80); unval = unval >> 7; if ((unval & ~0x7f) == 0) break;
    *p++ = ((unval & 0x7f) | 0x80); unval = unval >> 7;
    // clang-format on
  } while (false);

  *p++ = static_cast<uint8_t>(unval);
  c.append(p - orig_p);
  return static_cast<uint8_t>(p - orig_p);
}

} // namespace detail

template <class Cursor, class T>
uint8_t writeVarintUnrolled(Cursor& c, T value) {
  if (LIKELY((value & ~0x7f) == 0)) {
    c.template write<uint8_t>(static_cast<uint8_t>(value));
    return 1;
  }

  return apache::thrift::util::detail::writeVarintSlow<Cursor, T>(c, value);
}

#ifdef __BMI2__

template <class Cursor, class T>
uint8_t writeVarintBMI2(Cursor& c, T valueS) {
  auto value = folly::to_unsigned(valueS);
  if (LIKELY((value & ~0x7f) == 0)) {
    c.template write<uint8_t>(static_cast<uint8_t>(value));
    return 1;
  }

  if (sizeof(T) == 1) {
    c.template write<uint16_t>(value | 0x100);
    return 2;
  }

  constexpr uint64_t kMask = 0x8080808080808080ULL;
  static constexpr std::array<uint8_t, 64> kShift = []() {
    std::array<uint8_t, 64> v = {};
    for (size_t i = 0; i < 64; i++) {
      uint8_t byteShift = i == 0 ? 0 : (8 - ((63 - i) / 7));
      v[i] = byteShift * 8;
    }
    return v;
  }();
  static constexpr std::array<uint8_t, 64> kSize = []() {
    std::array<uint8_t, 64> v = {};
    for (size_t i = 0; i < 64; i++) {
      v[i] = ((63 - i) / 7) + 1;
    }
    return v;
  }();

  auto clzll = __builtin_clzll(static_cast<uint64_t>(value));
  // Only the first 56 bits of @value will be deposited in @v.
  uint64_t v = _pdep_u64(value, ~kMask) | (kMask >> kShift[clzll]);
  uint8_t size = kSize[clzll];

  if (sizeof(T) < sizeof(uint64_t)) {
    c.template write<uint64_t>(v, size);
  } else {
    // Ensure max encoding space for u64 varint (10 bytes).
    // Write 56 bits using pdep and the other 8 bits manually.
    // Write 10B to @c, but only update the size using @size.
    // (Writing exta bytes is faster than branching based on @size.)
    c.ensure(10);
    uint8_t* p = c.writableData();
    folly::storeUnaligned<uint64_t>(p, v);
    p[sizeof(uint64_t) + 0] = static_cast<uint64_t>(value) >> 56;
    p[sizeof(uint64_t) + 1] = 1;
    c.append(size);
  }
  return size;
}

template <class Cursor, class T>
uint8_t writeVarint(Cursor& c, T value) {
  return writeVarintBMI2(c, value);
}

#else // __BMI2__

template <class Cursor, class T>
uint8_t writeVarint(Cursor& c, T value) {
  return writeVarintUnrolled(c, value);
}

#endif // __BMI2__

inline int32_t zigzagToI32(uint32_t n) {
  return (n & 1) ? ~(n >> 1) : (n >> 1);
}

inline int64_t zigzagToI64(uint64_t n) {
  return (n & 1) ? ~(n >> 1) : (n >> 1);
}

constexpr inline uint32_t i32ToZigzag(const int32_t n) {
  return (static_cast<uint32_t>(n) << 1) ^ static_cast<uint32_t>(n >> 31);
}

constexpr inline uint64_t i64ToZigzag(const int64_t l) {
  return (static_cast<uint64_t>(l) << 1) ^ static_cast<uint64_t>(l >> 63);
}

} // namespace util
} // namespace thrift
} // namespace apache
