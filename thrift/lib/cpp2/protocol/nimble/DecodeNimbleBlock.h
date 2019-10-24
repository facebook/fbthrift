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
#include <cstdint>

#include <folly/GLog.h>
#include <folly/Range.h>
#include <folly/lang/Bits.h>

#include <thrift/lib/cpp2/protocol/nimble/ChunkRepr.h>
#include <thrift/lib/cpp2/protocol/nimble/Vectorization.h>

// See EncodeNimbleBlock.h to see why we work here in terms of "blocks" of four
// chunks each.
//
// The fact that a block's control data is stored within a single byte makes it
// faster and more convenient to decode the whole block at once. Here, we define
// such a decoding function.

namespace apache {
namespace thrift {
namespace detail {

struct OffsetAndMaskShift {
  constexpr OffsetAndMaskShift() : offsetOrSize(0), maskShift(0) {}

  // As a trick, we know the offset for chunk 0 is always 0, so we put the sum
  // of all sizes there to save space.
  std::uint8_t offsetOrSize;
  std::uint8_t maskShift;
};

struct NimbleBlockDecodeData {
  constexpr NimbleBlockDecodeData() : data{} {}

  OffsetAndMaskShift data[4];
};

struct NimbleBlockVectorDecodeData {
  constexpr NimbleBlockVectorDecodeData() : size{}, shuffleVector{} {}

  std::uint8_t size;
  // We rely on the shuffling that goes on here to get endianness right for the
  // underlying platform. For now we're using x86 vector instructions, so we
  // know we're little-endian.
  unsigned char shuffleVector[16];
};

extern const std::array<NimbleBlockDecodeData, 256> nimbleBlockDecodeData;
extern const std::array<NimbleBlockVectorDecodeData, 256>
    nimbleBlockVectorDecodeData;

// This function takes the control byte for the block, and a pointer to the
// associated data (which must contain at least 16 valid bytes), and places the
// decoded data into chunksOut. The return value is the number of bytes consumed
// from the input stream.
template <ChunkRepr repr, bool tryVectorize = true>
int decodeNimbleBlock(
    std::uint8_t control,
    folly::ByteRange data,
    std::uint32_t chunksOut[4]) {
  DCHECK(data.size() >= 16);
#if APACHE_THRIFT_DETAIL_NIMBLE_CAN_VECTORIZE
  static_assert(
      folly::kIsLittleEndian,
      "Vector shuffle output assumes little-endian order");
  if (tryVectorize) {
    // We need the preprocessor 'if' so that this file can still compile even
    // when the underlying intrinsics are unavailable. We need the
    // language-level 'if' to let us turn off vectorization so that we can test
    // both implementations.

    const NimbleBlockVectorDecodeData& decodeData =
        nimbleBlockVectorDecodeData[control];

    // This is the stream vbyte stage.
    __m128i input =
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(data.data()));
    __m128i shuf = _mm_loadu_si128(
        reinterpret_cast<const __m128i*>(&decodeData.shuffleVector));
    __m128i output = _mm_shuffle_epi8(input, shuf);

    // Note: no endianness conversion; see the note on the definition of
    // shuffleVector.

    if (repr == ChunkRepr::kZigzag) {
      __m128i zero = _mm_setzero_si128();
      __m128i one = _mm_set1_epi32(1);

      // shifted = n >> 1
      __m128i shifted = _mm_srli_epi32(output, 1);
      // anded = n & 1
      __m128i anded = _mm_and_si128(output, one);
      // negated = -(n & 1)
      __m128i negated = _mm_sub_epi32(zero, anded);
      // output = (n >> 1) ^ -(n & 1)
      output = _mm_xor_si128(shifted, negated);
    }

    _mm_storeu_si128(reinterpret_cast<__m128i*>(chunksOut), output);
    return decodeData.size;
  }
#endif
  // Note: early exit in the branch above; this never runs in the common path
  // where we vectorize.
  for (int i = 0; i < 4; ++i) {
    const OffsetAndMaskShift& metadata = nimbleBlockDecodeData[control].data[i];
    // We may shift this by 32 (if the chunk is 0), so it needs to have a size
    // greater than 32 bits.
    const std::uint64_t mask = (std::uint32_t)-1;
    int offset = (i == 0 ? 0 : metadata.offsetOrSize);
    chunksOut[i] = chunkFromWireRepr<repr>(
        folly::loadUnaligned<std::uint32_t>(&data[offset]) &
        (mask >> metadata.maskShift));
  }
  return nimbleBlockDecodeData[control].data[0].offsetOrSize;
}

} // namespace detail
} // namespace thrift
} // namespace apache
