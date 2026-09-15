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

#include <thrift/lib/cpp2/fast_thrift/thrift/stream/handler/PayloadPrefetchHandler.h>

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Portability.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>

namespace apache::thrift::fast_thrift::thrift::stream {

namespace {

using channel_pipeline::erase_and_box;
using channel_pipeline::Result;
using channel_pipeline::TypeErasedBox;

// ContextApi stand-in recording both directions. The tests assert only on the
// handler's observable contract — the demand it relays to the producer
// (fireRead), the payloads/terminals it sends toward the wire (fireWrite), and
// the flow-control result — never on internal buffer state.
//
// `fireWrite` always takes the element, modeling the sink contract that a
// Backpressure result means the downstream consumed it.
class FakeContext {
 public:
  Result fireRead(TypeErasedBox&& msg) noexcept {
    auto m = msg.take<ThriftStreamMessage>();
    if (m.payload.is<RequestN>()) {
      const uint64_t n = m.payload.get<RequestN>().n;
      demand.push_back(n);
      // Synchronous producer hook: a producer that delivers reentrantly within
      // our own relayed RequestN fires here, before fireRead returns. Used to
      // exercise the reentrancy contract.
      if (onRequest) {
        onRequest(n);
      }
    } else if (m.payload.is<Cancel>()) {
      ++cancels;
    }
    return Result::Success;
  }

  Result fireWrite(TypeErasedBox&& msg) noexcept {
    auto m = msg.take<ThriftStreamMessage>();
    if (m.payload.is<Payload>()) {
      if (auto& p = m.payload.get<Payload>(); p.data) {
        sentTags.push_back(folly::io::Cursor(p.data.get()).read<uint8_t>());
      }
    } else if (m.payload.is<Complete>()) {
      ++completions;
      completionAfterNTags = sentTags.size();
    } else if (m.payload.is<Error>()) {
      ++errors;
      errorAfterNTags = sentTags.size();
    }
    return nextWriteResult;
  }

  // NOLINTNEXTLINE(clang-diagnostic-unused-member-function)
  void fireException(folly::exception_wrapper&& e) noexcept {
    exceptions.push_back(std::move(e));
  }

  void awaitWriteReady() noexcept {
    awaitingWriteReady = true;
    ++awaitCalls;
  }
  void cancelAwaitWriteReady() noexcept {
    awaitingWriteReady = false;
    ++cancelCalls;
  }

