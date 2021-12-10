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

#include <immintrin.h>

#include <exception>
#include <numeric>
#include <random>
#include <type_traits>
#include <vector>

#include <glog/logging.h>
#include <folly/Benchmark.h>
#include <folly/BenchmarkUtil.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp/util/VarintUtils.h>

using namespace apache::thrift::util;

template <class T>
std::vector<T> gen(size_t n, T begin, T end) {
  std::default_random_engine rng(1729); // Make PRNG deterministic.
  std::uniform_int_distribution<T> dist(begin, end - 1);
  std::vector<T> v(n);
  std::generate(v.begin(), v.end(), [&] { return dist(rng); });
  return v;
}

uint64_t lim(size_t bytes) {
  return uint64_t(1) << (bytes * 7);
}

constexpr size_t N = 1000;
auto u8_1B = gen<uint8_t>(N, 0, lim(1));
auto u16_1B = gen<uint16_t>(N, 0, lim(1));
auto u32_1B = gen<uint32_t>(N, 0, lim(1));
auto u64_1B = gen<uint64_t>(N, 0, lim(1));

auto u8_2B = gen<uint8_t>(N, lim(1), lim(2));
auto u16_2B = gen<uint16_t>(N, lim(1), lim(2));
auto u32_2B = gen<uint32_t>(N, lim(1), lim(2));
auto u64_2B = gen<uint64_t>(N, lim(1), lim(2));

auto u16_3B = gen<uint16_t>(N, lim(2), lim(3));
auto u32_3B = gen<uint32_t>(N, lim(2), lim(3));
auto u64_3B = gen<uint64_t>(N, lim(2), lim(3));

auto u32_4B = gen<uint32_t>(N, lim(3), lim(4));
auto u64_4B = gen<uint64_t>(N, lim(3), lim(4));

auto u32_5B = gen<uint32_t>(N, lim(4), lim(5));
auto u64_5B = gen<uint64_t>(N, lim(4), lim(5));

auto u64_6B = gen<uint64_t>(N, lim(5), lim(6));
auto u64_7B = gen<uint64_t>(N, lim(6), lim(7));
auto u64_8B = gen<uint64_t>(N, lim(7), lim(8));
auto u64_9B = gen<uint64_t>(N, lim(8), lim(9));
auto u64_10B = gen<uint64_t>(N, lim(9), std::numeric_limits<uint64_t>::max());

auto u8_any = gen<uint8_t>(N, 0, std::numeric_limits<uint8_t>::max());
auto u16_any = gen<uint16_t>(N, 0, std::numeric_limits<uint16_t>::max());
auto u32_any = gen<uint32_t>(N, 0, std::numeric_limits<uint32_t>::max());
auto u64_any = gen<uint64_t>(N, 0, std::numeric_limits<uint64_t>::max());

