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

#include <thrift/lib/cpp2/logging/ThriftSinkLog.h>

#include <algorithm>
#include <utility>

namespace apache::thrift {

ThriftSinkLog::ThriftSinkLog(
    std::string_view methodName,
    IThriftServerCounters* counters,
    IThriftRequestLogging* logging)
    : methodName_(methodName), counters_(counters), logging_(logging) {}

ThriftSinkLog::~ThriftSinkLog() {
  if (!finished_.load(std::memory_order_acquire)) {
    finish(detail::SinkEndReason::ERROR);
  }
}

void ThriftSinkLog::log(const detail::SinkSubscribeEvent& /*event*/) {
  startTime_ = std::chrono::steady_clock::now();
  if (counters_) {
    counters_->onSinkSubscribe(methodName_);
  }
}

void ThriftSinkLog::log(const detail::SinkNextEvent& event) {
  auto now = std::chrono::steady_clock::now();

  if (counters_) {
    counters_->onSinkNext(methodName_);
  }

  totalBytes_ += event.payloadBytes;
  if (event.payloadBytes > 0) {
    minChunkSize_ = std::min(minChunkSize_, event.payloadBytes);
    maxChunkSize_ = std::max(maxChunkSize_, event.payloadBytes);
  }

  // Track receive interval
  const bool isFirst = std::exchange(isFirstChunk_, false);
  if (!isFirst) {
    auto intervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastChunkReceivedTime_);
    if (counters_) {
      counters_->onSinkReceiveInterval(methodName_, intervalMs);
    }
  }
  lastChunkReceivedTime_ = now;

  // Track credits for approximate pause detection
  if (creditsAvailable_ > 0) {
    --creditsAvailable_;
  }
  if (creditsAvailable_ == 0 && !clientPaused_) {
    clientPaused_ = true;
    pauseStartTime_ = now;
  }

  // Sample every Kth chunk for consume latency tracking.
  // Memory ordering: sampledChunkReceivedTime_ must be written BEFORE the
  // release-store to sampledChunkNumber_. The executor thread's acquire-load
  // of sampledChunkNumber_ then establishes happens-before, making the
  // time_point visible. Do not reorder these writes.
  auto received = receivedCounter_.fetch_add(1, std::memory_order_relaxed) + 1;
  if (received % kConsumeLatencySampleRate == 1 &&
      sampledChunkNumber_.load(std::memory_order_acquire) == 0) {
    sampledChunkReceivedTime_ = now;
    sampledChunkNumber_.store(received, std::memory_order_release);
  }
}

void ThriftSinkLog::log(const detail::SinkConsumedEvent& /*event*/) {
  if (counters_) {
    counters_->onSinkConsumed(methodName_);
  }

  auto consumed = consumedCounter_.fetch_add(1, std::memory_order_relaxed) + 1;
  auto sampledNum = sampledChunkNumber_.load(std::memory_order_acquire);
  if (consumed == sampledNum && sampledNum != 0) {
    auto delayMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - sampledChunkReceivedTime_);
    if (counters_) {
      counters_->onSinkConsumeLatency(methodName_, delayMs);
    }
    sampledChunkNumber_.store(0, std::memory_order_release);
  }
}

void ThriftSinkLog::log(const detail::SinkCancelEvent& /*event*/) {
  if (counters_) {
    counters_->onSinkCancel(methodName_);
  }
}

void ThriftSinkLog::log(const detail::SinkCreditEvent& event) {
  totalCreditsSent_ += event.credits;
  if (counters_) {
    counters_->onSinkCredit(methodName_, event.credits);
  }

  // If client was paused (credits exhausted), record approximate pause duration
  if (clientPaused_ && event.credits > 0) {
    auto pauseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - pauseStartTime_);
    if (counters_) {
      counters_->onSinkApproxPausedDuration(methodName_, pauseDuration);
    }
    clientPaused_ = false;
  }

  creditsAvailable_ += event.credits;
}

void ThriftSinkLog::log(const detail::SinkCompleteEvent& event) {
  finish(event.reason);
}

void ThriftSinkLog::finish(detail::SinkEndReason reason) {
  if (finished_.exchange(true, std::memory_order_release)) {
    return;
  }

  if (counters_) {
    counters_->onSinkComplete(methodName_, reason);
  }

  if (logging_) {
    IThriftRequestLogging::SinkSummary summary;
    summary.methodName = methodName_;
    summary.endReason = reason;
    summary.chunksReceived = receivedCounter_.load(std::memory_order_relaxed);
    summary.chunksConsumed = consumedCounter_.load(std::memory_order_relaxed);
    summary.totalCreditsSent = totalCreditsSent_;
    summary.startTime = startTime_;
    summary.endTime = std::chrono::steady_clock::now();
    summary.totalBytes = totalBytes_;
    summary.minChunkSize = (maxChunkSize_ > 0) ? minChunkSize_ : 0;
    summary.maxChunkSize = maxChunkSize_;
    logging_->onSinkComplete(summary);
  }
}

} // namespace apache::thrift
