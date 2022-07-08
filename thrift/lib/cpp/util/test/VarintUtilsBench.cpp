/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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
void bench_write(size_t iters, Fn fn, Case) {
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

#define BM_WRITE_LOOP(kind) \
  BENCHMARK_NAMED_PARAM(bench_write, kind##_unrolled, write_unrolled, kind())
#define BM_REL_WRITE_BMI2(kind) \
  BENCHMARK_RELATIVE_NAMED_PARAM(bench_write, kind##_bmi2, write_bmi2, kind())

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

template <typename Case>
void bench_read(size_t iters, Case) {
  folly::IOBufQueue iobufQueue(folly::IOBufQueue::cacheChainLength());
  const size_t kDesiredGrowth = sysconf(_SC_PAGE_SIZE);
  folly::io::QueueAppender c(&iobufQueue, kDesiredGrowth);
  BENCHMARK_SUSPEND {
    auto ints = Case::gen();
    c.ensure(ints.size() * 10);
    for (auto v : ints) {
      writeVarintBMI2(c, v);
    }
  }
  folly::io::Cursor rcursor(iobufQueue.front());
  while (iters--) {
    typename Case::integer_type val = 0;
    if (rcursor.isAtEnd()) {
      rcursor = folly::io::Cursor(iobufQueue.front());
    }
    readVarint(rcursor, val);
  }
}

BENCHMARK_DRAW_LINE();

BENCHMARK_NAMED_PARAM(bench_read, u8_any, u8_any())
BENCHMARK_NAMED_PARAM(bench_read, u8_1b, u8_1b())
BENCHMARK_NAMED_PARAM(bench_read, u8_2b, u8_2b())

BENCHMARK_NAMED_PARAM(bench_read, u16_any, u16_any())
BENCHMARK_NAMED_PARAM(bench_read, u16_1b, u16_1b())
BENCHMARK_NAMED_PARAM(bench_read, u16_2b, u16_2b())
BENCHMARK_NAMED_PARAM(bench_read, u16_3b, u16_3b())

BENCHMARK_NAMED_PARAM(bench_read, u32_any, u32_any())
BENCHMARK_NAMED_PARAM(bench_read, u32_1b, u32_1b())
BENCHMARK_NAMED_PARAM(bench_read, u32_2b, u32_2b())
BENCHMARK_NAMED_PARAM(bench_read, u32_3b, u32_3b())
BENCHMARK_NAMED_PARAM(bench_read, u32_4b, u32_4b())
BENCHMARK_NAMED_PARAM(bench_read, u32_5b, u32_5b())

BENCHMARK_NAMED_PARAM(bench_read, u64_any, u64_any())
BENCHMARK_NAMED_PARAM(bench_read, u64_1b, u64_1b())
BENCHMARK_NAMED_PARAM(bench_read, u64_2b, u64_2b())
BENCHMARK_NAMED_PARAM(bench_read, u64_3b, u64_3b())
BENCHMARK_NAMED_PARAM(bench_read, u64_4b, u64_4b())
BENCHMARK_NAMED_PARAM(bench_read, u64_5b, u64_5b())
BENCHMARK_NAMED_PARAM(bench_read, u64_6b, u64_6b())
BENCHMARK_NAMED_PARAM(bench_read, u64_7b, u64_7b())
BENCHMARK_NAMED_PARAM(bench_read, u64_8b, u64_8b())
BENCHMARK_NAMED_PARAM(bench_read, u64_9b, u64_9b())
BENCHMARK_NAMED_PARAM(bench_read, u64_10b, u64_10b())

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv, true);
  folly::runBenchmarks();
}

#if 0
$ buck run @mode/opt-clang-thinlto --config=cxx.use_default_autofdo_profile=false \
    //thrift/lib/cpu/util/test:varint_utils_bench -- --bm_min_iters=1000000
============================================================================
thrift/lib/cpp/util/test/VarintUtilsBench.cpp   relative  time/iter  iters/s
============================================================================
bench_write(u8_any_unrolled)                                 2.32us  431.26K
bench_write(u8_any_bmi2)                         114.19%     2.03us  492.46K
bench_write(u16_any_unrolled)                                5.89us  169.88K
bench_write(u16_any_bmi2)                        309.18%     1.90us  525.24K
bench_write(u32_any_unrolled)                                4.76us  210.02K
bench_write(u32_any_bmi2)                        233.53%     2.04us  490.46K
bench_write(u64_any_unrolled)                               11.30us   88.53K
bench_write(u64_any_bmi2)                        405.43%     2.79us  358.92K
bench_write(u8_1b_unrolled)                                  1.50us  665.71K
bench_write(u8_1b_bmi2)                          102.35%     1.47us  681.36K
bench_write(u8_2b_unrolled)                                  2.25us  443.95K
bench_write(u8_2b_bmi2)                          127.76%     1.76us  567.17K
bench_write(u16_1b_unrolled)                                 1.57us  636.62K
bench_write(u16_1b_bmi2)                         101.12%     1.55us  643.75K
bench_write(u16_2b_unrolled)                                 2.97us  336.28K
bench_write(u16_2b_bmi2)                         122.31%     2.43us  411.30K
bench_write(u16_3b_unrolled)                                 3.00us  333.84K
bench_write(u16_3b_bmi2)                         150.83%     1.99us  503.53K
bench_write(u32_1b_unrolled)                                 1.63us  614.07K
bench_write(u32_1b_bmi2)                         104.41%     1.56us  641.14K
bench_write(u32_2b_unrolled)                                 2.74us  365.19K
bench_write(u32_2b_bmi2)                         149.83%     1.83us  547.18K
bench_write(u32_3b_unrolled)                                 3.41us  293.63K
bench_write(u32_3b_bmi2)                         166.78%     2.04us  489.71K
bench_write(u32_4b_unrolled)                                 3.73us  268.01K
bench_write(u32_4b_bmi2)                         202.50%     1.84us  542.72K
bench_write(u32_5b_unrolled)                                 4.09us  244.42K
bench_write(u32_5b_bmi2)                         205.56%     1.99us  502.42K
bench_write(u64_1b_unrolled)                                 1.66us  602.68K
bench_write(u64_1b_bmi2)                         100.67%     1.65us  606.73K
bench_write(u64_2b_unrolled)                                 3.81us  262.77K
bench_write(u64_2b_bmi2)                         121.63%     3.13us  319.60K
bench_write(u64_3b_unrolled)                                 4.32us  231.41K
bench_write(u64_3b_bmi2)                         148.34%     2.91us  343.27K
bench_write(u64_4b_unrolled)                                 4.95us  202.02K
bench_write(u64_4b_bmi2)                         191.90%     2.58us  387.68K
bench_write(u64_5b_unrolled)                                 5.71us  175.26K
bench_write(u64_5b_bmi2)                         188.98%     3.02us  331.21K
bench_write(u64_6b_unrolled)                                 5.85us  170.95K
bench_write(u64_6b_bmi2)                         219.50%     2.67us  375.23K
bench_write(u64_7b_unrolled)                                 6.87us  145.49K
bench_write(u64_7b_bmi2)                         235.90%     2.91us  343.21K
bench_write(u64_8b_unrolled)                                 7.34us  136.17K
bench_write(u64_8b_bmi2)                         243.54%     3.02us  331.62K
bench_write(u64_9b_unrolled)                                 7.86us  127.25K
bench_write(u64_9b_bmi2)                         233.07%     3.37us  296.57K
bench_write(u64_10b_unrolled)                                8.14us  122.92K
bench_write(u64_10b_bmi2)                        250.79%     3.24us  308.27K
----------------------------------------------------------------------------
bench_read(u8_any)                                           6.21ns  161.05M
bench_read(u8_1b)                                            1.61ns  621.54M
bench_read(u8_2b)                                            2.02ns  496.05M
bench_read(u16_any)                                          4.64ns  215.54M
bench_read(u16_1b)                                           1.66ns  601.75M
bench_read(u16_2b)                                           3.07ns  326.04M
bench_read(u16_3b)                                           2.32ns  430.36M
bench_read(u32_any)                                          4.62ns  216.60M
bench_read(u32_1b)                                           1.29ns  776.61M
bench_read(u32_2b)                                           2.46ns  406.32M
bench_read(u32_3b)                                           2.23ns  447.59M
bench_read(u32_4b)                                           4.15ns  241.11M
bench_read(u32_5b)                                           3.17ns  315.28M
bench_read(u64_any)                                         11.06ns   90.43M
bench_read(u64_1b)                                           1.29ns  772.55M
bench_read(u64_2b)                                           4.75ns  210.44M
bench_read(u64_3b)                                           3.66ns  272.96M
bench_read(u64_4b)                                           4.81ns  208.07M
bench_read(u64_5b)                                           5.07ns  197.09M
bench_read(u64_6b)                                           5.68ns  176.13M
bench_read(u64_7b)                                           6.59ns  151.84M
bench_read(u64_8b)                                           6.77ns  147.64M
bench_read(u64_9b)                                           8.03ns  124.54M
bench_read(u64_10b)                                          7.72ns  129.51M
============================================================================
#endif
