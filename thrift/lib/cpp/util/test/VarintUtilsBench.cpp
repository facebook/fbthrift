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

#include <folly/Benchmark.h>
#include <folly/init/Init.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp/util/test/VarintUtilsTestUtil.h>

using namespace apache::thrift::util;

FOLLY_CREATE_QUAL_INVOKER_SUITE(write_unrolled, writeVarintUnrolled);
FOLLY_CREATE_QUAL_INVOKER_SUITE(write_bmi2, writeVarintBMI2);

template <typename Case, typename Fn>
void bench_write(Fn fn, size_t iters) {
  folly::BenchmarkSuspender braces;
  folly::IOBufQueue iobufQueue(folly::IOBufQueue::cacheChainLength());
  auto ints = Case::gen();
  while (iters--) {
    constexpr size_t kDesiredGrowth = 1 << 14;
    folly::io::QueueAppender c(&iobufQueue, kDesiredGrowth);
    c.ensure(ints.size() * 10);
    braces.dismissing([&] {
      for (auto v : ints) {
        fn(c, v);
      }
    });
    iobufQueue.clearAndTryReuseLargestBuffer();
  }
}

BENCHMARK(baseline_u8_1B, iters) {
  bench_write<u8_1b>(write_unrolled, iters);
}
BENCHMARK_DRAW_LINE();

#define BM_WRITE_LOOP(kind) \
  BENCHMARK(write_loop_##kind, i) { bench_write<kind>(write_unrolled, i); }
#define BM_REL_WRITE_BMI2(kind) \
  BENCHMARK_RELATIVE(write_bmi2_##kind, i) { bench_write<kind>(write_bmi2, i); }

#define BM_WRITE(kind) \
  BM_WRITE_LOOP(kind)  \
  BM_REL_WRITE_BMI2(kind)

BM_WRITE(u8_any)
BM_WRITE(u16_any)
BM_WRITE(u32_any)
BM_WRITE(u64_any)

BM_WRITE(u8_1b)
BM_WRITE(u8_2b)

BM_WRITE(u16_1b)
BM_WRITE(u16_2b)
BM_WRITE(u16_3b)

BM_WRITE(u32_1b)
BM_WRITE(u32_2b)
BM_WRITE(u32_3b)
BM_WRITE(u32_4b)
BM_WRITE(u32_5b)

BM_WRITE(u64_1b)
BM_WRITE(u64_2b)
BM_WRITE(u64_3b)
BM_WRITE(u64_4b)
BM_WRITE(u64_5b)
BM_WRITE(u64_6b)
BM_WRITE(u64_7b)
BM_WRITE(u64_8b)
BM_WRITE(u64_9b)
BM_WRITE(u64_10b)

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv, true);
  folly::runBenchmarks();
}

#if 0
// (Skylake) Intel(R) Xeon(R) Gold 6138 CPU @ 2.00GHz
$ buck run @mode/opt thrift/lib/cpp/util/test:varint_utils_bench -- --bm-min-iters 1000000
============================================================================
thrift/lib/cpp/util/test/VarintBenchmark.cpp    relative  time/iter  iters/s
============================================================================
baseline_u8_1B                                               1.45us  688.01K
----------------------------------------------------------------------------
write_loop_u8_any                                            1.94us  514.87K
write_bmi2_u8_any                                134.18%     1.45us  690.83K
write_loop_u16_any                                           3.99us  250.71K
write_bmi2_u16_any                               250.53%     1.59us  628.12K
write_loop_u32_any                                           5.43us  184.02K
write_bmi2_u32_any                               336.61%     1.61us  619.42K
write_loop_u64_any                                          10.37us   96.41K
write_bmi2_u64_any                               510.77%     2.03us  492.43K
write_loop_u8_1b                                             1.35us  741.16K
write_bmi2_u8_1b                                  99.97%     1.35us  740.96K
write_loop_u8_2b                                             2.46us  406.26K
write_bmi2_u8_2b                                 164.89%     1.49us  669.91K
write_loop_u16_1b                                            1.34us  748.00K
write_bmi2_u16_1b                                100.82%     1.33us  754.16K
write_loop_u16_2b                                            2.40us  415.90K
write_bmi2_u16_2b                                151.01%     1.59us  628.06K
write_loop_u16_3b                                            2.44us  409.41K
write_bmi2_u16_3b                                153.13%     1.60us  626.95K
write_loop_u32_1b                                            2.59us  386.17K
write_bmi2_u32_1b                                192.95%     1.34us  745.10K
write_loop_u32_2b                                            3.33us  300.39K
write_bmi2_u32_2b                                208.48%     1.60us  626.26K
write_loop_u32_3b                                            3.81us  262.77K
write_bmi2_u32_3b                                218.75%     1.74us  574.83K
write_loop_u32_4b                                            4.30us  232.69K
write_bmi2_u32_4b                                270.73%     1.59us  629.95K
write_loop_u32_5b                                            4.65us  214.88K
write_bmi2_u32_5b                                286.23%     1.63us  615.06K
write_loop_u64_1b                                            1.35us  740.24K
write_bmi2_u64_1b                                100.28%     1.35us  742.32K
write_loop_u64_2b                                            3.09us  324.13K
write_bmi2_u64_2b                                153.54%     2.01us  497.67K
write_loop_u64_3b                                            3.65us  274.07K
write_bmi2_u64_3b                                184.29%     1.98us  505.09K
write_loop_u64_4b                                            4.24us  236.05K
write_bmi2_u64_4b                                213.61%     1.98us  504.22K
write_loop_u64_5b                                            4.60us  217.40K
write_bmi2_u64_5b                                230.97%     1.99us  502.12K
write_loop_u64_6b                                            4.89us  204.61K
write_bmi2_u64_6b                                243.68%     2.01us  498.61K
write_loop_u64_7b                                            5.53us  180.80K
write_bmi2_u64_7b                                276.34%     2.00us  499.61K
write_loop_u64_8b                                            6.50us  153.80K
write_bmi2_u64_8b                                316.60%     2.05us  486.93K
write_loop_u64_9b                                            6.73us  148.51K
write_bmi2_u64_9b                                330.52%     2.04us  490.87K
/rite_loop_u64_10b                                           6.83us  146.37K
write_bmi2_u64_10b                               341.73%     2.00us  500.20K
============================================================================
#endif