  Result nextWriteResult{Result::Success};
  std::function<void(uint64_t)> onRequest; // synchronous producer, see fireRead
  std::vector<uint64_t> demand; // RequestN relayed upstream to the producer
  size_t cancels{0};
  std::vector<uint8_t> sentTags; // payloads sent downstream, by tag
  size_t completions{0};
  size_t errors{0};
  // Payloads already sent when the completion went out — lets a test assert
  // completion is emitted after every payload.
  size_t completionAfterNTags{0};
  // Same, for the error terminal.
  size_t errorAfterNTags{0};
  std::vector<folly::exception_wrapper> exceptions;
  bool awaitingWriteReady{false};
  size_t awaitCalls{0};
  size_t cancelCalls{0};
};

using Handler = PayloadPrefetchHandler<FakeContext>;

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

ThriftStreamMessage makeError() {
  return ThriftStreamMessage{
      .payload = Error{
          .ex = folly::make_exception_wrapper<std::runtime_error>("boom")}};
}

ThriftStreamMessage makeCancel() {
  return ThriftStreamMessage{.payload = Cancel{}};
}

// Consumer grants credit: delivers a RequestN inbound.
void grant(Handler& handler, FakeContext& ctx, uint64_t n) {
  (void)handler.onRead(ctx, erase_and_box(makeRequestN(n)));
}

// Producer emits one payload: delivers it outbound.
Result produce(Handler& handler, FakeContext& ctx, uint8_t tag) {
  return handler.onWrite(ctx, erase_and_box(makeItem(tag)));
}

PayloadPrefetchConfig smallConfig() {
  return PayloadPrefetchConfig{.capacity = 8, .replenishThreshold = 4};
}

uint64_t total(const std::vector<uint64_t>& v) {
  uint64_t s = 0;
  for (auto n : v) {
    s += n;
  }
  return s;
}

bool strictlyAscending(const std::vector<uint8_t>& v) {
  for (size_t i = 1; i < v.size(); ++i) {
    if (v[i] <= v[i - 1]) {
      return false;
    }
  }
  return true;
}

} // namespace

// =============================================================================
// Config validation: the refill logic needs replenishThreshold < capacity to
// batch sensibly, so a violating config fails fast at construction in every
// build.
// =============================================================================

TEST(PayloadPrefetchHandlerDeathTest, ThresholdNotBelowCapacityIsRejected) {
  EXPECT_DEATH(
      Handler(PayloadPrefetchConfig{.capacity = 4, .replenishThreshold = 4}),
      "replenishThreshold_");
}

TEST(PayloadPrefetchHandlerDeathTest, ZeroCapacityIsRejected) {
  EXPECT_DEATH(
      Handler(PayloadPrefetchConfig{.capacity = 0, .replenishThreshold = 0}),
      "capacity_");
}

// =============================================================================
// Demand metering: a RequestN is consumed here and relayed to the producer
// bounded to free buffer space, so the reserve it pulls ahead can never exceed
// what the ring can hold. The consumer's demand is satisfied by draining the
// reserve, not by inflating the request.
// =============================================================================

TEST(PayloadPrefetchHandlerTest, FirstGrantRequestsBufferCapacity) {
  Handler handler{smallConfig()};
  FakeContext ctx;

  grant(handler, ctx, 1);

  // The peer's RequestN(1) is not forwarded verbatim. The relayed request is
  // bounded to free buffer space (capacity 8, nothing held or in flight), never
  // the demand plus capacity: the request can never outrun the ring.
  const std::vector<uint64_t> relayed{8};
  EXPECT_EQ(ctx.demand, relayed);
}

TEST(PayloadPrefetchHandlerTest, WritePathRerequestsAsReserveDrains) {
  Handler handler{
      PayloadPrefetchConfig{.capacity = 4, .replenishThreshold = 2}};
  FakeContext ctx;

  // Grant plenty of credit so every produced payload flows straight through,
  // draining the reserve as the (asynchronous) producer delivers.
  grant(handler, ctx, 100);
  for (uint8_t tag = 1; tag <= 10; ++tag) {
    ASSERT_EQ(produce(handler, ctx, tag), Result::Success);
  }

  // The write path re-requests as the reserve drains — an asynchronous producer
  // delivering outside our request stack must keep being driven, or a standing
  // demand larger than the ring would stall after one ring's worth. Each
  // request is still bounded, so total demand never runs more than a capacity
  // ahead of what has been sent.
  EXPECT_GT(ctx.demand.size(), 1u) << "write path relayed follow-up requests";
  EXPECT_LE(total(ctx.demand), ctx.sentTags.size() + 4);
  const std::vector<uint8_t> sent{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  EXPECT_EQ(ctx.sentTags, sent);
}

TEST(PayloadPrefetchHandlerTest, LaterGrantTopsUpReserveBelowThreshold) {
  Handler handler{
      PayloadPrefetchConfig{.capacity = 4, .replenishThreshold = 2}};
  FakeContext ctx;

  // Fill the prefetch reserve: grant 4, produce 4 (all sent), then produce 4
  // more with no credit (all held). The reserve is now a full ring. The initial
  // grant relays capacity (4); as the producer delivers and the reserve drains
  // below threshold mid-fill, the write path relays one more (3).
  grant(handler, ctx, 4);
  for (uint8_t tag = 1; tag <= 8; ++tag) {
    (void)produce(handler, ctx, tag);
  }
  const std::vector<uint64_t> afterFill{4, 3};
  ASSERT_EQ(ctx.demand, afterFill);

  // Small grants drain the held reserve one at a time. The reserve counts both
  // held elements and in-flight requests; no top-up fires until it falls below
  // the threshold (2), then a single request refills free space back to
  // capacity.
  grant(handler, ctx, 1); // reserve 3->3: no request
  grant(handler, ctx, 1); // reserve 3->2: no request
  grant(handler, ctx, 1); // reserve 2->1: below threshold, request free space 3
  const std::vector<uint64_t> afterDrain{4, 3, 3};
  EXPECT_EQ(ctx.demand, afterDrain);
  const std::vector<uint8_t> sent{1, 2, 3, 4, 5, 6, 7};
  EXPECT_EQ(ctx.sentTags, sent);
}

// =============================================================================
// Capacity guarantee: the handler never requests more from the producer than
// its buffer can hold ahead of delivery. The relayed request is bounded to free
// buffer space (capacity - held - in-flight), so a well-behaved producer that
// delivers exactly what it was asked for can never overflow the ring — even
// when it delivers reentrantly within our own RequestN.
// =============================================================================

TEST(
    PayloadPrefetchHandlerTest,
    RequestNeverExceedsBufferCapacityAheadOfDelivery) {
  Handler handler{smallConfig()}; // capacity 8
  FakeContext ctx;

  // Enormous consumer demand, nothing produced yet: the request must not run
  // more than a buffer's worth ahead of what has been delivered. A design that
  // relayed demand-plus-capacity would fire ~1008 here and overflow the ring
  // once the producer honored it.
  grant(handler, ctx, 1000);
  EXPECT_LE(total(ctx.demand), ctx.sentTags.size() + 8);
  const std::vector<uint64_t> relayed{8};
  EXPECT_EQ(ctx.demand, relayed);
}

TEST(PayloadPrefetchHandlerTest, ReentrantSynchronousProducerNeverOverflows) {
  Handler handler{
      PayloadPrefetchConfig{.capacity = 4, .replenishThreshold = 2}};
  FakeContext ctx;

  size_t overflowErrors = 0;
  uint8_t nextTag = 1;
  // A synchronous producer that delivers exactly what it was asked for, inline
  // within our relayed RequestN (the reentrant case the write path is designed
  // to tolerate). Because requests are bounded to free space, this well-behaved
  // producer can never fill the ring past capacity.
  ctx.onRequest = [&](uint64_t n) {
    for (uint64_t i = 0; i < n; ++i) {
      if (produce(handler, ctx, nextTag++) == Result::Error) {
        ++overflowErrors;
      }
    }
  };

  for (int i = 0; i < 50; ++i) {
    grant(handler, ctx, 1);
    ASSERT_EQ(overflowErrors, 0u) << "reentrant delivery overflowed the ring";
    // The buffer never runs more than a capacity ahead of delivery.
    ASSERT_LE(total(ctx.demand), ctx.sentTags.size() + 4);
  }
  // Reentrant delivery preserves FIFO order end to end.
  EXPECT_TRUE(strictlyAscending(ctx.sentTags));
}

TEST(
    PayloadPrefetchHandlerTest,
    ReentrantDeliveryUnderBackpressureNeverOverflows) {
  Handler handler{
      PayloadPrefetchConfig{.capacity = 4, .replenishThreshold = 2}};
  FakeContext ctx;
  // Grant one credit so the first payload is sent-through, then congest the
  // transport: subsequent reentrant deliveries must land in the ring, and the
  // bounded request guarantees they fit.
  size_t overflowErrors = 0;
  uint8_t nextTag = 1;
  ctx.onRequest = [&](uint64_t n) {
    for (uint64_t i = 0; i < n; ++i) {
      if (produce(handler, ctx, nextTag++) == Result::Error) {
        ++overflowErrors;
      }
    }
  };

  ctx.nextWriteResult = Result::Backpressure;
  grant(handler, ctx, 1);

  // The reentrant producer honored the request (capacity 4) into a congested
  // transport; nothing overflowed because the request equaled free space.
  EXPECT_EQ(overflowErrors, 0u);
  EXPECT_LE(total(ctx.demand), ctx.sentTags.size() + 4);
}

// =============================================================================
// Demand larger than a ring's worth: a single relay is capped at capacity, so
// the handler must keep re-requesting until the standing demand is met. The
// refill loop handles a synchronous producer within one grant; the write path
// handles an asynchronous producer across later deliveries. Neither overflows.
// =============================================================================

TEST(PayloadPrefetchHandlerTest, SyncProducerLargeDemandServedWithinGrant) {
  Handler handler{
      PayloadPrefetchConfig{.capacity = 4, .replenishThreshold = 2}};
  FakeContext ctx;

  size_t overflowErrors = 0;
  uint8_t nextTag = 1;
  ctx.onRequest = [&](uint64_t n) {
    for (uint64_t i = 0; i < n; ++i) {
      if (produce(handler, ctx, nextTag++) == Result::Error) {
        ++overflowErrors;
      }
    }
  };

  // One grant far larger than the ring. A synchronous producer delivers inside
  // each relay, freeing space so the refill loop pulls the next capacity-sized
  // chunk — the whole standing demand of 20 is served within this single grant,
  // never overflowing the ring, and the loop terminates.
  grant(handler, ctx, 20);

  EXPECT_EQ(overflowErrors, 0u);
  EXPECT_EQ(ctx.sentTags.size(), 20u);
  EXPECT_TRUE(strictlyAscending(ctx.sentTags));
  EXPECT_LE(total(ctx.demand), ctx.sentTags.size() + 4);
}

TEST(PayloadPrefetchHandlerTest, AsyncProducerLargeDemandServedAcrossWrites) {
  Handler handler{
      PayloadPrefetchConfig{.capacity = 4, .replenishThreshold = 2}};
  FakeContext ctx;

  // Standing demand far larger than the ring, then an asynchronous producer
  // that delivers one payload per separate onWrite stack. The read-path grant
  // can only pull one ring's worth; the write path must re-request as each
  // delivery drains the reserve, or the stream stalls after 4.
  grant(handler, ctx, 20);

  uint8_t nextTag = 1;
  while (ctx.sentTags.size() < 20) {
    ASSERT_LT(nextTag, 100) << "delivery must converge, not stall";
    ASSERT_EQ(produce(handler, ctx, nextTag++), Result::Success);
    // Capacity invariant holds at every step.
    ASSERT_LE(total(ctx.demand), ctx.sentTags.size() + 4);
  }

  EXPECT_EQ(ctx.sentTags.size(), 20u);
  EXPECT_TRUE(strictlyAscending(ctx.sentTags));
  EXPECT_GT(ctx.demand.size(), 1u) << "write path re-requested to serve demand";
}

// =============================================================================
// Send-credit gate: payloads are held in memory until the consumer grants
// credit to send them, independent of how many are prefetched.
// =============================================================================

TEST(PayloadPrefetchHandlerTest, SendsOnlyUpToGrantedCredit) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 2);

