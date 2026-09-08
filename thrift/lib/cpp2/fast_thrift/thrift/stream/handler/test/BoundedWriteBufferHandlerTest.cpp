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

#include <thrift/lib/cpp2/fast_thrift/thrift/stream/handler/BoundedWriteBufferHandler.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <folly/ExceptionWrapper.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineImpl.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockAdapters.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/StreamEvents.h>

namespace apache::thrift::fast_thrift::thrift::stream {

namespace {

using channel_pipeline::erase_and_box;
using channel_pipeline::Result;
using channel_pipeline::TypeErasedBox;

// ContextApi stand-in. The tests assert only on the handler's observable
// contract — what it forwards (and in what order) and the flow-control result —
// never on internal buffer state.
//
// `fireWrite` always takes the element, modeling the sink contract that a
// Backpressure result means the downstream consumed it. `duringFireWrite`, if
// set, runs inside fireWrite before it returns — used to model the credit
// handler firing a synchronous FlowControlPause as it forwards the element that
// exhausts demand.
class FakeContext {
 public:
  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  Result fireRead(TypeErasedBox&& msg) noexcept {
    reads.push_back(std::move(msg));
    return Result::Success;
  }

  Result fireWrite(TypeErasedBox&& msg) noexcept {
    auto m = msg.take<ThriftStreamMessage>();
    if (m.payload.is<Payload>()) {
      if (auto& p = m.payload.get<Payload>(); p.data) {
        forwardedTags.push_back(
            folly::io::Cursor(p.data.get()).read<uint8_t>());
      }
    } else if (m.payload.is<RequestN>()) {
      forwardedGrants.push_back(m.payload.get<RequestN>().n);
    } else if (m.payload.is<Complete>()) {
      ++completions;
      completionAfterNTags = forwardedTags.size();
    }
    if (duringFireWrite) {
      duringFireWrite();
    }
    return nextWriteResult;
  }

  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  void fireException(folly::exception_wrapper&& e) noexcept {
    exceptions.push_back(std::move(e));
  }

  void awaitWriteReady() noexcept {}
  void cancelAwaitWriteReady() noexcept {}

  Result nextWriteResult{Result::Success};
  std::function<void()> duringFireWrite;
  std::vector<TypeErasedBox> reads;
  std::vector<uint8_t> forwardedTags;
  std::vector<uint64_t> forwardedGrants;
  std::vector<folly::exception_wrapper> exceptions;
  size_t completions{0};
  // Number of payloads already forwarded when the completion went out — lets a
  // test assert completion is emitted after all payloads.
  size_t completionAfterNTags{0};
};

using Handler = BoundedWriteBufferHandler<FakeContext>;

// Open the credit gate: models InboundCreditHandler firing FlowControlResume on
// a peer credit grant.
void resumeCredit(Handler& handler, FakeContext& ctx) {
  handler.onEvent(ctx, StreamEvent::FlowControlResume, TypeErasedBox{});
}

// Close the credit gate: models FlowControlPause on credit exhaustion.
void pauseCredit(Handler& handler, FakeContext& ctx) {
  handler.onEvent(ctx, StreamEvent::FlowControlPause, TypeErasedBox{});
}

ThriftStreamMessage makeItem(uint8_t tag) {
  return ThriftStreamMessage{
      .payload = Payload{.data = folly::IOBuf::copyBuffer(&tag, sizeof(tag))}};
}

ThriftStreamMessage makeRequestN(uint64_t n) {
  return ThriftStreamMessage{.payload = RequestN{.n = n}};
}

ThriftStreamMessage makeComplete() {
  return ThriftStreamMessage{.payload = Complete{}};
}

} // namespace

TEST(BoundedWriteBufferHandlerTest, ForwardsPayloadWhenResumedAndBufferEmpty) {
  Handler handler;
  FakeContext ctx;
  resumeCredit(handler, ctx);

  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(1))), Result::Success);
  const std::vector<uint8_t> expected{1};
  EXPECT_EQ(ctx.forwardedTags, expected);
}

// =============================================================================
// Credit gate: the buffer holds payloads while credit is paused and drains on
// resume. It starts paused — no credit until the peer's first RequestN.
// =============================================================================

