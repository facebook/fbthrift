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

#include <thrift/lib/cpp2/fast_thrift/thrift/stream/handler/InboundCreditHandler.h>

#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Portability.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/StreamEvents.h>

namespace apache::thrift::fast_thrift::thrift::stream {

namespace {

using channel_pipeline::erase_and_box;
using channel_pipeline::Result;
using channel_pipeline::TypeErasedBox;

// Minimal ContextApi stand-in. The tests assert only on the handler's
// observable contract — what it forwards, the flow-control result, and the
// flow-control events it fires — never on internal credit state.
class FakeContext {
 public:
  Result fireRead(TypeErasedBox&& msg) noexcept {
    reads.push_back(std::move(msg));
    return Result::Success;
  }

  Result fireWrite(TypeErasedBox&& msg) noexcept {
    writes.push_back(std::move(msg));
    return nextWriteResult;
  }

  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  void fireException(folly::exception_wrapper&& e) noexcept {
    exceptions.push_back(std::move(e));
  }

  void fireEvent(StreamEvent ev, TypeErasedBox&& /*msg*/) noexcept {
    firedEvents.push_back(ev);
  }

  Result nextWriteResult{Result::Success};
  std::vector<TypeErasedBox> reads;
  std::vector<TypeErasedBox> writes;
  std::vector<folly::exception_wrapper> exceptions;
  std::vector<StreamEvent> firedEvents;
};

ThriftStreamMessage makeRequestN(uint64_t n) {
  return ThriftStreamMessage{.payload = RequestN{.n = n}};
}

ThriftStreamMessage makeItem() {
  return ThriftStreamMessage{.payload = Payload{.data = nullptr}};
}

} // namespace

// =============================================================================
// The credit contract: a producer may emit an element only while the peer's
// granted credit remains. The element that spends the last credit is delivered
// but returns Backpressure so the producer pauses immediately; an element sent
// while exhausted is beyond demand and is dropped with Error. These tests
// observe that contract through forwarding + flow-control results, not through
// internal credit state.
// =============================================================================

TEST(InboundCreditHandlerTest, WriteWhileExhaustedIsAProtocolViolation) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  // A Payload written with no credit overruns the peer's demand. An upstream
  // buffer honoring FlowControlPause keeps this unreachable, so it is a bug: it
  // fatals in debug. In release the handler still enforces the hard contract —
  // the element is dropped with Error (tearing down the peer), not delivered.
  if (folly::kIsDebug) {
    EXPECT_DEATH(
        (void)handler.onWrite(ctx, erase_and_box(makeItem())),
        "credit is exhausted");
  } else {
    EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Error);
    EXPECT_TRUE(ctx.writes.empty()) << "no credit -> element dropped, not sent";
  }
}

TEST(InboundCreditHandlerTest, LastCreditIsDeliveredButBackpressures) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1)));

  // The sole credit is spent: the element is forwarded, but demand is now
  // exhausted so the producer is paused via Backpressure.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeItem())), Result::Backpressure);
  EXPECT_EQ(ctx.writes.size(), 1u) << "the exhausting element still goes out";
}

TEST(InboundCreditHandlerTest, CreditIsSpentPerItem) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(2)));

  // Two elements are authorized: the first flows freely, the second exhausts
  // demand (delivered + Backpressure). Writing beyond demand is a protocol
  // violation covered by WriteWhileExhaustedIsAProtocolViolation.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Success);
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeItem())), Result::Backpressure);

  EXPECT_EQ(ctx.writes.size(), 2u) << "only the two authorized elements go out";
}

TEST(InboundCreditHandlerTest, CreditAccumulatesAcrossGrants) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(2)));
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(3)));

  // 2 + 3 grants authorize five elements; the first four flow freely.
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Success)
        << "element " << i;
  }
  // The fifth exhausts demand (delivered + Backpressure). Writing beyond demand
  // is a protocol violation covered by WriteWhileExhaustedIsAProtocolViolation.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeItem())), Result::Backpressure);
  EXPECT_EQ(ctx.writes.size(), 5u);
}

