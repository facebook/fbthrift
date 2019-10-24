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

#include <numeric>

#include <folly/container/Array.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/nimble/EncodeNimbleBlock.h>

namespace apache {
namespace thrift {
namespace detail {

TEST(NimbleBlockEncodeData, SumsAddUp) {
  for (const auto& controlData : nimbleBlockEncodeData) {
    EXPECT_EQ(
        controlData.sum,
        std::accumulate(
            &controlData.sizes[0], &controlData.sizes[kChunksPerBlock], 0));
  }
}

TEST(NimbleBlockEncodeData, Monotonic) {
  for (int i = 0; i < 256; ++i) {
    for (int bit = 0; bit < 8; ++bit) {
      auto& origControl = nimbleBlockEncodeData[i];
      auto& biggerControl = nimbleBlockEncodeData[i | (1 << bit)];
      auto& smallerControl = nimbleBlockEncodeData[i & ~(1 << bit)];
      EXPECT_LE(origControl.sum, biggerControl.sum);
      EXPECT_GE(origControl.sum, smallerControl.sum);
      EXPECT_LE(origControl.sizes[bit / 2], biggerControl.sizes[bit / 2]);
      EXPECT_GE(origControl.sizes[bit / 2], smallerControl.sizes[bit / 2]);
      for (int j = 0; j < kChunksPerBlock; ++j) {
        if (bit / 2 != j) {
          EXPECT_EQ(origControl.sizes[j], biggerControl.sizes[j]);
          EXPECT_EQ(origControl.sizes[j], smallerControl.sizes[j]);
        }
      }
    }
  }
}

template <typename T>
class NimbleBlockEncodeDataControlByteTest : public ::testing::Test {};

struct NonVectorizedControlByteImpl {
  static unsigned char controlByteFromChunksImpl(const std::uint32_t* chunks) {
    return controlByteFromChunks<ChunkRepr::kRaw>(chunks);
  }
};

#if APACHE_THRIFT_DETAIL_NIMBLE_CAN_VECTORIZE
struct VectorizedControlByteImpl {
  static unsigned char controlByteFromChunksImpl(const std::uint32_t* chunks) {
    return controlByteFromVector(loadVector<ChunkRepr::kRaw>(chunks));
  }
};

using ControlByteParamTypes =
    ::testing::Types<NonVectorizedControlByteImpl, VectorizedControlByteImpl>;
#else
using ControlByteParamTypes = ::testing::Types<NonVectorizedControlByteImpl>;
#endif

TYPED_TEST_CASE(NimbleBlockEncodeDataControlByteTest, ControlByteParamTypes);

TYPED_TEST(NimbleBlockEncodeDataControlByteTest, ControlByteComputation) {
  struct TestCase {
    std::uint32_t chunk;
    std::uint8_t bitPair;
  };
  std::vector<TestCase> testData = {{0, 0},
                                    {1, 1},
                                    {2, 1},
                                    {100, 1},
                                    {255, 1},
                                    {256, 2},
                                    {257, 2},
                                    {10000, 2},
                                    {(1U << 16) - 1, 2},
                                    {(1U << 16), 3},
                                    {(1U << 16) + 1, 3},
                                    {1000 * 1000, 3},
                                    {(1U << 24) - 1, 3},
                                    {(1U << 24), 3},
                                    {(1U << 24) + 1, 3},
                                    {1000 * 1000 * 1000, 3},
                                    {0xFFFFFFFFU - 1, 3},
                                    {0xFFFFFFFFU, 3}};
  for (auto& p0 : testData) {
    for (auto& p1 : testData) {
      for (auto& p2 : testData) {
        for (auto& p3 : testData) {
          std::array<std::uint32_t, kChunksPerBlock> arr{
              p0.chunk, p1.chunk, p2.chunk, p3.chunk};
          std::uint8_t controlByte =
              TypeParam::controlByteFromChunksImpl(arr.data());
          std::uint8_t expectedControlByte = p0.bitPair | (p1.bitPair << 2) |
              (p2.bitPair << 4) | (p3.bitPair << 6);
          EXPECT_EQ(controlByte, expectedControlByte);
        }
      }
    }
  }
}

template <typename T, std::size_t m, std::size_t n>
void expectArrayPrefix(
    const std::array<T, m>& data,
    const std::array<T, n>& prefix,
    int line) {
  // This of course could be a static assert; but making it dynamic makes the
  // test infrastructure work easier (e.g. if you have several buggy tests but
  // only want to debug one at a time, starting with some other one).
  EXPECT_LE(n, m) << "Right side too big!";
  for (unsigned i = 0; i < n; ++i) {
    EXPECT_EQ(prefix[i], data[i]) << ", at index " << i << " of line " << line;
  }
}

#define EXPECT_PREFIX(...) expectArrayPrefix(__VA_ARGS__, __LINE__)

template <typename T>
class NimbleBlockEncoderTest : public ::testing::Test {};

template <bool vec>
struct NimbleBlockEncoderParam {
  constexpr static bool vectorized = vec;
};

using EncoderParamTypes = ::testing::
    Types<NimbleBlockEncoderParam<true>, NimbleBlockEncoderParam<false>>;
TYPED_TEST_CASE(NimbleBlockEncoderTest, EncoderParamTypes);

TYPED_TEST(NimbleBlockEncoderTest, EncodesRaw) {
  std::uint8_t control = 123;
  std::array<unsigned char, kMaxBytesPerBlock> output;
  auto content = folly::make_array<std::uint32_t>(0, 0, 0x22, 0xAB'CD);
  int out = encodeNimbleBlock<ChunkRepr::kRaw, TypeParam::vectorized>(
      content.data(), &control, output.data());
  EXPECT_EQ(3, out);
  EXPECT_EQ(0b10'01'00'00, control);
  EXPECT_PREFIX(output, folly::make_array<unsigned char>(0x22, 0xCD, 0xAB));

  content = folly::make_array<std::uint32_t>(0xABCDE, 0, 0x22, 0xABCD);
  out = encodeNimbleBlock<ChunkRepr::kRaw, TypeParam::vectorized>(
      content.data(), &control, output.data());
  EXPECT_EQ(7, out);
  EXPECT_EQ(0b10'01'00'11, control);
  EXPECT_PREFIX(
      output,
      folly::make_array<unsigned char>(
          0xDE, 0xBC, 0x0A, 0x00, 0x22, 0xCD, 0xAB));

  content = folly::make_array<std::uint32_t>(0, 0, 0, 0);
  out = encodeNimbleBlock<ChunkRepr::kRaw, TypeParam::vectorized>(
      content.data(), &control, output.data());
  EXPECT_EQ(0, out);
  EXPECT_EQ(0, control);
}

TYPED_TEST(NimbleBlockEncoderTest, EncodesZigzag) {
  std::uint8_t control = 123;
  std::array<unsigned char, kMaxBytesPerBlock> output;
  auto content = folly::make_array<std::uint32_t>(0, 0, 40, 0x76'54);
  int out = encodeNimbleBlock<ChunkRepr::kZigzag, TypeParam::vectorized>(
      content.data(), &control, output.data());
  EXPECT_EQ(3, out);
  EXPECT_EQ(0b10'01'00'00, control);
  EXPECT_PREFIX(output, folly::make_array<unsigned char>(80, 0xA8, 0xEC));

  content = folly::make_array<std::uint32_t>(-1, 0, 30, 0x82'73);
  out = encodeNimbleBlock<ChunkRepr::kZigzag, TypeParam::vectorized>(
      content.data(), &control, output.data());
  EXPECT_EQ(6, out);
  EXPECT_EQ(0b11'01'00'01, control);
  EXPECT_PREFIX(
      output, folly::make_array<unsigned char>(1, 60, 0xE6, 0x04, 0x01, 0x00));

  content = folly::make_array<std::uint32_t>(0, 0, 0, 0);
  out = encodeNimbleBlock<ChunkRepr::kZigzag, TypeParam::vectorized>(
      content.data(), &control, output.data());
  EXPECT_EQ(0, out);
  EXPECT_EQ(0, control);

  content = folly::make_array<std::uint32_t>(-1, -2, -0x8000, -0x8001);
  out = encodeNimbleBlock<ChunkRepr::kZigzag, TypeParam::vectorized>(
      content.data(), &control, output.data());
  EXPECT_EQ(8, out);
  EXPECT_EQ(0b11'10'01'01, control);
  EXPECT_PREFIX(
      output,
      folly::make_array<unsigned char>(
          1, 3, 0xFF, 0xFF, 0x01, 0x00, 0x01, 0x00));
}

} // namespace detail
} // namespace thrift
} // namespace apache
