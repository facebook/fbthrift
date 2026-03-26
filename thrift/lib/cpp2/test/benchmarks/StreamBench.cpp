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

#include <folly/Benchmark.h>
#include <folly/coro/BlockingWait.h>
#include <folly/init/Init.h>
#include <folly/logging/LoggerDB.h>
#include <thrift/lib/cpp2/test/benchmarks/gen-cpp2/StreamBenchService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace apache::thrift;
using namespace apache::thrift::benchmark;

struct StreamHandler : ServiceHandler<StreamBenchService> {
  ServerStream<int32_t> range(int32_t from, int32_t to) override {
    for (int32_t i = from; i <= to; ++i) {
      co_yield int32_t(i);
    }
  }

  ServerStream<std::string> rangePayload(
      int32_t count, int32_t payloadBytes) override {
    auto payload = std::string(payloadBytes, 'x');
    for (int32_t i = 0; i < count; ++i) {
      co_yield std::string(payload);
    }
  }

  ServerStream<int32_t> rangeWithWork(
      int32_t from, int32_t to, int32_t workIterations) override {
    for (int32_t i = from; i <= to; ++i) {
      int32_t val = i;
      for (int32_t w = 0; w < workIterations; ++w) {
        folly::doNotOptimizeAway(val);
      }
      co_yield int32_t(val);
    }
  }
};

template <int32_t NumElements>
void streamBench(size_t iters) {
  folly::BenchmarkSuspender susp;
  ScopedServerInterfaceThread runner(std::make_shared<StreamHandler>());
  auto client = runner.newClient<apache::thrift::Client<StreamBenchService>>();

  // Warm up the connection
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    auto gen = (co_await client->co_range(0, 0)).toAsyncGenerator();
    while (co_await gen.next()) {
    }
  }());

  susp.dismiss();

  while (iters--) {
    folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
      auto gen =
          (co_await client->co_range(0, NumElements - 1)).toAsyncGenerator();
      while (auto val = co_await gen.next()) {
        folly::doNotOptimizeAway(*val);
      }
    }());
  }

  susp.rehire();
}

template <int32_t NumElements, int32_t PayloadBytes>
void payloadBench(size_t iters) {
  folly::BenchmarkSuspender susp;
  ScopedServerInterfaceThread runner(std::make_shared<StreamHandler>());
  auto client = runner.newClient<apache::thrift::Client<StreamBenchService>>();

  // Warm up the connection
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    auto gen = (co_await client->co_rangePayload(1, 1)).toAsyncGenerator();
    while (co_await gen.next()) {
    }
  }());

  susp.dismiss();

  while (iters--) {
    folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
      auto gen = (co_await client->co_rangePayload(NumElements, PayloadBytes))
                     .toAsyncGenerator();
      while (auto val = co_await gen.next()) {
        folly::doNotOptimizeAway(*val);
      }
    }());
  }

  susp.rehire();
}

template <int32_t NumElements, int32_t WorkIterations>
void workBench(size_t iters) {
  folly::BenchmarkSuspender susp;
  ScopedServerInterfaceThread runner(std::make_shared<StreamHandler>());
  auto client = runner.newClient<apache::thrift::Client<StreamBenchService>>();

  // Warm up the connection
  folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
    auto gen = (co_await client->co_range(0, 0)).toAsyncGenerator();
    while (co_await gen.next()) {
    }
  }());

  susp.dismiss();

  while (iters--) {
    folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
      auto gen = (co_await client->co_rangeWithWork(
                      0, NumElements - 1, WorkIterations))
                     .toAsyncGenerator();
      while (auto val = co_await gen.next()) {
        folly::doNotOptimizeAway(*val);
      }
    }());
  }

  susp.rehire();
}

// --- Stream element count benchmarks ---

BENCHMARK(Rocket_ServerStream_1_Bench, iters) {
  streamBench<1>(iters);
}
BENCHMARK(Rocket_ServerStream_100_Bench, iters) {
  streamBench<100>(iters);
}
BENCHMARK(Rocket_ServerStream_10000_Bench, iters) {
  streamBench<10000>(iters);
}

// --- Payload size benchmarks (100 elements) ---

BENCHMARK(Rocket_ServerStream_Payload_64B_Bench, iters) {
  payloadBench<100, 64>(iters);
}
BENCHMARK(Rocket_ServerStream_Payload_1KB_Bench, iters) {
  payloadBench<100, 1024>(iters);
}
BENCHMARK(Rocket_ServerStream_Payload_64KB_Bench, iters) {
  payloadBench<100, 65536>(iters);
}

// --- Server work benchmarks (100 elements) ---

BENCHMARK(Rocket_ServerStream_Work_100_Bench, iters) {
  workBench<100, 100>(iters);
}
BENCHMARK(Rocket_ServerStream_Work_10000_Bench, iters) {
  workBench<100, 10000>(iters);
}

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  folly::LoggerDB::get().setLevel("", folly::LogLevel::ERR);
  folly::runBenchmarks();
  return 0;
}
