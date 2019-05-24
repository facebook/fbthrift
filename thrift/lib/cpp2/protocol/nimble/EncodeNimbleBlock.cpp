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
} // namespace

constexpr const std::array<NimbleBlockEncodeData, 256> nimbleBlockEncodeData =
    encodeDataFromIndices(std::make_index_sequence<256>{});

} // namespace detail
} // namespace thrift
} // namespace apache