  (void)produce(handler, ctx, 1);
  (void)produce(handler, ctx, 2);
  (void)produce(handler, ctx, 3);

  // Only the two credited payloads go out; the third is held as prefetch.
  const std::vector<uint8_t> sent{1, 2};
  EXPECT_EQ(ctx.sentTags, sent);
}

TEST(PayloadPrefetchHandlerTest, MoreCreditDrainsHeldPayloadsInOrder) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 2);

  for (uint8_t tag = 1; tag <= 5; ++tag) {
    (void)produce(handler, ctx, tag);
  }
  const std::vector<uint8_t> beforeMore{1, 2};
  ASSERT_EQ(ctx.sentTags, beforeMore);

  // A later grant releases the held payloads in FIFO order.
  grant(handler, ctx, 3);
  const std::vector<uint8_t> afterMore{1, 2, 3, 4, 5};
  EXPECT_EQ(ctx.sentTags, afterMore);
}

// =============================================================================
// Transport backpressure absorption (write path): because demand is bounded to
// free buffer space, the handler can absorb transport congestion into the ring
// and answer Success rather than surfacing Backpressure to the producer. The
// producer is throttled through the demand channel (a full ring stops
// requests), not through a flow-control result. Congestion is a `Backpressure`
// return from the sink, which means it took this element but wants no more.
// =============================================================================