TEST(InboundCreditHandlerTest, CreditGrantSaturatesInsteadOfOverflowing) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  // Grant the maximum budget, then grant more. A naive add would wrap the
  // budget back to zero and start dropping authorized elements; the grant must
  // saturate so outstanding demand still stands.
  (void)handler.onRead(
      ctx, erase_and_box(makeRequestN(std::numeric_limits<uint64_t>::max())));
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1)));

  // Credit did not wrap to zero: the next element is still authorized and sent.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Success);
  EXPECT_EQ(ctx.writes.size(), 1u);
}

TEST(InboundCreditHandlerTest, DownstreamStatusIsNotMaskedByExhaustion) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1)));

  // The last credit is spent, but downstream fails: its status propagates
  // unchanged rather than being overridden by the exhaustion Backpressure.
  ctx.nextWriteResult = Result::Error;
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Error);
}

TEST(InboundCreditHandlerTest, RequestNIsConsumedNotForwarded) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1)));

  EXPECT_TRUE(ctx.reads.empty())
      << "REQUEST_N is the grant itself, consumed here, not forwarded";
}

TEST(InboundCreditHandlerTest, InboundItemPassesThrough) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  // A consumer-side inbound item is not this handler's frame — pass through.
  EXPECT_EQ(handler.onRead(ctx, erase_and_box(makeItem())), Result::Success);
  EXPECT_EQ(ctx.reads.size(), 1u);
}

TEST(InboundCreditHandlerTest, OutboundRequestNPassesThroughUngated) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  // A consumer granting credit outbound is not gated by (its own) credit.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeRequestN(5))), Result::Success);
  EXPECT_EQ(ctx.writes.size(), 1u);
}

TEST(InboundCreditHandlerTest, CreditSurvivesPipelineInactive) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1)));

  handler.onPipelineInactive(ctx);

  // Granted credit is the peer's outstanding demand; a transport pause does not
  // discard it, so the write still goes out (the sole credit exhausts, so the
  // result is Backpressure).
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeItem())), Result::Backpressure);
  EXPECT_EQ(ctx.writes.size(), 1u);
}

// =============================================================================
// Flow-control events: credit stays private, so the handler publishes only
// readiness. It fires FlowControlPause as the last credit is spent and
// FlowControlResume when credit becomes available after exhaustion.
// =============================================================================

TEST(InboundCreditHandlerTest, GrantingCreditAfterExhaustionFiresResume) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  // A fresh handler holds no credit (exhausted); the first grant unblocks it.
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1)));

  const std::vector<StreamEvent> expected{StreamEvent::FlowControlResume};
  EXPECT_EQ(ctx.firedEvents, expected);
}

TEST(
    InboundCreditHandlerTest, GrantingCreditWhenNotExhaustedDoesNotFireResume) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  (void)handler.onRead(ctx, erase_and_box(makeRequestN(2))); // 0 -> 2: resume
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(3))); // 2 -> 5: no event

  const std::vector<StreamEvent> expected{StreamEvent::FlowControlResume};
  EXPECT_EQ(ctx.firedEvents, expected)
      << "no writer is paused while credit remains, so no resume is published";
}

TEST(InboundCreditHandlerTest, ConsumingLastCreditFiresPause) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1))); // resume
  ctx.firedEvents.clear();

  // Spending the sole credit exhausts demand: publish the pause.
  (void)handler.onWrite(ctx, erase_and_box(makeItem()));

  const std::vector<StreamEvent> expected{StreamEvent::FlowControlPause};
  EXPECT_EQ(ctx.firedEvents, expected);
}

TEST(InboundCreditHandlerTest, ConsumingNonLastCreditDoesNotFirePause) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(2))); // resume
  ctx.firedEvents.clear();

  // Two credits: the first spend leaves demand, so no pause is published.
  (void)handler.onWrite(ctx, erase_and_box(makeItem()));

  EXPECT_TRUE(ctx.firedEvents.empty()) << "demand remains, so no pause";
}

TEST(InboundCreditHandlerTest, ReExhaustAndRegrantRepublishReadiness) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1))); // resume
  (void)handler.onWrite(ctx, erase_and_box(makeItem())); // spend -> pause
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1))); // resume again

  const std::vector<StreamEvent> expected{
      StreamEvent::FlowControlResume,
      StreamEvent::FlowControlPause,
      StreamEvent::FlowControlResume};
  EXPECT_EQ(ctx.firedEvents, expected);
}

} // namespace apache::thrift::fast_thrift::thrift::stream
