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

#include <folly/Portability.h>
#include <folly/container/Array.h>

#include <thrift/lib/cpp2/protocol/nimble/ControlBitHelpers.h>
#include <thrift/lib/cpp2/protocol/nimble/DecodeNimbleBlock.h>

namespace apache {
namespace thrift {
namespace detail {

namespace {

constexpr NimbleBlockDecodeData decodeDataForControlByte(std::uint8_t byte) {
  NimbleBlockDecodeData result;
  int sum = 0;
  for (int i = 0; i < kChunksPerBlock; ++i) {
    int size = controlBitPairToSize(byte, i);

    // Offset for chunk n is sum of all previous chunk sizes.
    result.data[i].offsetOrSize = sum;
    // We read 4 bytes, but only size of them are for the current chunk. We need
    // to shift off the high 4-size, phrasing the shift in terms of bits.
    result.data[i].maskShift = (4 - size) * 8;

    sum += size;
  }
  result.data[0].offsetOrSize = sum;

  return result;
}

constexpr NimbleBlockVectorDecodeData vectorDecodeDataForControlByte(
    std::uint8_t byte) {
  NimbleBlockVectorDecodeData result;
  int sum = 0;
  int inIdx = 0;
  for (int i = 0; i < kChunksPerBlock; ++i) {
    int chunkSize = controlBitPairToSize(byte, i);
    // The first chunkSize bytes come from the input, the next 4-chunkSize are
    // 0.
    for (int j = 0; j < chunkSize; ++j) {
      result.shuffleVector[4 * i + j] = inIdx;
      ++inIdx;
    }
    for (int j = chunkSize; j < 4; ++j) {
      // A high bit of 1 means zero the corresponding byte of the output vec
      // after shuffling.
      result.shuffleVector[4 * i + j] = 128;
    }

    sum += chunkSize;
  }
  result.size = sum;
  return result;
}

} // namespace

FOLLY_STORAGE_CONSTEXPR const std::array<NimbleBlockDecodeData, 256>
    nimbleBlockDecodeData =
        folly::make_array_with<256>(decodeDataForControlByte);

FOLLY_STORAGE_CONSTEXPR const std::array<NimbleBlockVectorDecodeData, 256>
    nimbleBlockVectorDecodeData =
        folly::make_array_with<256>(vectorDecodeDataForControlByte);

} // namespace detail
} // namespace thrift
} // namespace apache