template <class T>
void testWrite(const std::vector<T>& ints) {
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

#define BENCH_WRITE(fn, ints, iters)                                   \
  folly::IOBufQueue iobufQueue(folly::IOBufQueue::cacheChainLength()); \
  while (iters--) {                                                    \
    constexpr size_t kDesiredGrowth = 1 << 14;                         \
    folly::io::QueueAppender c(&iobufQueue, kDesiredGrowth);           \
    c.ensure(ints.size() * 10);                                        \
    for (auto v : ints) {                                              \
      fn(c, v);                                                        \
    }                                                                  \
    iobufQueue.clearAndTryReuseLargestBuffer();                        \
  }

BENCHMARK(baseline_u8_1B, iters) {
  BENCH_WRITE(writeVarintUnrolled, u8_1B, iters);
}
BENCHMARK_DRAW_LINE();

auto toSignedVector = [](auto uints) {
  using U = typename decltype(uints)::value_type;
  using S = typename std::make_signed<U>::type;
  std::vector<S> v;
  for (auto u : uints) {
    v.push_back(static_cast<S>(u));
  }
  return v;
};

#define GROUP(kind)                                                \
  TEST(TestVarintWrite, Test_Unsigned_##kind) { testWrite(kind); } \
  TEST(TestVarintWrite, Test_Signed_##kind) {                      \
    testWrite(toSignedVector(kind));                               \
  }                                                                \
  BENCHMARK(write_unrol_##kind, iters) {                           \
    BENCH_WRITE(writeVarintUnrolled, kind, iters);                 \
  }                                                                \
  BENCHMARK_RELATIVE(write_bmi2__##kind, iters) {                  \
    BENCH_WRITE(writeVarintBMI2, kind, iters);                     \
  }

GROUP(u8_any)
GROUP(u16_any)
GROUP(u32_any)
GROUP(u64_any)

GROUP(u8_1B)
GROUP(u8_2B)

GROUP(u16_1B)
GROUP(u16_2B)
GROUP(u16_3B)

GROUP(u32_1B)
GROUP(u32_2B)
GROUP(u32_3B)
GROUP(u32_4B)
GROUP(u32_5B)

GROUP(u64_1B)
GROUP(u64_2B)
GROUP(u64_3B)
GROUP(u64_4B)
GROUP(u64_5B)
GROUP(u64_6B)
GROUP(u64_7B)
GROUP(u64_8B)
GROUP(u64_9B)
GROUP(u64_10B)

TEST(VarintTest, Varint) {
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

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  if (testing::FLAGS_gtest_list_tests) {
    return RUN_ALL_TESTS();
  }

  auto ret = RUN_ALL_TESTS();
  if (ret == 0 && FLAGS_benchmark) {
    folly::runBenchmarks();
  }
}

#if 0
// (Skylake) Intel(R) Xeon(R) Gold 6138 CPU @ 2.00GHz
$ buck run @mode/opt thrift/lib/cpp/util/test:varint_benchmark -- -bm_min_iters=1000000 --benchmark
============================================================================
thrift/lib/cpp/util/test/VarintBenchmark.cpp    relative  time/iter  iters/s
============================================================================
baseline_u8_1B                                               1.45us  688.01K
----------------------------------------------------------------------------
write_unrol_u8_any                                           1.94us  514.87K
write_bmi2__u8_any                               134.18%     1.45us  690.83K
write_unrol_u16_any                                          3.99us  250.71K
write_bmi2__u16_any                              250.53%     1.59us  628.12K
write_unrol_u32_any                                          5.43us  184.02K
write_bmi2__u32_any                              336.61%     1.61us  619.42K
write_unrol_u64_any                                         10.37us   96.41K
write_bmi2__u64_any                              510.77%     2.03us  492.43K
write_unrol_u8_1B                                            1.35us  741.16K
write_bmi2__u8_1B                                 99.97%     1.35us  740.96K
write_unrol_u8_2B                                            2.46us  406.26K
write_bmi2__u8_2B                                164.89%     1.49us  669.91K
write_unrol_u16_1B                                           1.34us  748.00K
write_bmi2__u16_1B                               100.82%     1.33us  754.16K
write_unrol_u16_2B                                           2.40us  415.90K
write_bmi2__u16_2B                               151.01%     1.59us  628.06K
write_unrol_u16_3B                                           2.44us  409.41K
write_bmi2__u16_3B                               153.13%     1.60us  626.95K
write_unrol_u32_1B                                           2.59us  386.17K
write_bmi2__u32_1B                               192.95%     1.34us  745.10K
write_unrol_u32_2B                                           3.33us  300.39K
write_bmi2__u32_2B                               208.48%     1.60us  626.26K
write_unrol_u32_3B                                           3.81us  262.77K
write_bmi2__u32_3B                               218.75%     1.74us  574.83K
write_unrol_u32_4B                                           4.30us  232.69K
write_bmi2__u32_4B                               270.73%     1.59us  629.95K
write_unrol_u32_5B                                           4.65us  214.88K
write_bmi2__u32_5B                               286.23%     1.63us  615.06K
write_unrol_u64_1B                                           1.35us  740.24K
write_bmi2__u64_1B                               100.28%     1.35us  742.32K
write_unrol_u64_2B                                           3.09us  324.13K
write_bmi2__u64_2B                               153.54%     2.01us  497.67K
write_unrol_u64_3B                                           3.65us  274.07K
write_bmi2__u64_3B                               184.29%     1.98us  505.09K
write_unrol_u64_4B                                           4.24us  236.05K
write_bmi2__u64_4B                               213.61%     1.98us  504.22K
write_unrol_u64_5B                                           4.60us  217.40K
write_bmi2__u64_5B                               230.97%     1.99us  502.12K
write_unrol_u64_6B                                           4.89us  204.61K
write_bmi2__u64_6B                               243.68%     2.01us  498.61K
write_unrol_u64_7B                                           5.53us  180.80K
write_bmi2__u64_7B                               276.34%     2.00us  499.61K
write_unrol_u64_8B                                           6.50us  153.80K
write_bmi2__u64_8B                               316.60%     2.05us  486.93K
write_unrol_u64_9B                                           6.73us  148.51K
write_bmi2__u64_9B                               330.52%     2.04us  490.87K
write_unrol_u64_10B                                          6.83us  146.37K
write_bmi2__u64_10B                              341.73%     2.00us  500.20K
============================================================================
#endif