TEST(BoundedWriteBufferHandlerTest, StartsCreditPausedSoPayloadIsHeld) {
  Handler handler;
  FakeContext ctx;

  // No FlowControlResume yet: the payload cannot be sent, so it is held.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(1))), Result::Success);
  EXPECT_TRUE(ctx.forwardedTags.empty())
      << "no credit -> payload held, not sent";
}

TEST(BoundedWriteBufferHandlerTest, ResumeDrainsHeldPayloadsInFifoOrder) {
  Handler handler;
  FakeContext ctx;

  // Credit paused: all three payloads are held in order.
  (void)handler.onWrite(ctx, erase_and_box(makeItem(1)));
  (void)handler.onWrite(ctx, erase_and_box(makeItem(2)));
  (void)handler.onWrite(ctx, erase_and_box(makeItem(3)));
  EXPECT_TRUE(ctx.forwardedTags.empty());

  resumeCredit(handler, ctx);
  const std::vector<uint8_t> expected{1, 2, 3};
  EXPECT_EQ(ctx.forwardedTags, expected);
}

TEST(BoundedWriteBufferHandlerTest, PauseHoldsSubsequentPayloads) {
  Handler handler;
  FakeContext ctx;
  resumeCredit(handler, ctx);

  // Credit available: the first payload flows.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(1))), Result::Success);
  // Credit exhausts: the next payload is held.
  pauseCredit(handler, ctx);
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(2))), Result::Success);
  const std::vector<uint8_t> afterPause{1};
  EXPECT_EQ(ctx.forwardedTags, afterPause) << "held while credit is paused";

  // Credit returns: the held payload drains.
  resumeCredit(handler, ctx);
  const std::vector<uint8_t> afterResume{1, 2};
  EXPECT_EQ(ctx.forwardedTags, afterResume);
}

// =============================================================================
// Only payloads are gated; control frames bypass the buffer
// =============================================================================

TEST(BoundedWriteBufferHandlerTest, ControlFrameBypassesWhileHeld) {
  Handler handler;
  FakeContext ctx;

  // Credit paused: a payload is held.
  (void)handler.onWrite(ctx, erase_and_box(makeItem(1)));
  // A control frame (RequestN) still goes straight to the wire.
  (void)handler.onWrite(ctx, erase_and_box(makeRequestN(9)));

  const std::vector<uint64_t> grants{9};
  EXPECT_EQ(ctx.forwardedGrants, grants) << "control frame was not buffered";
  EXPECT_TRUE(ctx.forwardedTags.empty()) << "payload 1 is still held";
}

// =============================================================================
// Transport gate: a downstream Backpressure holds subsequent payloads until
// write-ready, independent of credit.
// =============================================================================

TEST(BoundedWriteBufferHandlerTest, TransportBackpressureHoldsSubsequent) {
  Handler handler;
  FakeContext ctx;
  resumeCredit(handler, ctx);
  ctx.nextWriteResult = Result::Backpressure;

  // Payload 1 is consumed but the transport is now congested.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeItem(1))), Result::Backpressure);
  // Payload 2 is held until write-ready.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(2))), Result::Success);
  const std::vector<uint8_t> beforeDrain{1};
  EXPECT_EQ(ctx.forwardedTags, beforeDrain);

  ctx.nextWriteResult = Result::Success;
  handler.onWriteReady(ctx);
  const std::vector<uint8_t> afterDrain{1, 2};
  EXPECT_EQ(ctx.forwardedTags, afterDrain);
}

TEST(
    BoundedWriteBufferHandlerTest,
    FillingPayloadBackpressuresThenOverflowErrors) {
  Handler handler{BoundedWriteBufferConfig{.maxBufferedElements = 2}};
  FakeContext ctx;
  resumeCredit(handler, ctx);
  ctx.nextWriteResult = Result::Backpressure;

  // Enter backpressure: payload 1 is consumed, not buffered.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeItem(1))), Result::Backpressure);
  // Buffer up to the bound: 2 has room, 3 fills the buffer (held +
  // Backpressure).
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(2))), Result::Success);
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeItem(3))), Result::Backpressure);
  // Payload 4 overran the signal: dropped with Error.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(4))), Result::Error);

  ctx.nextWriteResult = Result::Success;
  handler.onWriteReady(ctx);
  const std::vector<uint8_t> expected{1, 2, 3}; // 4 was never held
  EXPECT_EQ(ctx.forwardedTags, expected);
}