TEST(PayloadPrefetchHandlerTest, WriteBackpressureAbsorbedThenResumed) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 5);
  ctx.nextWriteResult = Result::Backpressure;

  // The first payload probes the transport and is taken; that congestion signal
  // arms write-ready. Every later payload is absorbed into the ring and
  // answered Success — the handler is the shock absorber, not a pass-through
  // that dumps onto a congested transport.
  EXPECT_EQ(produce(handler, ctx, 1), Result::Success);
  EXPECT_TRUE(ctx.awaitingWriteReady);
  EXPECT_EQ(produce(handler, ctx, 2), Result::Success);
  EXPECT_EQ(produce(handler, ctx, 3), Result::Success);
  const std::vector<uint8_t> probed{1};
  EXPECT_EQ(ctx.sentTags, probed)
      << "only the probe reached the congested transport";

  // Transport drains: the absorbed payloads flow out in FIFO order.
  ctx.nextWriteResult = Result::Success;
  handler.onWriteReady(ctx);
  const std::vector<uint8_t> drained{1, 2, 3};
  EXPECT_EQ(ctx.sentTags, drained);
  EXPECT_FALSE(ctx.awaitingWriteReady);
}

TEST(PayloadPrefetchHandlerTest, CongestionThrottlesProducerViaDemand) {
  Handler handler{
      PayloadPrefetchConfig{.capacity = 4, .replenishThreshold = 2}};
  FakeContext ctx;
  ctx.nextWriteResult = Result::Backpressure; // congested throughout
  // Ample credit, so transport congestion — not send-credit — is the gate.
  grant(handler, ctx, 100);

  // An asynchronous producer that honors its grant: it delivers exactly what
  // has been requested so far, no more. Under congestion the handler absorbs
  // into the ring; the request bound stops granting once the reserve is
  // stocked, so a producer respecting its grant simply runs out of things to
  // deliver — throttled with no overflow and no Error surfaced to it.
  size_t overflowErrors = 0;
  uint8_t nextTag = 1;
  while (nextTag <= total(ctx.demand)) {
    if (produce(handler, ctx, nextTag++) == Result::Error) {
      ++overflowErrors;
    }
  }

  EXPECT_EQ(overflowErrors, 0u) << "well-behaved producer never overflows";
  EXPECT_TRUE(ctx.awaitingWriteReady) << "transport congestion detected";
  const uint64_t grantedWhileCongested = total(ctx.demand);
  EXPECT_EQ(uint64_t{nextTag} - 1, grantedWhileCongested)
      << "producer delivered exactly its grant, then stalled";

  // Transport drains: absorbed payloads flow out in order and demand resumes.
  ctx.nextWriteResult = Result::Success;
  handler.onWriteReady(ctx);
  EXPECT_TRUE(strictlyAscending(ctx.sentTags));
  EXPECT_GT(total(ctx.demand), grantedWhileCongested)
      << "draining freed space, so the producer is granted more";
}

