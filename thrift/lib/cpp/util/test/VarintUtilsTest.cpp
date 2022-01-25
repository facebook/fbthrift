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

#include <thrift/lib/cpp/util/VarintUtils.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp/util/test/VarintUtilsTestUtil.h>

using namespace apache::thrift::util;

class VarintUtilsTest : public testing::Test {};

TEST_F(VarintUtilsTest, example) {
  folly::IOBufQueue queueUnrolled;
  folly::IOBufQueue queueBMI2;
  folly::io::QueueAppender appenderUnrolled(&queueUnrolled, 1000);
  folly::io::QueueAppender appenderBMI2(&queueBMI2, 1000);

  auto test = [&](int bit) {
    std::string u;
    std::string b;
    queueUnrolled.appendToString(u);
    queueBMI2.appendToString(b);
    CHECK_EQ(u.size(), b.size()) << "bit: " << bit;
    CHECK_EQ(u, b) << "bit: " << bit;
  };

  int64_t v = 1;
  writeVarintUnrolled(appenderUnrolled, 0);
  writeVarintBMI2(appenderBMI2, 0);
  for (int bit = 0; bit < 64; bit++, v <<= int(bit < 64)) {
    if (bit < 8) {
      writeVarintUnrolled(appenderUnrolled, int8_t(v));
      writeVarintBMI2(appenderBMI2, int8_t(v));
      test(bit);
    }
    if (bit < 16) {
      writeVarintUnrolled(appenderUnrolled, int16_t(v));
      writeVarintBMI2(appenderBMI2, int16_t(v));
      test(bit);
    }
    if (bit < 32) {
      writeVarintUnrolled(appenderUnrolled, int32_t(v));
      writeVarintBMI2(appenderBMI2, int32_t(v));
      test(bit);
    }
    writeVarintUnrolled(appenderUnrolled, v);
    writeVarintBMI2(appenderBMI2, v);
    test(bit);
  }
  int32_t oversize = 1000000;
  writeVarintUnrolled(appenderUnrolled, oversize);
  writeVarintBMI2(appenderBMI2, oversize);

  {
    folly::io::Cursor rcursor(queueUnrolled.front());
    EXPECT_EQ(0, readVarint<int8_t>(rcursor));
    v = 1;
    for (int bit = 0; bit < 64; bit++, v <<= int(bit < 64)) {
      if (bit < 8) {
        EXPECT_EQ(int8_t(v), readVarint<int8_t>(rcursor));
      }
      if (bit < 16) {
        EXPECT_EQ(int16_t(v), readVarint<int16_t>(rcursor));
      }
      if (bit < 32) {
        EXPECT_EQ(int32_t(v), readVarint<int32_t>(rcursor));
      }
      EXPECT_EQ(v, readVarint<int64_t>(rcursor));
    }
    EXPECT_THROW(readVarint<uint8_t>(rcursor), std::out_of_range);
  }

  {
    folly::io::Cursor rcursor(queueBMI2.front());
    EXPECT_EQ(0, readVarint<int8_t>(rcursor));
    v = 1;
    for (int bit = 0; bit < 64; bit++, v <<= int(bit < 64)) {
      if (bit < 8) {
        EXPECT_EQ(int8_t(v), readVarint<int8_t>(rcursor));
      }
      if (bit < 16) {
        EXPECT_EQ(int16_t(v), readVarint<int16_t>(rcursor));
      }
      if (bit < 32) {
        EXPECT_EQ(int32_t(v), readVarint<int32_t>(rcursor));
      }
      EXPECT_EQ(v, readVarint<int64_t>(rcursor));
    }
    EXPECT_THROW(readVarint<uint8_t>(rcursor), std::out_of_range);
  }
}

template <typename Param>
struct VarintUtilsMegaTest : public testing::TestWithParam<Param> {};
TYPED_TEST_SUITE_P(VarintUtilsMegaTest);

TYPED_TEST_P(VarintUtilsMegaTest, example) {
  auto ints = TypeParam::gen();
  std::string strUnrolled;
  {
    folly::IOBufQueue q(folly::IOBufQueue::cacheChainLength());
    constexpr size_t kDesiredGrowth = 1 << 14;
    folly::io::QueueAppender c(&q, kDesiredGrowth);
    for (auto v : ints) {
      writeVarintUnrolled(c, v);
    }
    q.appendToString(strUnrolled);
  }

  std::string strBmi2;
  {
    folly::IOBufQueue q(folly::IOBufQueue::cacheChainLength());
    constexpr size_t kDesiredGrowth = 1 << 14;
    folly::io::QueueAppender c(&q, kDesiredGrowth);
    for (auto v : ints) {
      writeVarintBMI2(c, v);
    }
    q.appendToString(strBmi2);
  }
  EXPECT_EQ(strUnrolled, strBmi2);
}

REGISTER_TYPED_TEST_SUITE_P( //
    VarintUtilsMegaTest,
    example);

