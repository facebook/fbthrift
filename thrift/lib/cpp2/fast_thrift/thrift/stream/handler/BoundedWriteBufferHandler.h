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

#include <cstddef>
#include <utility>
#include <vector>

#include <glog/logging.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Backpressure.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Handler.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/ContextImpl.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/StreamEvents.h>

namespace apache::thrift::fast_thrift::thrift::stream {

/**
 * Configuration for BoundedWriteBufferHandler.
 */
struct BoundedWriteBufferConfig {
  // Maximum elements held while the handler cannot send. The element that fills
  // the buffer is held but returns Backpressure; any element sent while full is
  // dropped with Error. 0 disables buffering (every element sent while unable
  // to send is dropped).
  size_t maxBufferedElements{256};
};

namespace detail {

/**
 * A fixed-capacity, single-threaded FIFO ring of stream messages. The backing
 * store is allocated once, lazily on first push, so an idle handler holds no
 * buffer memory. push() must not be called when full().
 *
 * A ring rather than std::deque: with bounded occupancy the ring allocates
 * exactly once and then reuses its slots via modular indexing forever, so a
 * steady stream never touches the allocator. A deque has no capacity reserve
 * and, under the rolling push_back/pop_front pattern here, keeps allocating and
 * freeing blocks as the window slides even though live occupancy stays bounded.
 */
class BoundedMessageRing {
 public:
  explicit BoundedMessageRing(size_t capacity) noexcept : capacity_(capacity) {}

  bool empty() const noexcept { return count_ == 0; }
  bool full() const noexcept { return count_ == capacity_; }

  void push(ThriftStreamMessage msg) noexcept {
    DCHECK(!full()) << "push() on a full ring; caller must check full() first";
    if (slots_.empty()) {
      slots_.resize(capacity_);
    }
    slots_[(head_ + count_) % capacity_] = std::move(msg);
    ++count_;
  }

  ThriftStreamMessage& front() noexcept { return slots_[head_]; }

  void pop() noexcept {
    slots_[head_] = ThriftStreamMessage{}; // release the element's IOBuf
    head_ = (head_ + 1) % capacity_;
    --count_;
  }

 private:
  size_t capacity_;
  size_t head_{0};
  size_t count_{0};
  std::vector<ThriftStreamMessage> slots_;
};

} // namespace detail

/**
 * BoundedWriteBufferHandler — a bounded, FIFO outbound buffer for the
 * established stream sub-pipeline, and the single outbound flow-control point:
 * a payload is forwarded only when it can be sent, and held otherwise.
 *
 * Contract: only stream data (`Payload`) is buffered. Control frames (e.g. an
 * outbound `RequestN` grant) always bypass the buffer and are forwarded
 * immediately, even while payloads are held.
 *
 * Two independent gates decide whether a payload can be sent:
 *   - Credit: gated by StreamEvent flow-control events from
 * InboundCreditHandler (credit itself stays private there). `FlowControlPause`
 * closes the gate, `FlowControlResume` opens it. It starts closed — no credit
 * is granted until the peer's first RequestN.
 *   - Transport: gated by the data path. A `Result::Backpressure` from
 *     downstream (socket saturation, taken to mean the element was consumed
 *     since `fireWrite` is a sink) closes it; the pipeline's `onWriteReady`
 *     opens it.
 *
 * While either gate is closed the handler holds payloads in FIFO order up to
 * `maxBufferedElements`; the payload that fills the buffer is held but returns
 * `Result::Backpressure`, and any further payload while full is dropped with
 * `Result::Error`. When both gates are open, buffered payloads drain in FIFO
 * order. A `Backpressure` return during a forward is credit exhaustion if a
 * `FlowControlPause` was delivered in the same turn (it is synchronous, so
 * `creditPaused_` is already set), otherwise transport congestion — the two are
 * disjoint, so a wake for one never drains into the other.
 *
 * A `Complete` (end-of-stream) frame is ordered after payloads: if payloads are
 * still buffered or a gate is closed it is held as a pending-completion flag
 * and emitted once the buffer fully drains, otherwise it is forwarded
 * immediately. End-of-stream is terminal: once a `Complete` has been accepted
 * (held or forwarded), any further payload or a second `Complete` is a protocol
 * violation, dropped with `Result::Error`.
 */
template <typename Context>
class BoundedWriteBufferHandler {
 public:
  explicit BoundedWriteBufferHandler(BoundedWriteBufferConfig config = {})
      : buffer_(config.maxBufferedElements) {}

  // Detected by makeHandlerNode — registers this handler on the pipeline's
  // writeReadyList so onWriteReady() fires when the transport drains.
  channel_pipeline::WriteReadyHook writeReadyHook_;

  // Credit flow-control readiness is delivered out-of-band as user events.
  static constexpr channel_pipeline::Subscriptions<
      StreamEvent::FlowControlPause,
      StreamEvent::FlowControlResume>
      kSubscribedEvents{};

  // HandlerLifecycle
  void handlerAdded(Context& /*ctx*/) noexcept {}
  void handlerRemoved(Context& /*ctx*/) noexcept {}

