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
 * BoundedWriteBufferHandler microbenchmarks.
 *
 * `OnWrite_PassThrough` measures the hot path (no backpressure): a flag check +
 * forward. `FillAndDrainBatched` measures the backpressure path: buffering a
 * batch into the ring and draining it on write-ready.
 */

#include <cstddef>
#include <utility>
#include <vector>

#include <folly/Benchmark.h>
#include <folly/init/Init.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/StreamEvents.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/handler/BoundedWriteBufferHandler.h>

using namespace folly;
using namespace apache::thrift::fast_thrift::channel_pipeline;
using namespace apache::thrift::fast_thrift::thrift::stream;

namespace {

// Local bench context with a configurable fireWrite result so the handler's
// backpressure state can be armed from outside. fireWrite does not consume the
// box (models a refusing downstream), so buffered items are retained.
class BenchCtx {
 public:
  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  Result fireRead(TypeErasedBox&&) noexcept { return Result::Success; }
  Result fireWrite(TypeErasedBox&&) noexcept { return nextWriteResult; }
  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  void fireException(folly::exception_wrapper&&) noexcept {}
  void awaitWriteReady() noexcept {}
  void cancelAwaitWriteReady() noexcept {}

  Result nextWriteResult{Result::Success};
};

// Open the credit gate so the benchmark measures the buffer's own paths rather
// than credit gating (the handler starts credit-paused).
template <typename Handler>
void openCredit(Handler& handler, BenchCtx& ctx) {
  handler.onEvent(ctx, StreamEvent::FlowControlResume, TypeErasedBox{});
}

ThriftStreamMessage makeItem() {
  return ThriftStreamMessage{.payload = Payload{.data = nullptr}};
}

BENCHMARK(OnWrite_PassThrough, iters) {
  BenchmarkSuspender suspender;

  BoundedWriteBufferHandler<BenchCtx> handler;
  BenchCtx ctx;
  openCredit(handler, ctx);

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

  BoundedWriteBufferHandler<BenchCtx> handler{
      BoundedWriteBufferConfig{.maxBufferedElements = kBatch}};
  BenchCtx ctx;
  openCredit(handler, ctx);

  const size_t cycles = (iters + kBatch - 1) / kBatch;

  suspender.dismiss();

  for (size_t c = 0; c < cycles; ++c) {
    // Fill the ring under backpressure, then drain it.
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
