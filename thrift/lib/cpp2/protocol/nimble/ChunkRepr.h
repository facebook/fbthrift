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

#include <cstdint>

#include <folly/lang/Bits.h>

// A nimble-encoded struct is logically a set of streams of 32-bit chunks. On
// the wire, chunks are little-endian, variable-sized, and may be either
// zigzagged or not (depending on stream type, and whether or not we expect to
// see many negatives). These functions convert the in-memory representation of
// a chunk to the on-the-wire representation, and vice versa.

namespace apache {
namespace thrift {
namespace detail {

enum class ChunkRepr {
  kRaw,
  kZigzag,
};

// Our types don't nicely match the VarintUtils.h types; defining our own
// zigzagging primitives is convenient.

inline std::uint32_t zigzagEncode(std::uint32_t n) {
  // This is technically implementation-defined.
  static_assert(((-1) >> 1) == -1, "Morally bankrupt signed right shift");
  std::uint32_t arithmeticallyShifted =
      static_cast<std::uint32_t>(static_cast<std::int32_t>(n) >> 31);
  return arithmeticallyShifted ^ (n << 1);
}

inline std::uint32_t zigzagDecode(std::uint32_t n) {
  return (n & 1) ? ~(n >> 1) : (n >> 1);
}

template <ChunkRepr repr>
std::uint32_t chunkToWireRepr(std::uint32_t n) {
  if (repr == ChunkRepr::kZigzag) {
    n = zigzagEncode(n);
  }
  return folly::Endian::little(n);
}

template <ChunkRepr repr>
std::uint32_t chunkFromWireRepr(std::uint32_t n) {
  n = folly::Endian::little(n);
  if (repr == ChunkRepr::kZigzag) {
    n = zigzagDecode(n);
  }
  return n;
}

} // namespace detail
} // namespace thrift
} // namespace apache
