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

#include <stdexcept>
#include <utility>

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

template <std::size_t... Indices>
constexpr std::array<NimbleBlockDecodeData, 256> decodeDataFromIndices(
    std::index_sequence<Indices...>) {
  return {{decodeDataForControlByte(Indices)...}};
}
} // namespace

constexpr const std::array<NimbleBlockDecodeData, 256> nimbleBlockDecodeData =
    decodeDataFromIndices(std::make_index_sequence<256>{});

} // namespace detail
} // namespace thrift
} // namespace apache
