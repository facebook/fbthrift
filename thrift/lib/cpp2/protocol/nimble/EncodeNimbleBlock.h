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

#include <folly/lang/Bits.h>

#include <thrift/lib/cpp2/protocol/nimble/ChunkRepr.h>
#include <thrift/lib/cpp2/protocol/nimble/ControlBitHelpers.h>
#include <thrift/lib/cpp2/protocol/nimble/Vectorization.h>

namespace apache {
namespace thrift {
namespace detail {

// In chunked stream representations, we compress chunks so that small values
// take up fewer wire bytes. The control data (indicating the mapping between
// data and its sizes) is contained in a separate stream, with the data for four
// chunks contained in a byte (in two-bit pairs). We call such a set of 4 chunks
// a "block".
//
// The fact that a block's control data is stored within a single byte makes it
// faster and more convenient to decode the whole block at once. This file
// defines functions to encode a group of 4 chunks into a block (i.e.
// a byte emitted to a control stream, and some number of bytes emitted to a
// data stream).

struct NimbleBlockEncodeData {
  constexpr NimbleBlockEncodeData() : sum(0), sizes{0} {}
  std::uint8_t sum;
  std::uint8_t sizes[kChunksPerBlock];
};

struct NimbleBlockVectorEncodeData {
  constexpr NimbleBlockVectorEncodeData() : shuffleVector{} {}
  unsigned char shuffleVector[kMaxBytesPerBlock];
};

extern const std::array<NimbleBlockEncodeData, 256> nimbleBlockEncodeData;
extern const std::array<NimbleBlockVectorEncodeData, 256>
    nimbleBlockVectorEncodeData;

#if APACHE_THRIFT_DETAIL_NIMBLE_CAN_VECTORIZE
// The vectorization code below is based on the reference StreamVByte
// implementation at https://github.com/lemire/streamvbyte
template <ChunkRepr repr>
inline __m128i loadVector(const std::uint32_t chunks[kChunksPerBlock]) {
  static_assert(
      kChunksPerBlock == 4, "Vectorized implementation assumes 4-chunk blocks");

  __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(chunks));
  if (repr == ChunkRepr::kZigzag) {
    // left = n << 1
    __m128i left = _mm_slli_epi32(input, 1);
    // right = n >> 31, note that this is an arithmetic shift
    __m128i right = _mm_srai_epi32(input, 31);
    // block = (n << 1) ^ (n >> 31)
    input = _mm_xor_si128(left, right);
  }
  return input;
}

// Extracts control byte from 4-chunk block. Expects that the input data
// is already encoded to the right ChunkRepr representation if necessary.
// The code below is based on the reference StreamVByte implementation
// at https://github.com/lemire/streamvbyte
inline unsigned char controlByteFromVector(const __m128i& input) {
  // extracting control byte
  const __m128i ones = _mm_set1_epi32(0x01010101);
  const __m128i twos = _mm_set1_epi32(0x02020202);
  const __m128i bitsMoveMultiple = _mm_set1_epi32(0x08040102);
  const __m128i codesPackingVector = _mm_set_epi32(0, 0, 0, 0x0D090501);
  const __m128i codesPackingMultiple = _mm_set_epi32(0, 0, 0, 0x10400104);

  // tmp has 1 in bytes that originally were non-zero
  __m128i tmp = _mm_min_epu8(input, ones);
  // moves non zero bits 0, 8, 16, 24 to bits 8, 9, 10 and 11 respectively in
  // each 32 bit word; bytes 13, 9, 5 and 1 now represent intermediate codes,
  // the rest is insignificant
  tmp = _mm_madd_epi16(tmp, bitsMoveMultiple);
  // translates intermediate values into 2-bit codes
  // 0  -> 0
  // 1  -> 1
  // 2  -> 2
  // 3  -> 2
  // 4+ -> 3
  // first, leave only 0, 1 and 2 for all possible values
  __m128i tmpMaxTwos = _mm_min_epu8(tmp, twos);
  // then, add "1" if any of higher bits were originally set
  // by right shifting by 2 bits first
  tmp = _mm_srli_epi16(tmp, 2);
  // then leaving 1 if the value is non-zero
  tmp = _mm_min_epu8(tmp, ones);
  // and merging the 1 back
  tmp = _mm_or_si128(tmp, tmpMaxTwos);

  // packs bytes 13, 9, 5 and 1 in the least significant 32 bit word
  tmp = _mm_shuffle_epi8(tmp, codesPackingVector);
  // packs 2-bit codes in byte 1
  tmp = _mm_madd_epi16(tmp, codesPackingMultiple);

  unsigned short result = _mm_extract_epi16(tmp, 0);
  return (unsigned char)(result >> 8);
}
#endif

template <ChunkRepr repr>
inline unsigned char controlByteFromChunks(
    const std::uint32_t chunks[kChunksPerBlock]) {
  // Computes the pair of control byte bits associated with the given input
  // chunk.
  auto computeEnum = [](std::uint32_t input) {
    if (repr == ChunkRepr::kZigzag) {
      input = zigzagEncode(input);
    }
    int lastByteSet = (folly::findLastSet(input) + 7) / 8;
    // The function that converts byte lengths to a pair of control bits is:
    //   0 -> 0
    //   1 -> 1
    //   2 -> 2
    //   3 -> 3
    //   4 -> 3
    // We can get this with f(x) = x - floor(x/4).
    return lastByteSet - (lastByteSet >> 2);
  };
  unsigned char result = 0;
  for (int i = 0; i < kChunksPerBlock; ++i) {
    result |= (computeEnum(chunks[i]) << 2 * i);
  }
  return result;
}

// Encodes the 4 chunks indicated by `chunks`, filling in controlOut and dataOut
// as appropriate. We may overwrite up to 16 bytes of dataOut, but only some of
// those overwitten bytes are significant (in the sense that they need to be
// part of the final stream). We return the number of significant bytes written;
// subsequent encoded blocks should begin at the returned offset.
template <ChunkRepr repr, bool tryVectorize = true>
int encodeNimbleBlock(
    const std::uint32_t chunks[kChunksPerBlock],
    std::uint8_t* controlOut,
    unsigned char* dataOut) {
#if APACHE_THRIFT_DETAIL_NIMBLE_CAN_VECTORIZE
  static_assert(
      folly::kIsLittleEndian,
      "Vector shuffle output assumes little-endian order");
  if (tryVectorize) {
    __m128i input = loadVector<repr>(chunks);

    unsigned char control = controlByteFromVector(input);
    *controlOut = control;

    __m128i shuf = _mm_loadu_si128(reinterpret_cast<const __m128i*>(
        nimbleBlockVectorEncodeData[control].shuffleVector));
    __m128i output = _mm_shuffle_epi8(input, shuf);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dataOut), output);

    return nimbleBlockEncodeData[control].sum;
  }
#endif

  std::uint8_t control = controlByteFromChunks<repr>(chunks);
  *controlOut = control;
  const NimbleBlockEncodeData& controlData = nimbleBlockEncodeData[control];
  for (int i = 0; i < kChunksPerBlock; ++i) {
    folly::storeUnaligned(dataOut, chunkToWireRepr<repr>(chunks[i]));
    // TODO: this leads to longer dependency chains. We should probably be
    // encoding the offsets to store at in the table, rather than the lengths of
    // each item.
    dataOut += controlData.sizes[i];
  }

  return controlData.sum;
}

} // namespace detail
} // namespace thrift
} // namespace apache