  // Credit readiness. A Pause closes the credit gate; a Resume opens it and
  // drains what credit now allows.
  void onEvent(
      Context& ctx,
      StreamEvent ev,
      const channel_pipeline::TypeErasedBox& /*msg*/) noexcept {
    // Only the two subscribed flow-control events are ever delivered here.
    if (ev == StreamEvent::FlowControlPause) {
      creditPaused_ = true;
    } else if (ev == StreamEvent::FlowControlResume) {
      creditPaused_ = false;
      tryDrain(ctx);
    }
  }

  // OutboundHandler
  channel_pipeline::Result onWrite(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    auto& message = msg.get<ThriftStreamMessage>();
    if (message.payload.is<Complete>()) {
      return onComplete(ctx, std::move(msg));
    }
    // Only stream data is subject to flow control. Control frames (e.g. an
    // outbound RequestN credit grant) bypass the buffer and go straight to the
    // wire, even while payloads are held.
    if (!message.payload.is<Payload>()) {
      return ctx.fireWrite(std::move(msg));
    }
    if (completionSeen()) {
      // Data after end-of-stream is a protocol violation: drop it.
      return channel_pipeline::Result::Error;
    }
    if (holding()) {
      return bufferOrRefuse(std::move(msg));
    }
    // Both gates open and nothing queued: forward directly.
    const channel_pipeline::Result result = ctx.fireWrite(std::move(msg));
    if (result == channel_pipeline::Result::Backpressure) {
      noteBackpressure(ctx);
      return channel_pipeline::Result::Backpressure;
    }
    return result;
  }

  void onWriteReady(Context& ctx) noexcept {
    // The transport drained; reopen that gate and drain what credit allows.
    transportPaused_ = false;
    tryDrain(ctx);
  }

  // Intentionally a no-op: buffered elements are the stream's unsent output.
  // A transport pause is not a stream end, so they are kept (a reactivated
  // pipeline can still deliver them) and freed only when this per-stream
  // handler is destroyed.
  void onPipelineInactive(Context& /*ctx*/) noexcept {}

 private:
  bool holding() const noexcept {
    return creditPaused_ || transportPaused_ || !buffer_.empty();
  }

  // End-of-stream has been accepted — either held pending a buffer drain, or
  // already forwarded. Either way the stream is terminal.
  bool completionSeen() const noexcept {
    return completePending_ || completed_;
  }

  // Classify a downstream Backpressure and arm the matching resume. A
  // FlowControlPause fired synchronously during the forward would already have
  // set creditPaused_, so a set flag means credit exhaustion (wait for
  // FlowControlResume); otherwise it is transport congestion (await
  // write-ready).
  void noteBackpressure(Context& ctx) noexcept {
    if (!creditPaused_) {
      transportPaused_ = true;
      ctx.awaitWriteReady();
    }
  }

  void tryDrain(Context& ctx) noexcept {
    while (!buffer_.empty() && !creditPaused_ && !transportPaused_) {
      auto box = channel_pipeline::erase_and_box(std::move(buffer_.front()));
      buffer_.pop();
      if (ctx.fireWrite(std::move(box)) ==
          channel_pipeline::Result::Backpressure) {
        noteBackpressure(ctx);
        return;
      }
    }
    if (buffer_.empty()) {
      // Payloads are flushed. A held completion is not gated by credit (it
      // carries no data), so emit it now that it is the final element; then
      // stop awaiting the transport unless it is still congested.
      if (completePending_) {
        completePending_ = false;
        completed_ = true;
        (void)ctx.fireWrite(
            channel_pipeline::erase_and_box(
                ThriftStreamMessage{.payload = Complete{}}));
      }
      if (!transportPaused_) {
        ctx.cancelAwaitWriteReady();
      }
    }
  }

  channel_pipeline::Result onComplete(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    if (completionSeen()) {
      // End-of-stream is terminal: a second completion is a protocol violation.
      return channel_pipeline::Result::Error;
    }
    if (!buffer_.empty()) {
      // Payloads are still buffered; hold completion until they drain so it
      // stays the last element on the stream. Completion is ordered only after
      // payloads — it is not gated by credit or transport, both of which govern
      // data, so an empty buffer forwards it immediately.
      completePending_ = true;
      return channel_pipeline::Result::Success;
    }
    completed_ = true;
    return ctx.fireWrite(std::move(msg));
  }

  channel_pipeline::Result bufferOrRefuse(
      channel_pipeline::TypeErasedBox&& msg) noexcept {
    if (buffer_.full()) {
      // Already at the bound (or buffering disabled) and asked to hold more:
      // the producer overran the backpressure signal. Drop the element with an
      // error.
      return channel_pipeline::Result::Error;
    }
    buffer_.push(msg.take<ThriftStreamMessage>());
    // The element that fills the buffer is held, but backpressure is raised now
    // so the producer pauses as the bound is reached rather than after.
    return buffer_.full() ? channel_pipeline::Result::Backpressure
                          : channel_pipeline::Result::Success;
  }

  // Credit starts closed: no payload may be sent until the peer's first
  // RequestN produces a FlowControlResume.
  bool creditPaused_{true};
  bool transportPaused_{false};
  bool completePending_{false};
  bool completed_{false};
  detail::BoundedMessageRing buffer_;
};

static_assert(
    channel_pipeline::OutboundHandler<
        BoundedWriteBufferHandler<channel_pipeline::detail::ContextImpl>,
        channel_pipeline::detail::ContextImpl>,
    "BoundedWriteBufferHandler must satisfy OutboundHandler concept");

} // namespace apache::thrift::fast_thrift::thrift::stream
