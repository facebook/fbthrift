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
#include <cstring>

#include <folly/Format.h>
#include <folly/String.h>
#include <folly/container/Array.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/protocol/nimble/ChunkRepr.h>
#include <thrift/lib/cpp2/protocol/nimble/ControlBitHelpers.h>
#include <thrift/lib/cpp2/protocol/nimble/DecodeNimbleBlock.h>
#include <thrift/lib/cpp2/protocol/nimble/EncodeNimbleBlock.h>

namespace apache {
namespace thrift {
namespace detail {

template <ChunkRepr repr, bool vectorize>
void runRoundTripTest(
    const std::array<std::uint32_t, kChunksPerBlock>& unencoded) {
  std::array<unsigned char, kMaxBytesPerBlock> encoded;
  std::array<std::uint32_t, kChunksPerBlock> decoded;

  unsigned char control;
  int encodedSize =
      encodeNimbleBlock<repr>(unencoded.data(), &control, encoded.data());
  std::memset(&encoded[encodedSize], 0x99, kMaxBytesPerBlock - encodedSize);
  int decodedSize = decodeNimbleBlock<repr, vectorize>(
      control, folly::ByteRange(encoded), decoded.data());
  EXPECT_TRUE(encodedSize == decodedSize && unencoded == decoded)
      << folly::sformat(
             "Round trip failed in encoding {}, with data[{}] "
             "(encodedSize == {}, decodedSize == {})",
             repr == ChunkRepr::kRaw ? "Raw" : "Zigzag",
             folly::join(", ", unencoded),
             encodedSize,
             decodedSize);
}

template <ChunkRepr repr, bool vectorize>
void runHandPickedDecodeTest() {
  std::array<std::uint32_t, kChunksPerBlock> testData[] = {
      {0, 0, 0, 0},
      {(std::uint32_t)-1, 2, 3, 4},
      {1000, 255, 256, (std::uint32_t)-1000},
      {0, 0xABCD, 0, 0},
      {0xABCDEF, 0, 0x8000, 0},
      {0, 5000, 0x7FFF, (std::uint32_t)-0x7FFF},
      {0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF},
      {(std::uint32_t)-0xFF,
       (std::uint32_t)-0xFFFF,
       (std::uint32_t)-0xFFFFFF,
       (std::uint32_t)-0xFFFFFFFF},
      {0x80, 0x8000, 0x800000, 0x80000000},
      {(std::uint32_t)-0x80,
       (std::uint32_t)-0x8000,
       (std::uint32_t)-0x800000,
       (std::uint32_t)-0x80000000},
  };
  for (const std::array<std::uint32_t, kChunksPerBlock>& unencoded : testData) {
    runRoundTripTest<repr, vectorize>(unencoded);
  }
}

template <typename T>
class NimbleDecodeTest : public ::testing::Test {};

struct NonvectorizedImpl {
  constexpr static bool vectorize = false;
};

struct VectorizedImpl {
  constexpr static bool vectorize = true;
};

using DecodeParamTypes = ::testing::Types<NonvectorizedImpl, VectorizedImpl>;
TYPED_TEST_CASE(NimbleDecodeTest, DecodeParamTypes);

TYPED_TEST(NimbleDecodeTest, DecodesHandPickedZigzag) {
  runHandPickedDecodeTest<ChunkRepr::kZigzag, TypeParam::vectorize>();
}

TYPED_TEST(NimbleDecodeTest, DecodesHandPickedRaw) {
  runHandPickedDecodeTest<ChunkRepr::kRaw, TypeParam::vectorize>();
}

template <ChunkRepr repr, bool vectorize>
void runInterestingRoundTripTest() {
  // clang-format off
  std::vector<std::uint32_t> vec {
    0, 1,
    100,
    127, 128, 129,
    200,
    255, 256, 257,
    10*1000,
    32767, 32768, 32769,
    50*1000,
    (1U << 16) - 1,
    (1U << 16),
    (1U << 16) + 1,
    100*1000,
    (1U << 24) - 1,
    (1U << 24),
    (1U << 24) + 1,
    20*1000*1000,
    0xFFFFFFFEU,
    0xFFFFFFFFU
  };
  // clang-format on

  for (std::uint32_t i0 : vec) {
    for (std::uint32_t i1 : vec) {
      for (std::uint32_t i2 : vec) {
        for (std::uint32_t i3 : vec) {
          std::array<std::uint32_t, kChunksPerBlock> unencoded = {
              i0, i1, i2, i3};
          runRoundTripTest<repr, vectorize>(unencoded);
        }
      }
    }
  }
}

TYPED_TEST(NimbleDecodeTest, DecodesInterestingZigzag) {
  runInterestingRoundTripTest<ChunkRepr::kZigzag, TypeParam::vectorize>();
}

TYPED_TEST(NimbleDecodeTest, DecodesInterestingRaw) {
  runInterestingRoundTripTest<ChunkRepr::kRaw, TypeParam::vectorize>();
}

TYPED_TEST(NimbleDecodeTest, DecodesExtraZeros) {
  // Some use cases have people reserve a full chunk up front, even if they
  // won't need it until later on. The current iteration of the encoder won't
  // support these cases, but we should check it.

  // clang-format off
  std::array<unsigned char, kMaxBytesPerBlock> data = {
    /* chunk 0 */ 0, // = 0, 1 byte
    /* chunk 1 */ 0, 1, // = 256, 2 bytes,
    /* chunk 2 */ 0, 0, 0, 0, // = 0, 4 bytes
    /* chunk 3 */ 2, 0, 0, 1, // = 16777218, 4 bytes
    // data from next set of chunks; should be ignored.
    68, 97, 118, 105, 100,
  };
  // clang-format on

  std::array<std::uint32_t, kChunksPerBlock> decoded;
  std::uint8_t control = 0b11'11'10'01;
  int bytesDecoded = decodeNimbleBlock<ChunkRepr::kRaw, TypeParam::vectorize>(
      control, folly::ByteRange(data), decoded.data());
  EXPECT_EQ(11, bytesDecoded);
  EXPECT_EQ(0, decoded[0]);
  EXPECT_EQ(256, decoded[1]);
  EXPECT_EQ(0, decoded[2]);
  EXPECT_EQ(16777218, decoded[3]);

  bytesDecoded = decodeNimbleBlock<ChunkRepr::kZigzag, TypeParam::vectorize>(
      control, folly::ByteRange(data), decoded.data());
  EXPECT_EQ(11, bytesDecoded);
  EXPECT_EQ(zigzagDecode(0), decoded[0]);
  EXPECT_EQ(zigzagDecode(256), decoded[1]);
  EXPECT_EQ(zigzagDecode(0), decoded[2]);
  EXPECT_EQ(zigzagDecode(16777218), decoded[3]);
}

} // namespace detail
} // namespace thrift
} // namespace apache