INSTANTIATE_TYPED_TEST_SUITE_P(u8_1b, VarintUtilsMegaTest, u8_1b);
INSTANTIATE_TYPED_TEST_SUITE_P(u16_1b, VarintUtilsMegaTest, u16_1b);
INSTANTIATE_TYPED_TEST_SUITE_P(u32_1b, VarintUtilsMegaTest, u32_1b);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_1b, VarintUtilsMegaTest, u64_1b);

INSTANTIATE_TYPED_TEST_SUITE_P(u8_2b, VarintUtilsMegaTest, u8_2b);
INSTANTIATE_TYPED_TEST_SUITE_P(u16_2b, VarintUtilsMegaTest, u16_2b);
INSTANTIATE_TYPED_TEST_SUITE_P(u32_2b, VarintUtilsMegaTest, u32_2b);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_2b, VarintUtilsMegaTest, u64_2b);

INSTANTIATE_TYPED_TEST_SUITE_P(u16_3b, VarintUtilsMegaTest, u16_3b);
INSTANTIATE_TYPED_TEST_SUITE_P(u32_3b, VarintUtilsMegaTest, u32_3b);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_3b, VarintUtilsMegaTest, u64_3b);

INSTANTIATE_TYPED_TEST_SUITE_P(u32_4b, VarintUtilsMegaTest, u32_4b);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_4b, VarintUtilsMegaTest, u64_4b);

INSTANTIATE_TYPED_TEST_SUITE_P(u32_5b, VarintUtilsMegaTest, u32_5b);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_5b, VarintUtilsMegaTest, u64_5b);

INSTANTIATE_TYPED_TEST_SUITE_P(u64_6b, VarintUtilsMegaTest, u64_6b);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_7b, VarintUtilsMegaTest, u64_7b);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_8b, VarintUtilsMegaTest, u64_8b);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_9b, VarintUtilsMegaTest, u64_9b);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_10b, VarintUtilsMegaTest, u64_10b);

INSTANTIATE_TYPED_TEST_SUITE_P(u8_any, VarintUtilsMegaTest, u8_any);
INSTANTIATE_TYPED_TEST_SUITE_P(u16_any, VarintUtilsMegaTest, u16_any);
INSTANTIATE_TYPED_TEST_SUITE_P(u32_any, VarintUtilsMegaTest, u32_any);
INSTANTIATE_TYPED_TEST_SUITE_P(u64_any, VarintUtilsMegaTest, u64_any);

INSTANTIATE_TYPED_TEST_SUITE_P(s8_1b, VarintUtilsMegaTest, s8_1b);
INSTANTIATE_TYPED_TEST_SUITE_P(s16_1b, VarintUtilsMegaTest, s16_1b);
INSTANTIATE_TYPED_TEST_SUITE_P(s32_1b, VarintUtilsMegaTest, s32_1b);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_1b, VarintUtilsMegaTest, s64_1b);

INSTANTIATE_TYPED_TEST_SUITE_P(s8_2b, VarintUtilsMegaTest, s8_2b);
INSTANTIATE_TYPED_TEST_SUITE_P(s16_2b, VarintUtilsMegaTest, s16_2b);
INSTANTIATE_TYPED_TEST_SUITE_P(s32_2b, VarintUtilsMegaTest, s32_2b);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_2b, VarintUtilsMegaTest, s64_2b);

INSTANTIATE_TYPED_TEST_SUITE_P(s16_3b, VarintUtilsMegaTest, s16_3b);
INSTANTIATE_TYPED_TEST_SUITE_P(s32_3b, VarintUtilsMegaTest, s32_3b);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_3b, VarintUtilsMegaTest, s64_3b);

INSTANTIATE_TYPED_TEST_SUITE_P(s32_4b, VarintUtilsMegaTest, s32_4b);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_4b, VarintUtilsMegaTest, s64_4b);

INSTANTIATE_TYPED_TEST_SUITE_P(s32_5b, VarintUtilsMegaTest, s32_5b);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_5b, VarintUtilsMegaTest, s64_5b);

INSTANTIATE_TYPED_TEST_SUITE_P(s64_6b, VarintUtilsMegaTest, s64_6b);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_7b, VarintUtilsMegaTest, s64_7b);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_8b, VarintUtilsMegaTest, s64_8b);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_9b, VarintUtilsMegaTest, s64_9b);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_10b, VarintUtilsMegaTest, s64_10b);

INSTANTIATE_TYPED_TEST_SUITE_P(s8_any, VarintUtilsMegaTest, s8_any);
INSTANTIATE_TYPED_TEST_SUITE_P(s16_any, VarintUtilsMegaTest, s16_any);
INSTANTIATE_TYPED_TEST_SUITE_P(s32_any, VarintUtilsMegaTest, s32_any);
INSTANTIATE_TYPED_TEST_SUITE_P(s64_any, VarintUtilsMegaTest, s64_any);
