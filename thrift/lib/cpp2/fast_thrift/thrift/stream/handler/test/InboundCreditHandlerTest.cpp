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

namespace apache::thrift::fast_thrift::thrift::stream {

namespace {

using channel_pipeline::erase_and_box;
using channel_pipeline::Result;
using channel_pipeline::TypeErasedBox;

// Minimal ContextApi stand-in. The tests assert only on the handler's
// observable contract — what it forwards each direction and the flow-control
// result — never on internal credit state.
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

  Result nextWriteResult{Result::Success};
  std::vector<TypeErasedBox> reads;
  std::vector<TypeErasedBox> writes;
  std::vector<folly::exception_wrapper> exceptions;
};

ThriftStreamMessage makeRequestN(uint64_t n) {
  return ThriftStreamMessage{.payload = RequestN{.n = n}};
}

ThriftStreamMessage makeItem() {
  return ThriftStreamMessage{.payload = Payload{.data = nullptr}};
}

ThriftStreamMessage makeCancel() {
  return ThriftStreamMessage{.payload = Cancel{}};
}

} // namespace

// =============================================================================
// The credit contract: a producer may emit an element only while the peer's
// granted credit remains. The element that spends the last credit is delivered;
// an element sent while exhausted is beyond demand and is dropped with Error.
// These tests observe that contract through forwarding + flow-control results,
// not through internal credit state.
// =============================================================================

TEST(InboundCreditHandlerTest, WriteWhileExhaustedIsAProtocolViolation) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  // A Payload written with no credit overruns the peer's demand. An upstream
  // buffer/producer honoring its granted demand keeps this unreachable, so it
  // is a bug: it fatals in debug. In release the handler still enforces the
  // hard contract — the element is dropped with Error (tearing down the peer).
  if (folly::kIsDebug) {
    EXPECT_DEATH(
        (void)handler.onWrite(ctx, erase_and_box(makeItem())),
        "credit is exhausted");
  } else {
    EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Error);
    EXPECT_TRUE(ctx.writes.empty()) << "no credit -> element dropped, not sent";
  }
}

TEST(InboundCreditHandlerTest, LastCreditIsDelivered) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1)));

  // The sole credit is spent: the element is forwarded and reports success.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Success);
  EXPECT_EQ(ctx.writes.size(), 1u);
}

TEST(InboundCreditHandlerTest, CreditIsSpentPerItem) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(2)));

  // Two elements are authorized: both flow. Writing beyond demand is a protocol
  // violation covered by WriteWhileExhaustedIsAProtocolViolation.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Success);
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Success);

  EXPECT_EQ(ctx.writes.size(), 2u) << "only the two authorized elements go out";
}

TEST(InboundCreditHandlerTest, CreditAccumulatesAcrossGrants) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(2)));
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(3)));

  // 2 + 3 grants authorize five elements; all flow.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Success)
        << "element " << i;
  }
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

TEST(InboundCreditHandlerTest, OverflowGrantForwardsOnlyTheAcceptedDelta) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  // Fill the budget to one below the maximum, then grant far more than the
  // remaining headroom. The budget saturates, so only the 1 unit of headroom it
  // actually admitted may travel onward as demand — forwarding the raw ask
  // would authorize the source beyond what the budget will let through, and
  // those excess elements would later trip the exhaustion-violation path.
  (void)handler.onRead(
      ctx,
      erase_and_box(makeRequestN(std::numeric_limits<uint64_t>::max() - 1)));
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(10)));

  ASSERT_EQ(ctx.reads.size(), 2u);
  EXPECT_EQ(
      ctx.reads[0].take<ThriftStreamMessage>().payload.get<RequestN>().n,
      std::numeric_limits<uint64_t>::max() - 1)
      << "the first grant fit, so it is forwarded unchanged";
  EXPECT_EQ(
      ctx.reads[1].take<ThriftStreamMessage>().payload.get<RequestN>().n, 1u)
      << "only the admitted headroom is forwarded once the budget saturates";
}

TEST(InboundCreditHandlerTest, DownstreamStatusPropagatesUnchanged) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(1)));

  // Credit is available, but downstream fails: its status propagates unchanged.
  ctx.nextWriteResult = Result::Error;
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Error);
}

// =============================================================================
// Demand forwarding: RequestN is accounted AND forwarded on so the source can
// produce in response. Other inbound frames pass through untouched.
// =============================================================================

TEST(InboundCreditHandlerTest, RequestNIsAccountedAndForwardedAsDemand) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  (void)handler.onRead(ctx, erase_and_box(makeRequestN(3)));

  // Forwarded on as demand...
  ASSERT_EQ(ctx.reads.size(), 1u);
  auto forwarded = ctx.reads[0].take<ThriftStreamMessage>();
  ASSERT_TRUE(forwarded.payload.is<RequestN>());
  EXPECT_EQ(forwarded.payload.get<RequestN>().n, 3u);

  // ...and still accounted, so writes up to the grant are authorized.
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Success);
  }
}

TEST(InboundCreditHandlerTest, InboundItemPassesThrough) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  // A consumer-side inbound item is not this handler's frame — pass through.
  EXPECT_EQ(handler.onRead(ctx, erase_and_box(makeItem())), Result::Success);
  EXPECT_EQ(ctx.reads.size(), 1u);
}

TEST(InboundCreditHandlerTest, CancelPassesThroughInbound) {
  InboundCreditHandler<FakeContext> handler;
  FakeContext ctx;

  // Cancel is a teardown frame for the source, not credit — pass through.
  EXPECT_EQ(handler.onRead(ctx, erase_and_box(makeCancel())), Result::Success);
  ASSERT_EQ(ctx.reads.size(), 1u);
  EXPECT_TRUE(ctx.reads[0].take<ThriftStreamMessage>().payload.is<Cancel>());
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
  // discard it, so the write still goes out.
  EXPECT_EQ(handler.onWrite(ctx, erase_and_box(makeItem())), Result::Success);
  EXPECT_EQ(ctx.writes.size(), 1u);
}

} // namespace apache::thrift::fast_thrift::thrift::stream
