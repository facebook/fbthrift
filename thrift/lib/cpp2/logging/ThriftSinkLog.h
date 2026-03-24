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

#include <atomic>
#include <chrono>
#include <limits>
#include <string_view>

#include <thrift/lib/cpp2/logging/IThriftRequestLogging.h>
#include <thrift/lib/cpp2/logging/IThriftServerCounters.h>
#include <thrift/lib/cpp2/logging/ThriftEvent.h>

namespace apache::thrift {

/**
 * Accumulates state for a single server-side sink's lifecycle.
 *
 * Mirrors ThriftStreamLog for sink operations: tracks chunk receive intervals,
 * consume latency (sampled), credit-based approximate pause detection, and
 * dispatches to IThriftServerCounters / IThriftRequestLogging backends.
 *
 * Threading contract: log(SinkSubscribeEvent) and log(SinkConsumedEvent) are
 * called from the server executor (CPU) thread. log(SinkNextEvent),
 * log(SinkCreditEvent), log(SinkCancelEvent), and log(SinkCompleteEvent) are
 * called from the IO thread. Cross-thread coordination for consume-latency
 * sampling uses sampledChunkNumber_ (atomic) with acquire/release ordering
 * to safely publish sampledChunkReceivedTime_ from the IO thread to the
 * executor thread.
 */
class ThriftSinkLog {
 public:
  ThriftSinkLog(
      std::string_view methodName,
      IThriftServerCounters* counters,
      IThriftRequestLogging* logging);

  ~ThriftSinkLog();

  ThriftSinkLog(const ThriftSinkLog&) = delete;
  ThriftSinkLog& operator=(const ThriftSinkLog&) = delete;
  ThriftSinkLog(ThriftSinkLog&&) = delete;
  ThriftSinkLog& operator=(ThriftSinkLog&&) = delete;

  void log(const detail::SinkSubscribeEvent& event);
  void log(const detail::SinkNextEvent& event);
  void log(const detail::SinkConsumedEvent& event);
  void log(const detail::SinkCancelEvent& event);
  void log(const detail::SinkCreditEvent& event);
  void log(const detail::SinkCompleteEvent& event);

 private:
  static constexpr uint64_t kConsumeLatencySampleRate = 10;

  void finish(detail::SinkEndReason reason);

  std::string_view methodName_;
  IThriftServerCounters* counters_;
  IThriftRequestLogging* logging_;

  std::atomic<bool> finished_{false};
  std::chrono::steady_clock::time_point startTime_;

  // Receive interval tracking
  bool isFirstChunk_{true};
  std::chrono::steady_clock::time_point lastChunkReceivedTime_;

  // Consume latency tracking (sampled)
  std::atomic<uint64_t> receivedCounter_{0};
  std::atomic<uint64_t> consumedCounter_{0};
  std::atomic<uint64_t> sampledChunkNumber_{0};
  std::chrono::steady_clock::time_point sampledChunkReceivedTime_;

  // Byte tracking
  uint64_t totalBytes_{0};
  uint64_t minChunkSize_{std::numeric_limits<uint64_t>::max()};
  uint64_t maxChunkSize_{0};

  // Credit tracking (for approx pause detection)
  uint32_t creditsAvailable_{0};
  uint32_t totalCreditsSent_{0};
  bool clientPaused_{false};
  std::chrono::steady_clock::time_point pauseStartTime_;
};

} // namespace apache::thrift
