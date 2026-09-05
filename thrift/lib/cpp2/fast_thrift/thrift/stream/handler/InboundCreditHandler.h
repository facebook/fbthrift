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

#pragma once

#include <cstdint>
#include <limits>
#include <utility>

#include <glog/logging.h>
#include <folly/CPortability.h>
#include <folly/ExceptionWrapper.h>
#include <folly/Likely.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Handler.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/ContextImpl.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/StreamEvents.h>

namespace apache::thrift::fast_thrift::thrift::stream {

/**
 * InboundCreditHandler — the receiving end of the RSocket credit contract on an
 * established stream / sink / bidi exchange. The peer grants credit via
 * REQUEST_N, where one credit authorizes exactly one stream element (a
 * `Payload`); a producer may emit an element only while it holds credit. This
 * handler owns the credit budget *privately* — the count never leaves the
 * handler. It publishes only readiness, as StreamEvent flow-control events:
 *
 *   - Inbound `RequestN` adds to the credit and is consumed — it is the grant
 *     itself, not data to deliver upward. When credit becomes available after
 *     being exhausted, it fires `StreamEvent::FlowControlResume` so a writer
 *     that paused upstream (e.g. a buffer) can resume. Other inbound frames
 *     pass through.
 *   - Outbound `Payload` spends one credit and is forwarded. The element that
 *     spends the *last* credit is still forwarded, but the handler fires
 *     `StreamEvent::FlowControlPause` and returns `Result::Backpressure` so the
 *     producer is paused the moment demand is exhausted rather than one element
 *     too late. A further element sent while exhausted is beyond the peer's
 *     demand — a credit-contract violation an upstream buffer honoring
 *     `FlowControlPause` keeps unreachable, so it fatals in debug and is
 * dropped with `Result::Error` in release. Non-`Payload` frames pass through
 * ungated.
 *
 * Side-agnostic: a server stream producer and a client sink producer both spend
 * the credit their peer grants. Scope is credit accounting only — buffering
 * refused elements is a separate concern owned by another handler.
 */
template <typename Context>
class InboundCreditHandler {
 public:
  // HandlerLifecycle
  void handlerAdded(Context& /*ctx*/) noexcept {}
  void handlerRemoved(Context& /*ctx*/) noexcept {}
  void onPipelineActive(Context& /*ctx*/) noexcept {}
  void onReadReady(Context& /*ctx*/) noexcept {}

  // InboundHandler
  channel_pipeline::Result onRead(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    auto& message = msg.get<ThriftStreamMessage>();
    if (!message.payload.is<RequestN>()) {
      return ctx.fireRead(std::move(msg));
    }
    const bool wasExhausted = credits_ == 0;
    credits_ = addSaturating(credits_, message.payload.get<RequestN>().n);
    if (FOLLY_UNLIKELY(wasExhausted && credits_ > 0)) {
      // Credit just became available: tell a writer that paused upstream while
      // exhausted that it may resume. Credit stays private — only readiness is
      // published.
      ctx.fireEvent(
          StreamEvent::FlowControlResume, channel_pipeline::TypeErasedBox{});
    }
    return channel_pipeline::Result::Success;
  }

  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }

  // OutboundHandler
  channel_pipeline::Result onWrite(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    auto& message = msg.get<ThriftStreamMessage>();
    if (!message.payload.is<Payload>()) {
      return ctx.fireWrite(std::move(msg));
    }
    if (FOLLY_UNLIKELY(credits_ == 0)) {
      return onCreditExhaustedViolation();
    }
    --credits_;
    const channel_pipeline::Result forwarded = ctx.fireWrite(std::move(msg));
    if (credits_ == 0) {
      // Demand just hit zero: publish the pause before returning, so a
      // subscriber observes it in the same turn as the Backpressure result.
      ctx.fireEvent(
          StreamEvent::FlowControlPause, channel_pipeline::TypeErasedBox{});
    }
    if (forwarded != channel_pipeline::Result::Success) {
      // Downstream congestion or failure: propagate its status unchanged.
      return forwarded;
    }
    // Raise backpressure on the very element that exhausts demand so the
    // producer pauses before emitting the next one, not after.
    return credits_ == 0 ? channel_pipeline::Result::Backpressure
                         : channel_pipeline::Result::Success;
  }

  void onWriteReady(Context& /*ctx*/) noexcept {}

  // Intentionally a no-op: granted credit is the peer's outstanding demand and
  // persists across a transport pause — it is not reset here. The credit dies
  // with this per-stream handler when the stream ends.
  void onPipelineInactive(Context& /*ctx*/) noexcept {}

 private:
  // A Payload reached this consuming sink while demand is exhausted: the
  // producer overran the credit contract. An upstream buffer honoring
  // FlowControlPause keeps this unreachable, so reaching here is a bug rather
  // than a runtime condition — fail loudly in debug. In release the hard
  // contract still holds: the element is dropped with Result::Error (tearing
  // down the peer) rather than delivered beyond demand. Kept out-of-line so the
  // cold violation path adds nothing to the hot path.
  FOLLY_NOINLINE static channel_pipeline::Result
  onCreditExhaustedViolation() noexcept {
    DCHECK(false) << "Payload written while stream credit is exhausted "
                     "(credit-contract violation)";
    return channel_pipeline::Result::Error;
  }

  // A credit grant that would overflow the budget saturates to the maximum
  // rather than wrapping; a silent wrap would collapse outstanding demand back
  // toward zero and start dropping authorized elements. Overflow needs the peer
  // to grant ~2^64 credits, so it is practically unreachable — hence the
  // unlikely hint — but detecting it keeps the budget monotonic.
  static constexpr uint64_t addSaturating(
      uint64_t credits, uint64_t delta) noexcept {
    const uint64_t sum = credits + delta;
    if (FOLLY_UNLIKELY(sum < credits)) {
      return std::numeric_limits<uint64_t>::max();
    }
    return sum;
  }

  uint64_t credits_{0};
};

static_assert(
    channel_pipeline::InboundHandler<
        InboundCreditHandler<channel_pipeline::detail::ContextImpl>,
        channel_pipeline::detail::ContextImpl>,
    "InboundCreditHandler must satisfy InboundHandler concept");

static_assert(
    channel_pipeline::OutboundHandler<
        InboundCreditHandler<channel_pipeline::detail::ContextImpl>,
        channel_pipeline::detail::ContextImpl>,
    "InboundCreditHandler must satisfy OutboundHandler concept");

} // namespace apache::thrift::fast_thrift::thrift::stream
