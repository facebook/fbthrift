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
#include <cstdint>
#include <deque>
#include <optional>

#include <folly/Utility.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp2/fast_thrift/frame/write/ComposedFrame.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/PerStreamState.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/QuantumFragmentationHandlerConfig.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/SrptHeap.h>

namespace apache::thrift::fast_thrift::frame::write::handler {

/** Keeps yielding SRPT work between EventBase callbacks. */
// folly::NonCopyableNonMovable deletes copy and move.
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class YieldingSrptFrameFragmentationHandlerState final
    : private folly::EventBase::LoopCallback,
      private folly::NonCopyableNonMovable {
 public:
  enum class WriteDisposition { PassThrough, Defer };

  struct ScheduledFragment {
    uint32_t streamId{0};
    PerStreamState::Fragment fragment;
  };

  using FlushTrampoline = void (*)(void*, void*) noexcept;

  explicit YieldingSrptFrameFragmentationHandlerState(
      QuantumFragmentationHandlerConfig config = {}) noexcept;
  ~YieldingSrptFrameFragmentationHandlerState() override;

  void attach(
      folly::EventBase* eventBase,
      void* handler,
      void* context,
      FlushTrampoline flushTrampoline) noexcept;
  void detach() noexcept;
  void clearPending() noexcept;

  /**
   * For `CANCEL` and `ERROR`, removes the queued frames of the stream first.
   * Returns `Defer` for fragmentable data frames and `PassThrough` for all
   * other frames.
   */
  [[nodiscard]] WriteDisposition prepareWrite(
      const ComposedFrame& frame) noexcept;
  /** Returns false and drops the frame when the config holds a bad value. */
  [[nodiscard]] bool enqueue(ComposedFrame&& frame);

  /** Starts one pass and snapshots its number of Large turns. */
  void startPass() noexcept;
  /**
   * Returns the next Small or pinned Large fragment for this pass, or nullopt
   * when the pass is over. A write that fails inside the pass can close the
   * pipeline, and `clearPending()` then ends the pass early.
   */
  [[nodiscard]] std::optional<ScheduledFragment> nextFragment() noexcept;
  /** Schedules another pass when work remains. */
  [[nodiscard]] bool finishPass() noexcept;
  void stopAfterError() noexcept;

  [[nodiscard]] bool hasPendingStream(uint32_t streamId) const noexcept;
  [[nodiscard]] bool hasPendingWrites() const noexcept;
  [[nodiscard]] bool isScheduled() const noexcept;

 private:
  struct StreamState {
    PerStreamState stream;
    size_t priority{0};
    bool singleQuantum{false};
  };

  struct PriorityFn {
    size_t operator()(const StreamState& state) const noexcept {
      return state.priority;
    }
  };

  void runLoopCallback() noexcept override;
  void scheduleFlushIfNeeded() noexcept;
  void cancelLoopCallbackIfScheduled() noexcept;
  void removeStream(uint32_t streamId) noexcept;
  void mergeDeferredArrivals();
  void enqueueNow(ComposedFrame&& frame);
  void updatePriority(StreamState& state) const noexcept;
  [[nodiscard]] size_t largeStreamCount() noexcept;
  [[nodiscard]] ScheduledFragment takeSmallFragment() noexcept;
  [[nodiscard]] ScheduledFragment takeLargeFragment() noexcept;
  [[nodiscard]] PerStreamState::Fragment takeFragment(
      StreamState& state) noexcept;
  void spendCallbackBudget(size_t dataBytes) noexcept;

  const size_t streamQuantum_;
  const size_t callbackBudget_;
  const bool configValid_;
  folly::EventBase* eventBase_{nullptr};
  void* handler_{nullptr};
  void* context_{nullptr};
  FlushTrampoline flushTrampoline_{nullptr};
  bool isScheduled_{false};
  bool passActive_{false};
  bool smallPhase_{false};
  size_t remainingCallbackBudget_{0};
  size_t remainingLargeTurns_{0};
  SrptHeap<StreamState, PriorityFn> streams_;
  std::optional<StreamState> pinnedLarge_;
  std::deque<ComposedFrame> deferredArrivals_;
};

} // namespace apache::thrift::fast_thrift::frame::write::handler