// =============================================================================
// Credit-drain pause (read path): when a grant drains the prefetch reserve and
// the transport backpressures with elements still queued, the drain stops and
// resumes on onWriteReady rather than dumping the backlog into a congested
// transport.
// =============================================================================

TEST(PayloadPrefetchHandlerTest, CreditDrainPausesOnBackpressureThenResumes) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 1);

  // Build a held reserve: one payload sent on the single credit, the rest held
  // for want of credit.
  for (uint8_t tag = 1; tag <= 4; ++tag) {
    (void)produce(handler, ctx, tag);
  }
  const std::vector<uint8_t> beforeDrain{1};
  ASSERT_EQ(ctx.sentTags, beforeDrain);

  // A grant drains the reserve, but the transport backpressures after the first
  // held payload — the drain stops with the rest queued and arms write-ready.
  ctx.nextWriteResult = Result::Backpressure;
  grant(handler, ctx, 5);
  const std::vector<uint8_t> paused{1, 2};
  EXPECT_EQ(ctx.sentTags, paused);
  EXPECT_TRUE(ctx.awaitingWriteReady);

  // When the transport drains, onWriteReady resumes into the banked credit.
  ctx.nextWriteResult = Result::Success;
  handler.onWriteReady(ctx);
  const std::vector<uint8_t> afterDrain{1, 2, 3, 4};
  EXPECT_EQ(ctx.sentTags, afterDrain);
  EXPECT_FALSE(ctx.awaitingWriteReady);
}

