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

/**
 * LoopBatchingFrameHandler - Outbound handler for loop-iteration write
 * batching.
 *
 * Mirrors RocketClient's client-side batching pattern: accumulates all writes
 * enqueued within a single event loop iteration and flushes them together at
 * the end of the iteration via LoopCallback with double-scheduling (reschedule
 * once to push to the back of the loop queue, ensuring all writes from the
 * current iteration are captured).
 *
 * Input:  std::unique_ptr<folly::IOBuf> (individual frames)
 * Output: std::unique_ptr<folly::IOBuf> (coalesced batch)
 */

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/WriteCompletionTracker.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/util/BatchFlushScheduler.h>

#include <functional>
#include <stdexcept>

namespace apache::thrift::fast_thrift::frame::write::handler {

template <WriteCompletionTracker Tracker = NoOpWriteCompletionTracker>
class LoopBatchingFrameHandlerT {
 public:
  LoopBatchingFrameHandlerT() noexcept
      : scheduler_(
            util::BatchFlushScheduler::DeferredFlushMode::EndOfNextLoop) {}

  ~LoopBatchingFrameHandlerT() { scheduler_.cancelAll(); }

  LoopBatchingFrameHandlerT(const LoopBatchingFrameHandlerT&) = delete;
  LoopBatchingFrameHandlerT& operator=(const LoopBatchingFrameHandlerT&) =
      delete;
  LoopBatchingFrameHandlerT(LoopBatchingFrameHandlerT&&) = delete;
  LoopBatchingFrameHandlerT& operator=(LoopBatchingFrameHandlerT&&) = delete;

  // ===========================================================================
  // HandlerLifecycle
  // ===========================================================================

  template <typename Context>
  void handlerAdded(Context& ctx) noexcept {
    scheduler_.setEventBase(ctx.eventBase());
    scheduler_.setFlushFunction(
        [this, &ctx]() { flushAndPropagateErrors(ctx); });
  }

  template <typename Context>
  void handlerRemoved(Context& /*ctx*/) noexcept {
    clearPendingState();
    scheduler_.setEventBase(nullptr);
    scheduler_.clearFlushFunction();
    scheduler_.setFlushList(nullptr);
  }

  // ===========================================================================
  // OutboundHandler
  // ===========================================================================

  template <typename Context>
  [[nodiscard]] channel_pipeline::Result onWrite(
      Context& /*ctx*/, channel_pipeline::TypeErasedBox&& msg) noexcept {
    auto frame = msg.take<std::unique_ptr<folly::IOBuf>>();

    if (!frame) {
      return channel_pipeline::Result::Success;
    }

    bufferedWritesQueue_.append(std::move(frame));
    tracker_.onWrite();
    scheduleFlushIfNeeded();
    return channel_pipeline::Result::Success;
  }

  template <typename Context>
  void onPipelineInactive(Context& /*ctx*/) noexcept {
    drain();
  }

  template <typename Context>
  void onWriteReady(Context& /*ctx*/) noexcept {}

  using PublishedEvents = typename Tracker::PublishedEvents;
  using SubscribedEvents = typename Tracker::SubscribedEvents;

  template <channel_pipeline::PipelineEvent E, typename Context>
  void on(Context& ctx, const typename E::Payload& event) noexcept {
    tracker_.template on<E>(ctx, event);
  }

  /**
   * Synchronously flush all pending writes.
   * Cancels any scheduled callbacks and flushes immediately.
   */
  void drain() noexcept {
    if (bufferedWritesQueue_.empty()) {
      return;
    }
    cancelLoopCallbackIfScheduled();
    scheduler_.flushNow();
  }

  // ===========================================================================
  // Flush list
  // ===========================================================================

  // An intrusive list of deferred flush callbacks. Each handler enqueues at
  // most one scheduler-owned entry.
  using FlushList = util::BatchFlushScheduler::FlushList;

  // Redirects the end-of-loop flush from the EventBase to a caller-owned list.
  //
  // By default the batcher schedules its deferred flush on the EventBase and
  // flushes at the end of the loop iteration (double-scheduled to capture the
  // whole iteration). When a flush list is provided, the batcher enqueues onto
  // that list instead of self-scheduling, and the caller's drain point defines
  // the flush boundary, so the buffered writes flush directly on drain.
  //
  // The list must outlive this handler. Pass nullptr to restore EventBase
  // scheduling.
  void setFlushList(FlushList* flushList) noexcept {
    scheduler_.setFlushList(flushList);
    if (!bufferedWritesQueue_.empty()) {
      scheduleFlushIfNeeded();
    }
  }

  // ===========================================================================
  // Accessors (for testing)
  // ===========================================================================

  bool isScheduled() const noexcept {
    return scheduler_.hasScheduledDeferredFlush();
  }
  bool empty() const noexcept { return bufferedWritesQueue_.empty(); }
  Tracker& tracker() noexcept { return tracker_; }

 private:
  void scheduleFlushIfNeeded() noexcept { scheduler_.scheduleDeferredFlush(); }

  void cancelLoopCallbackIfScheduled() noexcept {
    scheduler_.cancelDeferredFlush();
  }

  void clearPendingState() noexcept {
    cancelLoopCallbackIfScheduled();
    bufferedWritesQueue_.move(); // discard
    tracker_.onDiscard();
  }

  template <typename Context>
  void flushAndPropagateErrors(Context& ctx) noexcept {
    if (doFlush(ctx) == channel_pipeline::Result::Error) {
      ctx.fireException(
          folly::make_exception_wrapper<std::runtime_error>(
              "LoopBatchingFrameHandler: downstream write failed"));
    }
  }

  template <typename Context>
  [[nodiscard]] channel_pipeline::Result doFlush(Context& ctx) noexcept {
    auto batchToSend = bufferedWritesQueue_.move();
    if (!batchToSend) {
      return channel_pipeline::Result::Success;
    }

    tracker_.onFlush();
    return ctx.fireWrite(
        channel_pipeline::TypeErasedBox(std::move(batchToSend)));
  }

  util::BatchFlushScheduler scheduler_;

  folly::IOBufQueue bufferedWritesQueue_{folly::IOBufQueue::cacheChainLength()};

  // Per-write tracker mixin; NoOp by default.
  [[no_unique_address]] Tracker tracker_{};
};

// Default specialization preserves the existing class name for callers that
// don't opt into per-write tracking.
using LoopBatchingFrameHandler =
    LoopBatchingFrameHandlerT<NoOpWriteCompletionTracker>;

} // namespace apache::thrift::fast_thrift::frame::write::handler