TEST(BoundedWriteBufferHandlerTest, ZeroBoundDropsOverflowWithError) {
  Handler handler{BoundedWriteBufferConfig{.maxBufferedElements = 0}};
  FakeContext ctx;
  resumeCredit(handler, ctx);
  ctx.nextWriteResult = Result::Backpressure;

  // Payload 1 triggers backpressure and is consumed.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeItem(1))), Result::Backpressure);
  // With no buffer capacity, the next payload is dropped with Error.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(2))), Result::Error);

  const std::vector<uint8_t> sent{1};
  EXPECT_EQ(ctx.forwardedTags, sent);
}

TEST(
    BoundedWriteBufferHandlerTest, DrainStopsAndResumesUnderTransportPressure) {
  Handler handler;
  FakeContext ctx;
  resumeCredit(handler, ctx);
  ctx.nextWriteResult = Result::Backpressure;

  (void)handler.onWrite(ctx, erase_and_box(makeItem(1))); // consumed
  (void)handler.onWrite(ctx, erase_and_box(makeItem(2))); // buffered
  (void)handler.onWrite(ctx, erase_and_box(makeItem(3))); // buffered

  // Still congested: the drain sends one element then stops, losing nothing.
  handler.onWriteReady(ctx);
  const std::vector<uint8_t> partial{1, 2};
  EXPECT_EQ(ctx.forwardedTags, partial);

  // Recovered: the rest drains in order.
  ctx.nextWriteResult = Result::Success;
  handler.onWriteReady(ctx);
  const std::vector<uint8_t> expected{1, 2, 3};
  EXPECT_EQ(ctx.forwardedTags, expected);
}

// =============================================================================
// The two gates are disjoint: a transport write-ready must not drain into a
// still-paused credit gate (that is exactly the drop this design prevents).
// =============================================================================

TEST(BoundedWriteBufferHandlerTest, TransportWakeDoesNotBypassCreditGate) {
  Handler handler;
  FakeContext ctx;
  resumeCredit(handler, ctx);

  // The forward that spends the last credit: the credit handler fires
  // FlowControlPause synchronously and the write returns Backpressure. The
  // buffer must read this as credit exhaustion, not transport congestion.
  ctx.nextWriteResult = Result::Backpressure;
  ctx.duringFireWrite = [&] { pauseCredit(handler, ctx); };
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeItem(1))), Result::Backpressure);
  ctx.duringFireWrite = nullptr;
  ctx.nextWriteResult = Result::Success;

  // Subsequent payload is held by the credit gate.
  (void)handler.onWrite(ctx, erase_and_box(makeItem(2)));
  const std::vector<uint8_t> onlyFirst{1};
  EXPECT_EQ(ctx.forwardedTags, onlyFirst);

  // A transport write-ready must NOT drain the credit-paused payload — doing so
  // would forward it into the credit handler's exhausted-demand Error.
  handler.onWriteReady(ctx);
  EXPECT_EQ(ctx.forwardedTags, onlyFirst)
      << "transport wake must not bypass the credit gate";

  // Only a credit resume releases it.
  resumeCredit(handler, ctx);
  const std::vector<uint8_t> both{1, 2};
  EXPECT_EQ(ctx.forwardedTags, both);
}

// =============================================================================
// Completion is ordered after payloads, but is not gated by credit or transport
// =============================================================================

TEST(BoundedWriteBufferHandlerTest, CompletionForwardedWhenBufferEmpty) {
  Handler handler;
  FakeContext ctx;
  resumeCredit(handler, ctx);

  // No payloads buffered: completion goes straight through.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeComplete())), Result::Success);
  EXPECT_EQ(ctx.completions, 1u);
}

TEST(
    BoundedWriteBufferHandlerTest, CompletionForwardedWhileCreditPausedIfIdle) {
  Handler handler;
  FakeContext ctx;

  // Credit paused but no payloads pending: completion is not gated by credit
  // (it carries no data), so it is forwarded immediately.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeComplete())), Result::Success);
  EXPECT_EQ(ctx.completions, 1u);
}

