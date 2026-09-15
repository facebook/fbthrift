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
 * PayloadPrefetchHandler microbenchmarks.
 *
 * `OnWrite_PassThrough` measures the hot path: a produced payload is sent
 * straight through while send-credit and transport are both open.
 * `FillAndDrainBatched` measures the held path: a batch is held behind
 * transport backpressure, then drained on write-ready.
 */

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <folly/Benchmark.h>
#include <folly/init/Init.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/handler/PayloadPrefetchHandler.h>

using namespace folly;
using namespace apache::thrift::fast_thrift::channel_pipeline;
using namespace apache::thrift::fast_thrift::thrift::stream;

namespace {

// Local bench context with a configurable fireWrite result so the handler's
// transport backpressure state can be armed from outside. fireRead absorbs the
// demand the buffer meters to (an absent) producer.
class BenchCtx {
 public:
  Result fireRead(TypeErasedBox&&) noexcept { return Result::Success; }
  Result fireWrite(TypeErasedBox&&) noexcept { return nextWriteResult; }
  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  void fireException(folly::exception_wrapper&&) noexcept {}
  void awaitWriteReady() noexcept {}
  void cancelAwaitWriteReady() noexcept {}

  Result nextWriteResult{Result::Success};
};

// Grant send-credit by delivering a RequestN inbound.
template <typename Handler>
void grant(Handler& handler, BenchCtx& ctx, uint64_t n) {
  (void)handler.onRead(
      ctx, erase_and_box(ThriftStreamMessage{.payload = RequestN{.n = n}}));
}

ThriftStreamMessage makeItem() {
  return ThriftStreamMessage{.payload = Payload{.data = nullptr}};
}

BENCHMARK(OnWrite_PassThrough, iters) {
  BenchmarkSuspender suspender;

  PayloadPrefetchHandler<BenchCtx> handler{
      PayloadPrefetchConfig{.capacity = 1024, .replenishThreshold = 512}};
  BenchCtx ctx;
  grant(handler, ctx, iters + 1); // enough credit to send every payload

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

constexpr size_t kBatch = 8;

BENCHMARK(FillAndDrainBatched, iters) {
  BenchmarkSuspender suspender;

  PayloadPrefetchHandler<BenchCtx> handler{
      PayloadPrefetchConfig{.capacity = kBatch, .replenishThreshold = 1}};
  BenchCtx ctx;
  grant(handler, ctx, iters + kBatch); // ample send-credit; transport gates

  const size_t cycles = (iters + kBatch - 1) / kBatch;

  suspender.dismiss();

  for (size_t c = 0; c < cycles; ++c) {
    // Hold a batch behind transport backpressure, then drain it on write-ready.
    ctx.nextWriteResult = Result::Backpressure;
    for (size_t i = 0; i < kBatch; ++i) {
      (void)handler.onWrite(ctx, erase_and_box(makeItem()));
    }
    ctx.nextWriteResult = Result::Success;
    handler.onWriteReady(ctx);
    doNotOptimizeAway(&handler);
  }
}

} // namespace

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  runBenchmarks();
  return 0;
}
