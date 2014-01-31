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

#include "thrift/lib/cpp/util/VarintUtils.h"
#include "thrift/lib/cpp/TApplicationException.h"

#include <stdint.h>

namespace apache { namespace thrift { namespace util {

/**
 * Read an i16 from the wire as a varint. The MSB of each byte is set
 * if there is another byte to follow. This can read up to 3 bytes.
 */
uint32_t readVarint16(uint8_t const* ptr, int16_t* i16,
                      uint8_t const* boundary) {
  int64_t val;
  uint32_t rsize = readVarint64(ptr, &val, boundary);
  *i16 = (int16_t)val;
  return rsize;
}

/**
 * Read an i32 from the wire as a varint. The MSB of each byte is set
 * if there is another byte to follow. This can read up to 5 bytes.
 */
uint32_t readVarint32(uint8_t const* ptr, int32_t* i32,
                      uint8_t const* boundary) {
  int64_t val;
  uint32_t rsize = readVarint64(ptr, &val, boundary);
  *i32 = (int32_t)val;
  return rsize;
}

/**
 * Read an i64 from the wire as a proper varint. The MSB of each byte is set
 * if there is another byte to follow. This can read up to 10 bytes.
 * Caller is responsible for advancing ptr after call.
 */
uint32_t readVarint64(uint8_t const* ptr, int64_t* i64,
                      uint8_t const* boundary) {
  uint32_t rsize = 0;
  uint64_t val = 0;
  int shift = 0;
  uint8_t buf[10];  // 64 bits / (7 bits/byte) = 10 bytes.
  uint32_t buf_size = sizeof(buf);

  while (true) {
    if (ptr == boundary) {
      throw TApplicationException(
        TApplicationException::INVALID_MESSAGE_TYPE,
        "Trying to read past header boundary");
    }
    uint8_t byte = *(ptr++);
    rsize++;
    val |= (uint64_t)(byte & 0x7f) << shift;
    shift += 7;
    if (!(byte & 0x80)) {
      *i64 = val;
      return rsize;
    }
  }
}

/**
 * Write an i32 as a varint. Results in 1-5 bytes on the wire.
 */
uint32_t writeVarint32(uint32_t n, uint8_t* pkt) {
  uint8_t buf[5];
  uint32_t wsize = 0;

  while (true) {
    if ((n & ~0x7F) == 0) {
      buf[wsize++] = (int8_t)n;
      break;
    } else {
      buf[wsize++] = (int8_t)((n & 0x7F) | 0x80);
      n >>= 7;
    }
  }

  // Caller will advance pkt.
  for (int i = 0; i < wsize; i++) {
    pkt[i] = buf[i];
  }

  return wsize;
}

uint32_t writeVarint16(uint16_t n, uint8_t* pkt) {
  return writeVarint32(n, pkt);
}

uint32_t i32ToZigzag(const int32_t n) {
  return (n << 1) ^ (n >> 31);
}

uint64_t i64ToZigzag(const int64_t l) {
  return (l << 1) ^ (l >> 63);
}

int32_t zigzagToI32(uint32_t n) {
  return (n >> 1) ^ -(n & 1);
}

int64_t zigzagToI64(uint64_t n) {
  return (n >> 1) ^ -(n & 1);
}

}}} // apache::thrift::util