TEST(PayloadPrefetchHandlerTest, PrefetchHoldWithOpenTransportReturnsSuccess) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 1);

  (void)produce(handler, ctx, 1); // sent, credit now exhausted
  // A payload held as prefetch (no demand) with the transport open is a normal
  // hold, not congestion: Success, so the producer keeps filling the reserve.
  EXPECT_EQ(produce(handler, ctx, 2), Result::Success);
  EXPECT_FALSE(ctx.awaitingWriteReady);
}

TEST(PayloadPrefetchHandlerTest, NoWriteReadyChurnOnSteadySendPath) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 100);

  // Credit available and transport accepting: producing payloads must not touch
  // the write-ready registration at all — it is armed only when a read-path
  // drain pauses, so the steady-state path stays free of that out-of-line call.
  for (uint8_t tag = 1; tag <= 5; ++tag) {
    (void)produce(handler, ctx, tag);
  }
  EXPECT_EQ(ctx.awaitCalls, 0u) << "never registered without backpressure";
  EXPECT_EQ(ctx.cancelCalls, 0u) << "no per-payload cancel churn";
}

// =============================================================================
// Terminals (Complete/Error): per Reactive Streams a terminal is ordered after
// every payload but is NOT gated by send-credit — it is delivered regardless of
// outstanding demand, needing no credit of its own. It is still subject to
// ordering (behind queued payloads) and transport readiness.
// =============================================================================

TEST(PayloadPrefetchHandlerTest, CompletionDeliveredWithoutCredit) {
  Handler handler{smallConfig()};
  FakeContext ctx;

  // No credit has been granted and nothing is queued: the completion still goes
  // straight out. A terminal does not require a send-credit.
  EXPECT_EQ(
      handler.onWrite(ctx, erase_and_box(makeComplete())), Result::Success);
  EXPECT_EQ(ctx.completions, 1u);
}

TEST(
    PayloadPrefetchHandlerTest,
    CompletionDeliveredAfterPayloadsWithoutExtraCredit) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 1);

  // One payload flows on the single credit, one is held (credit exhausted),
  // then the completion arrives and is held behind the queued payload.
  (void)produce(handler, ctx, 1);
  (void)produce(handler, ctx, 2);
  (void)handler.onWrite(ctx, erase_and_box(makeComplete()));
  EXPECT_EQ(ctx.completions, 0u) << "completion held behind the queued payload";

  // A single grant drains the held payload; the completion follows immediately
  // in the same turn — it needs no credit of its own, only the payload ahead of
  // it to be gone.
  grant(handler, ctx, 1);
  const std::vector<uint8_t> sent{1, 2};
  EXPECT_EQ(ctx.sentTags, sent);
  EXPECT_EQ(ctx.completions, 1u) << "terminal is not credit-gated";
  EXPECT_EQ(ctx.completionAfterNTags, 2u)
      << "completion is emitted after every payload";
}

