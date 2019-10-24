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

#include <stdexcept>
#include <utility>

#include <folly/Portability.h>
#include <folly/container/Array.h>

#include <thrift/lib/cpp2/protocol/nimble/ControlBitHelpers.h>
#include <thrift/lib/cpp2/protocol/nimble/EncodeNimbleBlock.h>

namespace apache {
namespace thrift {
namespace detail {

namespace {

constexpr NimbleBlockEncodeData encodeDataForControlByte(std::uint8_t byte) {
  NimbleBlockEncodeData result;
  result.sum = 0;
  for (int i = 0; i < kChunksPerBlock; ++i) {
    int size = controlBitPairToSize(byte, i);
    result.sizes[i] = size;
    result.sum += size;
  }
  return result;
}

template <std::size_t... Indices>
constexpr std::array<NimbleBlockEncodeData, 256> encodeDataFromIndices(
    std::index_sequence<Indices...>) {
  return {{encodeDataForControlByte(Indices)...}};
}

constexpr NimbleBlockVectorEncodeData vectorEncodeDataForControlByte(
    std::uint8_t byte) {
  NimbleBlockVectorEncodeData result;
  int vectorIdx = 0;
  for (int i = 0; i < kChunksPerBlock; ++i) {
    for (int j = 0; j < controlBitPairToSize(byte, i); ++j) {
      result.shuffleVector[vectorIdx++] = 4 * i + j;
    }
  }
  while (vectorIdx < kMaxBytesPerBlock) {
    // set 7th bit to 1 to place "0" in the resulting vector,
    // see _mm_shuffle_epi8()
    result.shuffleVector[vectorIdx++] = 0x80;
  }
  return result;
}

} // namespace

FOLLY_STORAGE_CONSTEXPR const std::array<NimbleBlockEncodeData, 256>
    nimbleBlockEncodeData =
        encodeDataFromIndices(std::make_index_sequence<256>{});

FOLLY_STORAGE_CONSTEXPR const std::array<NimbleBlockVectorEncodeData, 256>
    nimbleBlockVectorEncodeData =
        folly::make_array_with<256>(vectorEncodeDataForControlByte);

} // namespace detail
} // namespace thrift
} // namespace apache
