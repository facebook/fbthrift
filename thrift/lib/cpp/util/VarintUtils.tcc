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

template <class T, class CursorT,
          typename std::enable_if<
            std::is_constructible<folly::io::Cursor, const CursorT&>::value,
            bool>::type = false>
uint8_t readVarint(CursorT& c, T& value) {
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
