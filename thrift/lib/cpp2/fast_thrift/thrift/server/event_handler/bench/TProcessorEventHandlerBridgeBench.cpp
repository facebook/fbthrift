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

#include <thrift/lib/cpp2/fast_thrift/thrift/server/event_handler/TProcessorEventHandlerBridge.h>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include <folly/Benchmark.h>
#include <folly/init/Init.h>
#include <folly/io/async/ScopedEventBaseThread.h>

namespace apache::thrift::fast_thrift::thrift::server {
namespace {

constexpr std::array<std::string_view, 8> kMethods{
    "get", "set", "del", "leaseGet", "leaseSet", "gets", "cas", "touch"};

constexpr std::array<std::string_view, 8> kQualifiedMethods{
    "UcacheService.get",
    "UcacheService.set",
    "UcacheService.del",
    "UcacheService.leaseGet",
    "UcacheService.leaseSet",
    "UcacheService.gets",
    "UcacheService.cas",
    "UcacheService.touch"};

constexpr std::string_view kLongMethod{
    "taoMultiShardTransactionReserveRequest"};

folly::EventBase* benchmarkEventBase() {
  static folly::ScopedEventBaseThread eventBaseThread;
  return eventBaseThread.getEventBase();
}

BENCHMARK(ThriftRequestContext_EvbBump, iters) {
  auto* eventBase = benchmarkEventBase();
  folly::BenchmarkSuspender suspender;
  eventBase->runInEventBaseThreadAndWait([&] {
    (void)apache::thrift::fast_thrift::mem::EvbAllocator::getOrCreate(
        *eventBase);
  });
  suspender.dismiss();

  eventBase->runInEventBaseThreadAndWait([&] {
    for (std::size_t i = 0; i < iters; ++i) {
      auto context = makeThriftRequestContext(*eventBase);
      folly::doNotOptimizeAway(context.get());
    }
  });
}

BENCHMARK_RELATIVE(ThriftRequestContext_Heap, iters) {
  auto* eventBase = benchmarkEventBase();
  eventBase->runInEventBaseThreadAndWait([&] {
    for (std::size_t i = 0; i < iters; ++i) {
      auto context = std::make_unique<ThriftRequestContext>();
      folly::doNotOptimizeAway(context.get());
    }
  });
}

BENCHMARK(Cpp2RequestContext_LongMethod, iters) {
  const std::string method(kLongMethod);
  for (std::size_t i = 0; i < iters; ++i) {
    apache::thrift::Cpp2RequestContext context{nullptr};
    apache::thrift::detail::Cpp2RequestContextUnsafeAPI(context)
        .setBorrowedMethodName(method);
    folly::doNotOptimizeAway(context.getMethodName().data());
  }
}

void runLookupBenchmark(std::size_t iters, std::size_t methodCount) {
  folly::BenchmarkSuspender suspender;
  ThriftServerMethodMetadataRegistry methods;
  for (std::size_t i = 0; i < kMethods.size(); ++i) {
    methods.add({
        .serviceName = "UcacheService",
        .definingServiceName = "UcacheService",
        .methodName = kMethods[i],
        .qualifiedMethodName = kQualifiedMethods[i],
    });
  }
  for (std::size_t i = 0; i < methodCount; ++i) {
    folly::doNotOptimizeAway(methods.find(kMethods[i]));
  }
  suspender.dismiss();

  for (std::size_t i = 0; i < iters; ++i) {
    folly::doNotOptimizeAway(methods.find(kMethods[i % methodCount]));
  }
}

BENCHMARK(Lookup_OneHotMethod, iters) {
  runLookupBenchmark(iters, 1);
}

BENCHMARK(Lookup_TwoAlternatingMethods, iters) {
  runLookupBenchmark(iters, 2);
}

BENCHMARK(Lookup_FourRoundRobinMethods, iters) {
  runLookupBenchmark(iters, 4);
}

BENCHMARK(Lookup_EightRoundRobinMethods, iters) {
  runLookupBenchmark(iters, 8);
}

} // namespace
} // namespace apache::thrift::fast_thrift::thrift::server

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
