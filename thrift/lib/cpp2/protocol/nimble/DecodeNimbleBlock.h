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

#include <folly/GLog.h>
#include <folly/Range.h>
#include <folly/lang/Bits.h>

#include <thrift/lib/cpp2/protocol/nimble/ChunkRepr.h>

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

extern const std::array<NimbleBlockDecodeData, 256> nimbleBlockDecodeData;

// This function takes the control byte for the block, and a pointer to the
// associated data (which must contain at least 16 valid bytes), and places the
// decoded data into chunksOut. The return value is the number of bytes consumed
// from the input stream.
template <ChunkRepr repr>
int decodeNimbleBlock(
    std::uint8_t control,
    folly::ByteRange data,
    std::uint32_t chunksOut[4]) {
  DCHECK(data.size() >= 16);
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
