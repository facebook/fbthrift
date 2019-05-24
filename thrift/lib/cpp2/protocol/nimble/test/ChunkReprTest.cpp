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
#include <initializer_list>

#include <folly/lang/Bits.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/nimble/ChunkRepr.h>

using namespace apache::thrift::detail;

template <ChunkRepr repr>
void expectRepr(
    std::uint32_t n,
    std::initializer_list<unsigned char> expected,
    int line) {
  EXPECT_EQ(4, expected.size());
  const unsigned char* expectedBytes = expected.begin();
  std::array<unsigned char, sizeof(n)> actualBytes;
  std::uint32_t asRepr = chunkToWireRepr<repr>(n);
  std::memcpy(actualBytes.data(), &asRepr, sizeof(asRepr));
  EXPECT_EQ(expectedBytes[0], actualBytes[0]) << " at line " << line;
  EXPECT_EQ(expectedBytes[1], actualBytes[1]) << " at line " << line;
  EXPECT_EQ(expectedBytes[2], actualBytes[2]) << " at line " << line;
  EXPECT_EQ(expectedBytes[3], actualBytes[3]) << " at line " << line;
}

#define EXPECT_REPR_ZIGZAG(...) \
  expectRepr<ChunkRepr::kZigzag>(__VA_ARGS__, __LINE__)
#define EXPECT_REPR_RAW(...) expectRepr<ChunkRepr::kRaw>(__VA_ARGS__, __LINE__)

TEST(ChunkRepr, ZigzagSpotCheck) {
  EXPECT_REPR_ZIGZAG(0, {0, 0, 0, 0});
  EXPECT_REPR_ZIGZAG(static_cast<std::uint32_t>(-1), {1, 0, 0, 0});
  EXPECT_REPR_ZIGZAG(1, {2, 0, 0, 0});
  EXPECT_REPR_ZIGZAG(static_cast<std::uint32_t>(-2), {3, 0, 0, 0});
  EXPECT_REPR_ZIGZAG(2, {4, 0, 0, 0});
  EXPECT_REPR_ZIGZAG(static_cast<std::uint32_t>(-3), {5, 0, 0, 0});
  EXPECT_REPR_ZIGZAG(3, {6, 0, 0, 0});
  EXPECT_REPR_ZIGZAG(static_cast<std::uint32_t>(-4), {7, 0, 0, 0});
  EXPECT_REPR_ZIGZAG(4, {8, 0, 0, 0});

  EXPECT_REPR_ZIGZAG(256, {0, 2, 0, 0});
  EXPECT_REPR_ZIGZAG(257, {2, 2, 0, 0});
  EXPECT_REPR_ZIGZAG(258, {4, 2, 0, 0});

  EXPECT_REPR_ZIGZAG(static_cast<std::uint32_t>(-256), {255, 1, 0, 0});
  EXPECT_REPR_ZIGZAG(static_cast<std::uint32_t>(-257), {1, 2, 0, 0});
  EXPECT_REPR_ZIGZAG(static_cast<std::uint32_t>(-258), {3, 2, 0, 0});

  EXPECT_REPR_ZIGZAG(2147483647, {254, 255, 255, 255});
  EXPECT_REPR_ZIGZAG(
      static_cast<std::uint32_t>(-2147483648), {255, 255, 255, 255});
}

TEST(ChunkRepr, RawSpotCheck) {
  EXPECT_REPR_RAW(0, {0, 0, 0, 0});
  EXPECT_REPR_RAW(1, {1, 0, 0, 0});
  EXPECT_REPR_RAW(2, {2, 0, 0, 0});
  EXPECT_REPR_RAW(3, {3, 0, 0, 0});
  EXPECT_REPR_RAW(4, {4, 0, 0, 0});
  EXPECT_REPR_RAW(256, {0, 1, 0, 0});
  EXPECT_REPR_RAW(257, {1, 1, 0, 0});
  EXPECT_REPR_RAW(258, {2, 1, 0, 0});

  EXPECT_REPR_RAW(static_cast<std::uint32_t>(-1), {255, 255, 255, 255});
  EXPECT_REPR_RAW(static_cast<std::uint32_t>(-2), {254, 255, 255, 255});
  EXPECT_REPR_RAW(static_cast<std::uint32_t>(-3), {253, 255, 255, 255});
  EXPECT_REPR_RAW(static_cast<std::uint32_t>(-4), {252, 255, 255, 255});
  EXPECT_REPR_RAW(static_cast<std::uint32_t>(-256), {0, 255, 255, 255});
  EXPECT_REPR_RAW(static_cast<std::uint32_t>(-257), {255, 254, 255, 255});
  EXPECT_REPR_RAW(static_cast<std::uint32_t>(-258), {254, 254, 255, 255});

  EXPECT_REPR_RAW(2147483647, {255, 255, 255, 127});
  EXPECT_REPR_RAW(static_cast<std::uint32_t>(-2147483648), {0, 0, 0, 128});
}

TEST(ChunkRepr, ZigzagInvertible1) {
  for (std::uint64_t i = 0; i < (1ULL << 32); ++i) {
    std::uint32_t testVal = static_cast<std::uint32_t>(i);
    EXPECT_EQ(
        testVal,
        chunkFromWireRepr<ChunkRepr::kZigzag>(
            chunkToWireRepr<ChunkRepr::kZigzag>(i)));
  }
}

TEST(ChunkRepr, ZigzagInvertible2) {
  for (std::uint64_t i = 0; i < (1ULL << 32); ++i) {
    std::uint32_t testVal = static_cast<std::uint32_t>(i);
    EXPECT_EQ(
        testVal,
        chunkToWireRepr<ChunkRepr::kZigzag>(
            chunkFromWireRepr<ChunkRepr::kZigzag>(i)));
  }
}

TEST(ChunkRepr, RawInvertible1) {
  for (std::uint64_t i = 0; i < (1ULL << 32); ++i) {
    std::uint32_t testVal = static_cast<std::uint32_t>(i);
    EXPECT_EQ(
        testVal,
        chunkFromWireRepr<ChunkRepr::kRaw>(
            chunkToWireRepr<ChunkRepr::kRaw>(i)));
  }
}

TEST(ChunkRepr, RawInvertible2) {
  for (std::uint64_t i = 0; i < (1ULL << 32); ++i) {
    std::uint32_t testVal = static_cast<std::uint32_t>(i);
    EXPECT_EQ(
        testVal,
        chunkToWireRepr<ChunkRepr::kRaw>(
            chunkFromWireRepr<ChunkRepr::kRaw>(i)));
  }
}
