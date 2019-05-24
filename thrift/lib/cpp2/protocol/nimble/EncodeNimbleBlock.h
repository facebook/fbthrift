/*
 * Copyright 2019-present Facebook, Inc.
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
#include <cstdint>

#include <folly/lang/Bits.h>

#include <thrift/lib/cpp2/protocol/nimble/ChunkRepr.h>
#include <thrift/lib/cpp2/protocol/nimble/ControlBitHelpers.h>

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
extern const std::array<NimbleBlockEncodeData, 256> nimbleBlockEncodeData;

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
template <ChunkRepr repr>
int encodeNimbleBlock(
    const std::uint32_t chunks[kChunksPerBlock],
    std::uint8_t* controlOut,
    unsigned char* dataOut) {
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