TEST(BoundedWriteBufferHandlerTest, CompletionHeldUntilBufferedPayloadsDrain) {
  Handler handler;
  FakeContext ctx;

  // Credit paused: payloads are buffered.
  (void)handler.onWrite(ctx, erase_and_box(makeItem(1)));
  (void)handler.onWrite(ctx, erase_and_box(makeItem(2)));

  // Completion arrives while payloads are buffered: it is held.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeComplete())), Result::Success);
  EXPECT_EQ(ctx.completions, 0u) << "completion held until payloads drain";

  resumeCredit(handler, ctx);
  const std::vector<uint8_t> expected{1, 2};
  EXPECT_EQ(ctx.forwardedTags, expected);
  EXPECT_EQ(ctx.completions, 1u);
  EXPECT_EQ(ctx.completionAfterNTags, 2u)
      << "completion is emitted after every payload";
}

TEST(BoundedWriteBufferHandlerTest, PayloadAfterCompletionIsDropped) {
  Handler handler;
  FakeContext ctx;

  // Credit paused so the payload is buffered and completion is held after it.
  (void)handler.onWrite(ctx, erase_and_box(makeItem(1)));
  (void)handler.onWrite(ctx, erase_and_box(makeComplete()));

  // A payload after completion is a protocol violation and is dropped.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(2))), Result::Error);
}

TEST(BoundedWriteBufferHandlerTest, PayloadAfterForwardedCompletionIsDropped) {
  Handler handler;
  FakeContext ctx;
  resumeCredit(handler, ctx);

  // Completion forwarded immediately (buffer empty). End-of-stream is now
  // reached even though nothing is pending.
  (void)handler.onWrite(ctx, erase_and_box(makeComplete()));
  EXPECT_EQ(ctx.completions, 1u);

  // A payload after end-of-stream is a protocol violation and is dropped.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem(1))), Result::Error);
  EXPECT_TRUE(ctx.forwardedTags.empty());
}

TEST(BoundedWriteBufferHandlerTest, SecondCompletionAfterForwardIsRejected) {
  Handler handler;
  FakeContext ctx;
  resumeCredit(handler, ctx);

  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeComplete())), Result::Success);
  // A repeated Complete must not double-emit end-of-stream.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeComplete())), Result::Error);
  EXPECT_EQ(ctx.completions, 1u);
}

TEST(BoundedWriteBufferHandlerTest, SecondCompletionWhileHeldIsRejected) {
  Handler handler;
  FakeContext ctx;

  // Credit paused: a payload is buffered and the completion is held after it.
  (void)handler.onWrite(ctx, erase_and_box(makeItem(1)));
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeComplete())), Result::Success);
  // A second completion while one is already pending is a protocol violation.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeComplete())), Result::Error);

  resumeCredit(handler, ctx);
  const std::vector<uint8_t> expected{1};
  EXPECT_EQ(ctx.forwardedTags, expected);
  EXPECT_EQ(ctx.completions, 1u) << "exactly one completion is emitted";
}

// =============================================================================
// Lifecycle
// =============================================================================

TEST(BoundedWriteBufferHandlerTest, BufferSurvivesPipelineInactive) {
  Handler handler;
  FakeContext ctx;

  (void)handler.onWrite(
      ctx, erase_and_box(makeItem(1))); // held (credit paused)
  (void)handler.onWrite(ctx, erase_and_box(makeItem(2)));
  (void)handler.onWrite(ctx, erase_and_box(makeItem(3)));

  handler.onPipelineInactive(ctx);

  // Buffered payloads are unsent output; a transport pause must not drop them.
  resumeCredit(handler, ctx);
  const std::vector<uint8_t> expected{1, 2, 3};
  EXPECT_EQ(ctx.forwardedTags, expected);
}

// =============================================================================
// Integration: embedded in a real PipelineImpl parameterized with StreamEvent.
// Catches wiring bugs the FakeContext cannot — a missing writeReadyHook_ that
// keeps the handler off the writeReadyList, or an unlinked event subscription
// that would drop FlowControlResume.
// =============================================================================

namespace cp = channel_pipeline;

namespace {

HANDLER_TAG(buffer);

class BoundedWriteBufferHandlerPipelineTest : public ::testing::Test {
 protected:
  using PipelineHandler = BoundedWriteBufferHandler<cp::detail::ContextImpl>;

  cp::PipelineImpl::Ptr buildPipeline(
      std::unique_ptr<PipelineHandler> handler) {
    return cp::PipelineBuilder<
               cp::test::MockHeadHandler,
               cp::test::MockTailHandler,
               cp::test::TestAllocator,
               StreamEvent>()
        .setEventBase(&evb_)
        .setHead(&head_)
        .setTail(&tail_)
        .setAllocator(&allocator_)
        .addNextOutbound<PipelineHandler>(buffer_tag, std::move(handler))
        .build();
  }

