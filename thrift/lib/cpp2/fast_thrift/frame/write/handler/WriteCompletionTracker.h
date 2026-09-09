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

#include <concepts>
#include <cstddef>
#include <deque>
#include <utility>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/transport/WriteCompletion.h>

namespace apache::thrift::fast_thrift::frame::write::handler {

/**
 * Composable tracker mixin for batching frame handlers. The batcher invokes
 * the tracker's hooks at four points:
 *   - onWrite()           — per outbound frame entering the current batch.
 *   - onFlush()           — when the current batch is handed off downstream
 *                           (the batch boundary).
 *   - onDiscard()         — when the batcher abandons its buffered writes
 *                           instead of flushing them, so the tracker's counts
 *                           don't outlive the frames they stand for.
 *   - on<Event>(ctx, box) — when the pipeline's typed event arrives via the
 *                           batcher's matching handler. The tracker subscribes
 *                           only to the raw transport-fired
 *                           TransportWriteComplete event, so its own enriched
 *                           re-fires are never routed back to it.
 *
 * `on<Event>` is a member template parameterized on the pipeline's Context
 * type, so the tracker — not the batcher — owns the event type.
 */
template <typename T>
concept WriteCompletionTracker =
    channel_pipeline::kIsEventSet<typename T::PublishedEvents> &&
    channel_pipeline::kIsEventSet<typename T::SubscribedEvents> &&
    requires(T tracker) {
      { tracker.onWrite() } noexcept;
      { tracker.onFlush() } noexcept;
      { tracker.onDiscard() } noexcept;
    };

struct NoOpWriteCompletionTracker {
  using PublishedEvents = channel_pipeline::Events<>;
  using SubscribedEvents = channel_pipeline::Events<>;
  using FlushWritesEventType = void;

  void onWrite() noexcept {}
  void onFlush() noexcept {}
  void onDiscard() noexcept {}
};

static_assert(
    WriteCompletionTracker<NoOpWriteCompletionTracker>,
    "NoOpWriteCompletionTracker must satisfy WriteCompletionTracker concept");

namespace detail {
template <
    channel_pipeline::PipelineEvent... A,
    channel_pipeline::PipelineEvent... B>
constexpr channel_pipeline::Events<A..., B...> concatEvents(
    channel_pipeline::Events<A...>, channel_pipeline::Events<B...>) noexcept {
  return {};
}

template <typename E>
constexpr auto optionalEventSet() noexcept {
  if constexpr (channel_pipeline::PipelineEvent<E>) {
    return channel_pipeline::Events<E>{};
  } else {
    return channel_pipeline::Events<>{};
  }
}
} // namespace detail

template <typename Tracker, typename FlushEvent>
using BatcherSubscribedEvents = decltype(detail::concatEvents(
    typename Tracker::SubscribedEvents{},
    detail::optionalEventSet<FlushEvent>()));

template <typename T>
concept BatchWriteCompleteEventFactory =
    channel_pipeline::PipelineEvent<
        typename T::TransportWriteCompleteEventType> &&
    channel_pipeline::PipelineEvent<typename T::BatchWriteCompleteEventType> &&
    requires(
        typename T::TransportWriteCompleteEventType transportEvent,
        size_t frameCount,
        bool quiesced) {
      typename T::FlushWritesEventType;
      {
        T::makeBatchWriteComplete(
            transportEvent.status, frameCount, transportEvent.bytes, quiesced)
      } noexcept -> std::same_as<typename T::BatchWriteCompleteEventType>;
    };

/**
 * Concrete tracker — counts outbound frames per batch and, on each raw
 * TransportWriteComplete from transport, pops the front batch's frame count
 * and fires a BatchWriteComplete (enriched with frameCount and whether egress
 * has gone idle) upstream via
 * `EventFactory::makeBatchWriteComplete(status, count, bytes, quiesced)`.
 *
 * Batch-level is as far as this tracker's knowledge goes. Turning a batch
 * completion into whatever the pipeline's upper layers want — per-frame, or
 * per-connection — belongs to the handler above, which is the first one that
 * knows what the batch was made of.
 *
 * The tracker is the only place that can decide quiescence: it owns both the
 * queue of batches handed to the socket and the count of frames still buffered
 * for the next flush.
 *
 * Quiescence is an edge carried on a completion, not a state to be polled, and
 * the edge is not guaranteed to arrive: only a completion that pops a batch
 * reports it, and a connection torn down with batches still outstanding stops
 * receiving completions altogether. A consumer that releases a resource on
 * quiescence must release it on teardown too.
 *
 * Templated on the pipeline's event factory (see
 * BatchWriteCompleteEventFactory). The factory must expose:
 *   - `using EventId = ...;` with `TransportWriteComplete` and
 *     `BatchWriteComplete` values.
 *   - `using TransportWriteCompleteEventType = ...;` — the message carried by
 *     the TransportWriteComplete event, with `status` and `bytes` fields.
 *   - `static TypeErasedBox makeBatchWriteComplete(status, count, bytes,
 * quiesced) noexcept;`
 *
 * EB-thread only — no synchronization. The batch-count FIFO stays in
 * lockstep with the transport's writeSuccess/writeErr FIFO ordering
 * (per AsyncSocket's structural write-queue guarantee).
 */
template <BatchWriteCompleteEventFactory EventFactory>
class WriteCompletionTrackerT {
 public:
  using TransportEvent = typename EventFactory::TransportWriteCompleteEventType;
  using BatchEvent = typename EventFactory::BatchWriteCompleteEventType;
  using FlushWritesEventType = typename EventFactory::FlushWritesEventType;
  using PublishedEvents = channel_pipeline::Events<BatchEvent>;
  using SubscribedEvents = channel_pipeline::Events<TransportEvent>;

  void onWrite() noexcept { ++framesInCurrentBatch_; }

  void onFlush() noexcept {
    if (framesInCurrentBatch_ == 0) {
      return;
    }
    batchFrameCounts_.push_back(framesInCurrentBatch_);
    framesInCurrentBatch_ = 0;
  }

  // The batcher threw away what it had buffered, so the counts standing for
  // those frames have to go too — otherwise the partial batch is charged to
  // whatever flushes next and quiescence can never be reached again. The
  // batcher only discards on teardown, which is also the point past which no
  // completion arrives for the batches already handed to the socket.
  void onDiscard() noexcept {
    framesInCurrentBatch_ = 0;
    batchFrameCounts_.clear();
  }

  template <channel_pipeline::PipelineEvent E, typename Context>
    requires std::same_as<E, TransportEvent>
  void on(Context& ctx, const TransportEvent& evt) noexcept {
    if (batchFrameCounts_.empty()) {
      return;
    }
    auto count = batchFrameCounts_.front();
    batchFrameCounts_.pop_front();
    const bool quiesced =
        batchFrameCounts_.empty() && framesInCurrentBatch_ == 0;
    PublishedEvents::template fire<BatchEvent>(
        ctx,
        EventFactory::makeBatchWriteComplete(
            evt.status, count, evt.bytes, quiesced));
  }

 private:
  size_t framesInCurrentBatch_{0};
  std::deque<size_t> batchFrameCounts_;
};

} // namespace apache::thrift::fast_thrift::frame::write::handler
