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
#include <stdexcept>

namespace apache {
namespace thrift {
namespace detail {

const static int kChunksPerBlock = 4; // I.e. 1 control byte's worth of chunks.
const static int kMaxBytesPerBlock = 16; // i.e. 4 chunks.

constexpr int controlBitPairToSize(std::uint8_t bits, int pair) {
  int bitPair = (bits >> (pair * 2)) & 3;
  switch (bitPair) {
    case 0:
      return 0;
    case 1:
      return 1;
    case 2:
      return 2;
    case 3:
      return 4;
    default:
      // Ideally, we would throw or do something else that breaks constexpr
      // compilation. Unfortunately some compilers incorrectly error out on any
      // occurrence of "throw" in a constexpr context, even if it never
      // executes.
      return 0;
  }
}

} // namespace detail
} // namespace thrift
} // namespace apache
