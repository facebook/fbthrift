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

#include <folly/io/Cursor.h>

namespace apache { namespace thrift {

namespace util {

namespace detail {

template <class T, class CursorT,
          typename std::enable_if<
            std::is_constructible<folly::io::Cursor, const CursorT&>::value,
            bool>::type = false>
uint8_t readVarintSlow(CursorT& c, T& value) {
  // ceil(sizeof(T) * 8) / 7
  static const size_t maxSize = (8 * sizeof(T) + 6) / 7;
  T retVal = 0;
  uint8_t shift = 0;
  uint8_t rsize = 0;
  while (true) {
    uint8_t byte = c.pullByte();
    rsize++;
    retVal |= (uint64_t)(byte & 0x7f) << shift;
    shift += 7;
    if (!(byte & 0x80)) {
      value = retVal;
      return rsize;
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
void throwInvalidVarint();

} // namespace detail

template <class T, class CursorT,
          typename std::enable_if<
            std::is_constructible<folly::io::Cursor, const CursorT&>::value,
            bool>::type = false>
uint8_t readVarint(CursorT& c, T& value) {
  enum { maxSize = (8 * sizeof(T) + 6) / 7 };

  const uint8_t* p = c.data();
  size_t len = c.length();
  if (LIKELY(len > 0 && !(*p & 0x80))) {
    value = static_cast<T>(*p);
    c.skip(1);
    return 1;
  }

  // check that the available data is more than the longest possible varint or
  // that the last available byte ends a varint
  if (LIKELY(len >= maxSize || (len > 0 && !(p[len - 1] & 0x80)))) {
    uint64_t result;
    const uint8_t* start = p;
    do {
      uint64_t byte; // byte is uint64_t so that all shifts are 64-bit
      byte = *p++; result  = (byte & 0x7f);       if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) <<  7; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 14; if (!(byte & 0x80)) break;
      if (sizeof(T) <= 2) detail::throwInvalidVarint();
      byte = *p++; result |= (byte & 0x7f) << 21; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 28; if (!(byte & 0x80)) break;
      if (sizeof(T) <= 4) detail::throwInvalidVarint();
      byte = *p++; result |= (byte & 0x7f) << 35; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 42; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 49; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 56; if (!(byte & 0x80)) break;
      byte = *p++; result |= (byte & 0x7f) << 63; if (!(byte & 0x80)) break;
      detail::throwInvalidVarint();
    } while (false);
    value = static_cast<T>(result);
    c.skip(p - start);
    return p - start;
  } else {
    return detail::readVarintSlow<T, CursorT>(c, value);
  }
}

template <class T, class CursorT,
          typename std::enable_if<
            std::is_constructible<folly::io::Cursor, const CursorT&>::value,
            bool>::type = false>
T readVarint(CursorT& c) {
  T value;
  readVarint<T, CursorT>(c, value);
  return value;
}

template <class Cursor, class T>
uint8_t writeVarint(Cursor& c, T value) {
  uint8_t sz = 0;
  while (true) {
    if ((value & ~0x7F) == 0) {
      c.template write<uint8_t>((int8_t)value);
      sz++;
      break;
    } else {
      c.template write<uint8_t>((int8_t)((value & 0x7F) | 0x80));
      sz++;
      typedef typename std::make_unsigned<T>::type un_type;
      value = (un_type)value >> 7;
    }
  }

  return sz;
}

}}} // apache::thrift::util