  folly::EventBase evb_;
  cp::test::MockHeadHandler head_;
  cp::test::MockTailHandler tail_;
  cp::test::TestAllocator allocator_;
};

} // namespace

TEST_F(
    BoundedWriteBufferHandlerPipelineTest,
    CreditResumeEventDrainsHeldPayloads) {
  auto handler = std::make_unique<PipelineHandler>();

  std::vector<uint8_t> written;
  head_.setOnWriteCallback([&](cp::TypeErasedBox&& box) noexcept {
    auto m = box.take<ThriftStreamMessage>();
    if (m.payload.is<Payload>()) {
      if (auto& p = m.payload.get<Payload>(); p.data) {
        written.push_back(folly::io::Cursor(p.data.get()).read<uint8_t>());
      }
    }
    return cp::Result::Success;
  });

  auto pipeline = buildPipeline(std::move(handler));
  ASSERT_NE(pipeline, nullptr);

  // Credit starts paused: payloads are held, nothing reaches the head.
  (void)pipeline->fireWrite(cp::erase_and_box(makeItem(1)));
  (void)pipeline->fireWrite(cp::erase_and_box(makeItem(2)));
  EXPECT_TRUE(written.empty());

  // A FlowControlResume event drains them in order — proves the subscription is
  // linked through the real pipeline.
  pipeline->fireEvent(StreamEvent::FlowControlResume, cp::TypeErasedBox{});
  const std::vector<uint8_t> expected{1, 2};
  EXPECT_EQ(written, expected);
}

TEST_F(
    BoundedWriteBufferHandlerPipelineTest,
    RegistersOnWriteReadyListWhenTransportBackpressured) {
  auto handler = std::make_unique<PipelineHandler>();
  head_.setOnWriteCallback([](cp::TypeErasedBox&& box) noexcept {
    (void)box.take<ThriftStreamMessage>();
    return cp::Result::Backpressure;
  });

  auto pipeline = buildPipeline(std::move(handler));
  ASSERT_NE(pipeline, nullptr);
  pipeline->fireEvent(StreamEvent::FlowControlResume, cp::TypeErasedBox{});
  ASSERT_FALSE(pipeline->hasPendingWriteReady());

  (void)pipeline->fireWrite(cp::erase_and_box(makeItem(1)));

  EXPECT_TRUE(pipeline->hasPendingWriteReady())
      << "handler must be on the writeReadyList so onWriteReady fires";
}

TEST_F(BoundedWriteBufferHandlerPipelineTest, DrainsBufferOnWriteReady) {
  auto handler = std::make_unique<PipelineHandler>();

  std::vector<uint8_t> written;
  cp::Result headResult = cp::Result::Backpressure;
  head_.setOnWriteCallback([&](cp::TypeErasedBox&& box) noexcept {
    auto m = box.take<ThriftStreamMessage>();
    if (m.payload.is<Payload>()) {
      if (auto& p = m.payload.get<Payload>(); p.data) {
        written.push_back(folly::io::Cursor(p.data.get()).read<uint8_t>());
      }
    }
    return headResult;
  });

  auto pipeline = buildPipeline(std::move(handler));
  ASSERT_NE(pipeline, nullptr);
  pipeline->fireEvent(StreamEvent::FlowControlResume, cp::TypeErasedBox{});

  // Payload 1 is consumed by the head and arms transport backpressure; 2 and 3
  // are buffered.
  (void)pipeline->fireWrite(cp::erase_and_box(makeItem(1)));
  (void)pipeline->fireWrite(cp::erase_and_box(makeItem(2)));
  (void)pipeline->fireWrite(cp::erase_and_box(makeItem(3)));
  ASSERT_TRUE(pipeline->hasPendingWriteReady());

  // Head drains; onWriteReady empties the buffer in FIFO order.
  headResult = cp::Result::Success;
  pipeline->onWriteReady();

  const std::vector<uint8_t> expected{1, 2, 3};
  EXPECT_EQ(written, expected);
  EXPECT_FALSE(pipeline->hasPendingWriteReady());
}

} // namespace apache::thrift::fast_thrift::thrift::stream
