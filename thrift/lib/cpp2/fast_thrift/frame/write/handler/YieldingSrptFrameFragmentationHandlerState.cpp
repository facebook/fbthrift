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

#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/YieldingSrptFrameFragmentationHandlerState.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include <glog/logging.h>

#include <thrift/lib/cpp2/fast_thrift/frame/FrameType.h>

namespace apache::thrift::fast_thrift::frame::write::handler {

namespace {

// A frame sends all its metadata with its first fragment.
size_t unsentMetadataSize(const PendingFrame& frame) noexcept {
  return !frame.firstEmitted && frame.originalFrame.metadata != nullptr
      ? frame.originalFrame.metadata->computeChainDataLength()
      : 0;
}

// Only fragmentable frames are deferred, so a RESERVED one was removed.
bool isRemovedArrival(const ComposedFrame& frame) noexcept {
  return frame.frameType == FrameType::RESERVED;
}

} // namespace

YieldingSrptFrameFragmentationHandlerState::
    YieldingSrptFrameFragmentationHandlerState(
        QuantumFragmentationHandlerConfig config) noexcept
    : streamQuantum_(config.streamQuantum),
      callbackBudget_(config.callbackBudget),
      configValid_(config.isValid()) {}

YieldingSrptFrameFragmentationHandlerState::
    ~YieldingSrptFrameFragmentationHandlerState() {
  cancelLoopCallbackIfScheduled();
}

void YieldingSrptFrameFragmentationHandlerState::attach(
    folly::EventBase* eventBase,
    void* handler,
    void* context,
    FlushTrampoline flushTrampoline) noexcept {
  eventBase_ = eventBase;
  handler_ = handler;
  context_ = context;
  flushTrampoline_ = flushTrampoline;
}

void YieldingSrptFrameFragmentationHandlerState::detach() noexcept {
  clearPending();
  eventBase_ = nullptr;
  handler_ = nullptr;
  context_ = nullptr;
  flushTrampoline_ = nullptr;
}

void YieldingSrptFrameFragmentationHandlerState::clearPending() noexcept {
  cancelLoopCallbackIfScheduled();
  streams_.clear();
  pinnedLarge_.reset();
  deferredArrivals_.clear();
  passActive_ = false;
  smallPhase_ = false;
  remainingCallbackBudget_ = 0;
  remainingLargeTurns_ = 0;
}

YieldingSrptFrameFragmentationHandlerState::WriteDisposition
YieldingSrptFrameFragmentationHandlerState::prepareWrite(
    const ComposedFrame& frame) noexcept {
  if (isTerminalFrameType(frame.frameType)) {
    removeStream(frame.streamId);
    return WriteDisposition::PassThrough;
  }
  if (!frame.canFragment()) {
    return WriteDisposition::PassThrough;
  }
  return WriteDisposition::Defer;
}

bool YieldingSrptFrameFragmentationHandlerState::enqueue(
    ComposedFrame&& frame) {
  if (!configValid_) {
    return false;
  }
  if (passActive_) {
    deferredArrivals_.push_back(std::move(frame));
  } else {
    enqueueNow(std::move(frame));
    scheduleFlushIfNeeded();
  }
  return true;
}

void YieldingSrptFrameFragmentationHandlerState::startPass() noexcept {
  DCHECK(!passActive_);
  passActive_ = true;
  smallPhase_ = true;
  remainingCallbackBudget_ = callbackBudget_;
  remainingLargeTurns_ = largeStreamCount();
}

std::optional<YieldingSrptFrameFragmentationHandlerState::ScheduledFragment>
YieldingSrptFrameFragmentationHandlerState::nextFragment() noexcept {
  if (remainingCallbackBudget_ == 0) {
    return std::nullopt;
  }

  if (smallPhase_) {
    if (!streams_.empty() && streams_.peekMin().singleQuantum) {
      ScheduledFragment scheduled = takeSmallFragment();
      spendCallbackBudget(scheduled.fragment.payloadBytes);
      return scheduled;
    }
    smallPhase_ = false;
  }

  if (remainingLargeTurns_ == 0) {
    return std::nullopt;
  }
  if (!pinnedLarge_.has_value()) {
    if (streams_.empty()) {
      return std::nullopt;
    }
    DCHECK(!streams_.peekMin().singleQuantum);
  }

  --remainingLargeTurns_;
  ScheduledFragment scheduled = takeLargeFragment();
  spendCallbackBudget(scheduled.fragment.payloadBytes);
  return scheduled;
}

bool YieldingSrptFrameFragmentationHandlerState::finishPass() noexcept {
  passActive_ = false;
  mergeDeferredArrivals();
  if (!hasPendingWrites()) {
    return true;
  }
  scheduleFlushIfNeeded();
  return false;
}

void YieldingSrptFrameFragmentationHandlerState::stopAfterError() noexcept {
  clearPending();
}

bool YieldingSrptFrameFragmentationHandlerState::hasPendingStream(
    uint32_t streamId) const noexcept {
  if (streams_.contains(streamId) ||
      (pinnedLarge_.has_value() && pinnedLarge_->stream.streamId == streamId)) {
    return true;
  }
  for (const ComposedFrame& frame : deferredArrivals_) {
    if (!isRemovedArrival(frame) && frame.streamId == streamId) {
      return true;
    }
  }
  return false;
}

bool YieldingSrptFrameFragmentationHandlerState::hasPendingWrites()
    const noexcept {
  return !streams_.empty() || pinnedLarge_.has_value() ||
      !std::all_of(
          deferredArrivals_.begin(), deferredArrivals_.end(), isRemovedArrival);
}

bool YieldingSrptFrameFragmentationHandlerState::isScheduled() const noexcept {
  return isScheduled_;
}

void YieldingSrptFrameFragmentationHandlerState::runLoopCallback() noexcept {
  isScheduled_ = false;
  if (flushTrampoline_ != nullptr) {
    flushTrampoline_(handler_, context_);
  }
}

void YieldingSrptFrameFragmentationHandlerState::
    scheduleFlushIfNeeded() noexcept {
  if (!isScheduled_ && eventBase_ != nullptr && hasPendingWrites()) {
    eventBase_->runInLoop(this);
    isScheduled_ = true;
  }
}

void YieldingSrptFrameFragmentationHandlerState::
    cancelLoopCallbackIfScheduled() noexcept {
  if (isScheduled_) {
    cancelLoopCallback();
    isScheduled_ = false;
  }
}

void YieldingSrptFrameFragmentationHandlerState::removeStream(
    uint32_t streamId) noexcept {
  streams_.erase(streamId);
  if (pinnedLarge_.has_value() && pinnedLarge_->stream.streamId == streamId) {
    pinnedLarge_.reset();
  }
  for (ComposedFrame& frame : deferredArrivals_) {
    if (frame.streamId == streamId) {
      frame = ComposedFrame{};
    }
  }
}

void YieldingSrptFrameFragmentationHandlerState::mergeDeferredArrivals() {
  for (ComposedFrame& frame : deferredArrivals_) {
    if (!isRemovedArrival(frame)) {
      enqueueNow(std::move(frame));
    }
  }
  deferredArrivals_.clear();
}

void YieldingSrptFrameFragmentationHandlerState::enqueueNow(
    ComposedFrame&& frame) {
  const uint32_t streamId = frame.streamId;
  std::unique_ptr<folly::IOBuf> data = std::move(frame.data);
  const bool originalComplete = frame.isComplete();

  if (pinnedLarge_.has_value() && pinnedLarge_->stream.streamId == streamId) {
    pinnedLarge_->stream.enqueue(
        std::move(frame), std::move(data), originalComplete);
    return;
  }

  if (StreamState* state = streams_.find(streamId)) {
    state->stream.enqueue(std::move(frame), std::move(data), originalComplete);
    updatePriority(*state);
    streams_.update(streamId);
    return;
  }

  StreamState state;
  state.stream.streamId = streamId;
  state.stream.enqueue(std::move(frame), std::move(data), originalComplete);
  updatePriority(state);
  streams_.insert(streamId, std::move(state));
}

void YieldingSrptFrameFragmentationHandlerState::updatePriority(
    StreamState& state) const noexcept {
  CHECK(!state.stream.frames.empty());
  const size_t metadataSize = unsentMetadataSize(state.stream.frames.front());
  state.singleQuantum = state.stream.frames.size() == 1 &&
      metadataSize <= streamQuantum_ &&
      state.stream.remaining() <= streamQuantum_ - metadataSize;
  if (state.singleQuantum) {
    state.priority = state.stream.remaining();
  } else {
    const size_t largeOffset = streamQuantum_ + 1;
    const size_t remaining = state.stream.remaining();
    state.priority =
        remaining > std::numeric_limits<size_t>::max() - largeOffset
        ? std::numeric_limits<size_t>::max()
        : largeOffset + remaining;
  }
}

size_t YieldingSrptFrameFragmentationHandlerState::largeStreamCount() noexcept {
  size_t count = pinnedLarge_.has_value() ? 1 : 0;
  streams_.forEach([&](uint32_t, StreamState& state) {
    if (!state.singleQuantum) {
      ++count;
    }
  });
  return count;
}

YieldingSrptFrameFragmentationHandlerState::ScheduledFragment
YieldingSrptFrameFragmentationHandlerState::takeSmallFragment() noexcept {
  StreamState state = streams_.extractMin();
  const uint32_t streamId = state.stream.streamId;
  DCHECK(state.singleQuantum);
  PerStreamState::Fragment fragment = takeFragment(state);
  DCHECK(fragment.currentFrameDone);
  DCHECK(!state.stream.hasMore());
  return ScheduledFragment{streamId, std::move(fragment)};
}

YieldingSrptFrameFragmentationHandlerState::ScheduledFragment
YieldingSrptFrameFragmentationHandlerState::takeLargeFragment() noexcept {
  if (!pinnedLarge_.has_value()) {
    pinnedLarge_.emplace(streams_.extractMin());
  }
  const uint32_t streamId = pinnedLarge_->stream.streamId;
  PerStreamState::Fragment fragment = takeFragment(*pinnedLarge_);
  if (!pinnedLarge_->stream.hasMore()) {
    pinnedLarge_.reset();
  }
  return ScheduledFragment{streamId, std::move(fragment)};
}

PerStreamState::Fragment
YieldingSrptFrameFragmentationHandlerState::takeFragment(
    StreamState& state) noexcept {
  CHECK(!state.stream.frames.empty());
  const size_t metadataSize = unsentMetadataSize(state.stream.frames.front());
  const size_t dataLimit =
      metadataSize >= streamQuantum_ ? 0 : streamQuantum_ - metadataSize;
  return state.stream.nextFragment(dataLimit);
}

void YieldingSrptFrameFragmentationHandlerState::spendCallbackBudget(
    size_t dataBytes) noexcept {
  if (dataBytes >= remainingCallbackBudget_) {
    remainingCallbackBudget_ = 0;
  } else {
    remainingCallbackBudget_ -= dataBytes;
  }
}

} // namespace apache::thrift::fast_thrift::frame::write::handler
