/*
 * Copyright (c) 2015-2018, Wojciech Muła. All rights reserved.
 * Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.


  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#ifdef __ARM_NEON

#define packed_byte(x) vdup_n_u8(x)

FOLLY_ALWAYS_INLINE uint8x8_t
lookup_pshufb_bitmask(const uint8x8_t input, uint8x8_t& error) {
  const uint8x8_t higher_nibble = vshr_n_u8(input, 4);
  const uint8x8_t lower_nibble = vand_u8(input, packed_byte(0x0f));

  const uint8x8x2_t shiftLUT = {
    0, 0, 19, 4, uint8_t(-65), uint8_t(-65), uint8_t(-71), uint8_t(-71),
    0, 0, 0, 0, 0, 0, 0, 0};

  const uint8x8x2_t maskLUT = {
      /* 0        : 0b1010_1000*/ 0xa8,
      /* 1 .. 9   : 0b1111_1000*/ 0xf8,
      0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8,
      0xf8, 0xf8,
      /* 10       : 0b1111_0000*/ 0xf0,
      /* 11       : 0b0101_0100*/ 0x54,
      /* 12 .. 14 : 0b0101_0000*/ 0x50,
      0x50,
      0x50,
      /* 15       : 0b0101_0100*/ 0x54};

  const uint8x8x2_t bitposLUT = {
      0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

  const uint8x8_t sh = vtbl2_u8(shiftLUT, higher_nibble);
  const uint8x8_t eq_2f = vceq_u8(input, packed_byte(0x2f));
  const uint8x8_t shift = vbsl_u8(eq_2f, packed_byte(16), sh);

  const uint8x8_t M = vtbl2_u8(maskLUT, lower_nibble);
  const uint8x8_t bit = vtbl2_u8(bitposLUT, higher_nibble);

  error = vceq_u8(vand_u8(M, bit), packed_byte(0));

  const uint8x8_t result = vadd_u8(input, shift);

  return result;
}

inline bool isBase64Terminating(const uint8x8_t field) {
  static const uint8x8_t delimPattern =
      vdup_n_u8(apache::thrift::detail::json::kJSONStringDelimiter);
  static const uint8x8_t padPattern = vdup_n_u8('=');

  const uint8x8_t delimMatch = vceq_u8(field, delimPattern);
  const uint8x8_t padMatch = vceq_u8(field, padPattern);

  return vget_lane_u64(vreinterpret_u64_u8(delimMatch), 0) != 0 ||
      vget_lane_u64(vreinterpret_u64_u8(padMatch), 0) != 0;
}

static const uint8_t kBase64DecodeTable[256] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
  0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
  0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
  0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24,
  0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
  0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,
};


inline void JSONProtocolReaderCommon::readJSONBase64Neon(folly::io::QueueAppender& s) {
     ensureCharNoWhitespace(apache::thrift::detail::json::kJSONStringDelimiter);
  for (auto peek = in_.peekBytes(); !peek.empty(); peek = in_.peekBytes()) {
    uint8_t buf[4];
    buf[3] = '\0';
    size_t skipped = 0;
    int i = 0;
    for (; i + 4 * 8 - 1 < peek.size(); i += 4 * 8) {
      const uint8x8x4_t in = vld4_u8((const uint8_t*)&peek[0] + i);
      s.ensure(3 * 8);

      if (isBase64Terminating(in.val[0]) || isBase64Terminating(in.val[1]) ||
          isBase64Terminating(in.val[2]) || isBase64Terminating(in.val[3])) {
        if (skipped > 0)
          in_.skip(skipped);
        goto remainder;
      }
      uint8x8_t error_a;
      uint8x8_t error_b;
      uint8x8_t error_c;
      uint8x8_t error_d;
#define lookup_fn lookup_pshufb_bitmask

      uint8x8_t field_a = lookup_fn(in.val[0], error_a);
      uint8x8_t field_b = lookup_fn(in.val[1], error_b);
      uint8x8_t field_c = lookup_fn(in.val[2], error_c);
      uint8x8_t field_d = lookup_fn(in.val[3], error_d);

      const uint8x8_t error =
          vorr_u8(vorr_u8(error_a, error_b), vorr_u8(error_c, error_d));

      const uint64_t scalarError = vget_lane_u64(vreinterpret_u64_u8(error), 0);
      if (scalarError) {
          LOG(FATAL) << "Error decoding base64" <<  scalarError;
      }

      uint8x8x3_t result;
      result.val[0] = vorr_u8(vshr_n_u8(field_b, 4), vshl_n_u8(field_a, 2));
      result.val[1] = vorr_u8(vshr_n_u8(field_c, 2), vshl_n_u8(field_b, 4));
      result.val[2] = vorr_u8(field_d, vshl_n_u8(field_c, 6));

      vst3_u8(s.writableData(), result);
      s.append(3 * 8);
      skipped += 4 * 8;
    }
    for (; i + 3 < peek.size(); i += 4) {
      if (peek[i] == apache::thrift::detail::json::kJSONStringDelimiter ||
          peek[i] == '=' ||
          peek[i + 1] == apache::thrift::detail::json::kJSONStringDelimiter ||
          peek[i + 1] == '=' ||
          peek[i + 2] == apache::thrift::detail::json::kJSONStringDelimiter ||
          peek[i + 2] == '=' ||
          peek[i + 3] == apache::thrift::detail::json::kJSONStringDelimiter ||
          peek[i + 3] == '=') {
        if (skipped > 0) {
          in_.skip(skipped);
        }
        goto remainder;
      }
      s.ensure(3);
      *s.writableData() = (kBase64DecodeTable[peek[i]] << 2) |
          (kBase64DecodeTable[peek[i + 1]] >> 4);
      *(s.writableData() + 1) =
          ((kBase64DecodeTable[peek[i + 1]] << 4) & 0xf0) |
          (kBase64DecodeTable[peek[i + 2]] >> 2);
      *(s.writableData() + 2) =
          ((kBase64DecodeTable[peek[i + 2]] << 6) & 0xc0) |
          (kBase64DecodeTable[peek[i + 3]]);
      s.append(3);
      skipped += 4;
    }
    if (skipped == 0)
      break;
    in_.skip(skipped);
  }

remainder:
  uint8_t input[4];
  uint8_t inputBufSize = 0;
  // remainder
  bool paddingReached{false};
  while (true) {
    const auto c = in_.read<uint8_t>();
    if (c == apache::thrift::detail::json::kJSONStringDelimiter) {
      break;
    }
    if (paddingReached) {
      continue;
    }
    if (c == '=') {
      paddingReached = true;
      continue;
    }
    input[inputBufSize++] = c;
    if (inputBufSize == 4) {
      base64_decode(input, 4);
      input[3] = '\0';
      s.push(input, 3);
      inputBufSize = 0;
    }
  }

  if (inputBufSize > 1) {
    base64_decode(input, inputBufSize);
    input[inputBufSize - 1] = '\0';
    s.push(input, inputBufSize - 1);
  }
  }
#endif // __ARM_NEON
} // namespace thrift
} // namespace apache