TEST(
    PayloadPrefetchHandlerTest, ErrorDeliveredAfterPayloadsWithoutExtraCredit) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 1);

  // One payload flows, one is held, then the error arrives behind it.
  (void)produce(handler, ctx, 1);
  (void)produce(handler, ctx, 2);
  (void)handler.onWrite(ctx, erase_and_box(makeError()));
  EXPECT_EQ(ctx.errors, 0u) << "error held behind the queued payload";

  // Draining the held payload releases the error in the same turn — no extra
  // credit required.
  grant(handler, ctx, 1);
  const std::vector<uint8_t> sent{1, 2};
  EXPECT_EQ(ctx.sentTags, sent);
  EXPECT_EQ(ctx.errors, 1u) << "terminal is not credit-gated";
  EXPECT_EQ(ctx.errorAfterNTags, 2u)
      << "error is emitted after every payload the producer handed over";
}

TEST(
    PayloadPrefetchHandlerTest, TerminalWaitsForCongestedTransportThenFlushes) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 1);
  ctx.nextWriteResult = Result::Backpressure;

  // One payload probes the congested transport (taken, arms write-ready). The
  // completion then arrives with an empty ring but a congested transport: it is
  // ordered fine, but must wait for the transport, not for credit.
  (void)produce(handler, ctx, 1);
  ASSERT_TRUE(ctx.awaitingWriteReady);
  (void)handler.onWrite(ctx, erase_and_box(makeComplete()));
  EXPECT_EQ(ctx.completions, 0u) << "held for the congested transport";

  // Transport drains: the completion flushes even though no credit was granted
  // for it.
  ctx.nextWriteResult = Result::Success;
  handler.onWriteReady(ctx);
  EXPECT_EQ(ctx.completions, 1u);
}

// =============================================================================
// Cancel: we can send nothing more once cancelled. It is forwarded to the
// producer, and held/late output is not delivered.
// =============================================================================

TEST(PayloadPrefetchHandlerTest, CancelStopsSendingAndForwards) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 1);

  (void)produce(handler, ctx, 1); // sent
  (void)produce(handler, ctx, 2); // held (credit exhausted)
  (void)produce(handler, ctx, 3); // held

  EXPECT_EQ(handler.onRead(ctx, erase_and_box(makeCancel())), Result::Success);
  EXPECT_EQ(ctx.cancels, 1u) << "Cancel forwarded to the producer";

  // Once cancelled we send nothing more: a later grant does not drain the held
  // payloads, and late producer output is dropped.
  grant(handler, ctx, 5);
  EXPECT_EQ(produce(handler, ctx, 4), Result::Success);
  const std::vector<uint8_t> sent{1};
  EXPECT_EQ(ctx.sentTags, sent) << "nothing is sent after a cancel";
}

TEST(PayloadPrefetchHandlerTest, DemandAfterCancelIsForwardedToProducer) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 1);
  (void)handler.onRead(ctx, erase_and_box(makeCancel()));
  ASSERT_EQ(ctx.cancels, 1u);

  // Once cancelled the handler can act on nothing; a later frame is forwarded
  // to the producer untouched rather than swallowed.
  std::vector<uint64_t> expected = ctx.demand;
  expected.push_back(5);
  EXPECT_EQ(
      handler.onRead(ctx, erase_and_box(makeRequestN(5))), Result::Success);
  EXPECT_EQ(ctx.demand, expected) << "post-cancel frames pass straight through";
}

// =============================================================================
// Lifecycle
// =============================================================================

TEST(PayloadPrefetchHandlerTest, HeldPayloadsSurvivePipelineInactive) {
  Handler handler{smallConfig()};
  FakeContext ctx;
  grant(handler, ctx, 1);

  (void)produce(handler, ctx, 1); // sent
  (void)produce(handler, ctx, 2); // held
  (void)produce(handler, ctx, 3); // held

  handler.onPipelineInactive(ctx);

  // Held payloads are unsent output; the pipeline going inactive must not drop
  // them.
  grant(handler, ctx, 2);
  const std::vector<uint8_t> sent{1, 2, 3};
  EXPECT_EQ(ctx.sentTags, sent);
}

} // namespace apache::thrift::fast_thrift::thrift::stream
