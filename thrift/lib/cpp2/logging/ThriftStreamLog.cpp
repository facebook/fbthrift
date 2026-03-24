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

#include <thrift/lib/cpp2/logging/ThriftStreamLog.h>

#include <algorithm>
#include <utility>

namespace apache::thrift {

ThriftStreamLog::ThriftStreamLog(
    std::string_view methodName,
    IThriftServerCounters* counters,
    IThriftRequestLogging* logging)
    : methodName_(methodName), counters_(counters), logging_(logging) {}

ThriftStreamLog::~ThriftStreamLog() {
  if (!finished_.load(std::memory_order_acquire)) {
    finish(detail::StreamEndReason::ERROR);
  }
}

void ThriftStreamLog::log(const detail::StreamSubscribeEvent& /*event*/) {
  startTime_ = std::chrono::steady_clock::now();
  if (counters_) {
    counters_->onStreamSubscribe(methodName_);
  }
}

void ThriftStreamLog::log(const detail::StreamNextEvent& event) {
  auto now = std::chrono::steady_clock::now();

  if (counters_) {
    counters_->onStreamNext(methodName_);
  }

  totalBytes_ += event.payloadBytes;
  if (event.payloadBytes > 0) {
    minChunkSize_ = std::min(minChunkSize_, event.payloadBytes);
    maxChunkSize_ = std::max(maxChunkSize_, event.payloadBytes);
  }

  if (credits_ > 0) {
    --credits_;
  }

  const bool isFirst = std::exchange(isFirstChunk_, false);
  if (!isFirst) {
    auto intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastChunkGeneratedTime_);
    intervalMs -= pauseDurationSinceLastChunk_;
    intervalMs = std::max(intervalMs, std::chrono::milliseconds{0});
    if (counters_) {
      counters_->onStreamChunkGenerationInterval(methodName_, intervalMs);
    }
  }

  lastChunkGeneratedTime_ = now;
  pauseDurationSinceLastChunk_ = std::chrono::milliseconds{0};

  // Sample every Kth chunk for send delay tracking.
  // Memory ordering: sampledChunkGeneratedTime_ must be written BEFORE the
  // release-store to sampledChunkNumber_. The IO thread's acquire-load of
  // sampledChunkNumber_ then establishes happens-before, making the
  // time_point visible. Do not reorder these writes.
  ++chunkCounter_;
  if (chunkCounter_ % kSendDelaySampleInterval == 1 &&
      sampledChunkNumber_.load(std::memory_order_acquire) == 0) {
    sampledChunkGeneratedTime_ = now;
    sampledChunkNumber_.store(chunkCounter_, std::memory_order_release);
  }
}

void ThriftStreamLog::log(const detail::StreamNextSentEvent& /*event*/) {
  auto sentCount =
      sentChunkCounter_.fetch_add(1, std::memory_order_relaxed) + 1;
  auto sampledNum = sampledChunkNumber_.load(std::memory_order_acquire);
  if (sentCount == sampledNum) {
    auto delayMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - sampledChunkGeneratedTime_);
    if (counters_) {
      counters_->onStreamChunkSendDelay(methodName_, delayMs);
    }
    sampledChunkNumber_.store(0, std::memory_order_release);
  }
}

void ThriftStreamLog::log(const detail::StreamCreditEvent& event) {
  credits_ += event.credits;
  totalCreditsReceived_ += event.credits;
  if (counters_) {
    counters_->onStreamCredit(methodName_, event.credits, credits_);
  }
  if (event.credits > 0) {
    handleResume();
  }
}

void ThriftStreamLog::log(const detail::StreamPauseEvent& event) {
  if (!isPaused_) {
    isPaused_ = true;
    pauseReason_ = event.reason;
    pauseStartTime_ = std::chrono::steady_clock::now();
    ++totalPauseEvents_;
    if (counters_) {
      counters_->onStreamPause(methodName_, event.reason);
    }
  }
}

void ThriftStreamLog::log(const detail::StreamResumeEvent& /*event*/) {
  handleResume();
}

void ThriftStreamLog::log(const detail::StreamCompleteEvent& event) {
  finish(event.reason);
}

void ThriftStreamLog::handleResume() {
  if (!isPaused_) {
    return;
  }

  auto pauseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - pauseStartTime_);
  totalPauseDuration_ += pauseDuration;
  pauseDurationSinceLastChunk_ += pauseDuration;
  isPaused_ = false;

  if (counters_) {
    counters_->onStreamResume(methodName_, pauseReason_, pauseDuration);
  }
}

void ThriftStreamLog::finish(detail::StreamEndReason reason) {
  if (finished_.exchange(true, std::memory_order_release)) {
    return;
  }

  handleResume(); // Clean up if stream ends while paused

  if (counters_) {
    counters_->onStreamComplete(
        methodName_, reason, totalPauseEvents_, totalPauseDuration_);
  }

  if (logging_) {
    IThriftRequestLogging::StreamSummary summary;
    summary.methodName = methodName_;
    summary.endReason = reason;
    summary.chunksGenerated = chunkCounter_;
    summary.chunksSent = sentChunkCounter_.load(std::memory_order_acquire);
    summary.totalCreditsReceived = totalCreditsReceived_;
    summary.totalPauseEvents = totalPauseEvents_;
    summary.totalPauseDuration = totalPauseDuration_;
    summary.startTime = startTime_;
    summary.endTime = std::chrono::steady_clock::now();
    summary.totalBytes = totalBytes_;
    summary.minChunkSize = (maxChunkSize_ > 0) ? minChunkSize_ : 0;
    summary.maxChunkSize = maxChunkSize_;
    logging_->onStreamComplete(summary);
  }
}

} // namespace apache::thrift
