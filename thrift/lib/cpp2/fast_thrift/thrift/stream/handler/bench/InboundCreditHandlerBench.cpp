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

/**
 * InboundCreditHandler Microbenchmarks
 *
 * The two hot paths are:
 *   - onWrite of a Payload when credit remains (the steady-state producer
 *     path): a variant check, a credit decrement, and a forward.
 *   - onRead of a RequestN grant (the peer credit path): a variant check and a
 *     credit add.
 *
 * These guard those paths against regressions such as the credit counter
 * becoming atomic or the item gate acquiring per-frame allocation.
 */

#include <cstdint>
#include <utility>
#include <vector>

#include <folly/Benchmark.h>
#include <folly/init/Init.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/StreamEvents.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/handler/InboundCreditHandler.h>

using namespace folly;
using namespace apache::thrift::fast_thrift::channel_pipeline;
using namespace apache::thrift::fast_thrift::thrift::stream;

namespace {

class BenchCtx {
 public:
  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  Result fireRead(TypeErasedBox&&) noexcept { return Result::Success; }
  Result fireWrite(TypeErasedBox&&) noexcept { return Result::Success; }
  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  void fireException(folly::exception_wrapper&&) noexcept {}
  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  void fireEvent(StreamEvent, TypeErasedBox&&) noexcept {}
};

ThriftStreamMessage makeRequestN(uint64_t n) {
  return ThriftStreamMessage{.payload = RequestN{.n = n}};
}

ThriftStreamMessage makeItem() {
  return ThriftStreamMessage{.payload = Payload{.data = nullptr}};
}

BENCHMARK(OnWrite_ForwardWithCredit, iters) {
  BenchmarkSuspender suspender;

  InboundCreditHandler<BenchCtx> handler;
  BenchCtx ctx;
  // Grant one more credit than items so every item takes the pure forward path
  // and none hits the demand-exhausted (Backpressure) branch.
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(iters + 1)));

  std::vector<TypeErasedBox> items;
  items.reserve(iters);
  for (size_t i = 0; i < iters; ++i) {
    items.push_back(erase_and_box(makeItem()));
  }

  suspender.dismiss();

  for (auto& item : items) {
    auto result = handler.onWrite(ctx, std::move(item));
    doNotOptimizeAway(result);
  }
}

BENCHMARK(OnRead_RequestNGrant, iters) {
  BenchmarkSuspender suspender;

  InboundCreditHandler<BenchCtx> handler;
  BenchCtx ctx;

  std::vector<TypeErasedBox> grants;
  grants.reserve(iters);
  for (size_t i = 0; i < iters; ++i) {
    grants.push_back(erase_and_box(makeRequestN(1)));
  }

  suspender.dismiss();

  for (auto& grant : grants) {
    auto result = handler.onRead(ctx, std::move(grant));
    doNotOptimizeAway(result);
  }
}

} // namespace

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  runBenchmarks();
  return 0;
}
