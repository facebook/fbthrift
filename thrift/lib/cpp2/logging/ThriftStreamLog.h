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
#include <string_view>

#include <thrift/lib/cpp2/logging/IThriftRequestLogging.h>
#include <thrift/lib/cpp2/logging/IThriftServerCounters.h>
#include <thrift/lib/cpp2/logging/ThriftEvent.h>

namespace apache::thrift {

/**
 * Accumulates state for a single server-side stream's lifecycle.
 *
 * Each log() overload updates local counters and dispatches to the injected
 * IThriftServerCounters for real-time metrics. On terminal state, dispatches
 * to IThriftRequestLogging for structured logging.
 *
 * Threading contract: all log() calls except log(StreamNextSentEvent) must
 * be serialized on the server executor thread. Only log(StreamNextSentEvent)
 * may be called from the IO thread. Cross-thread coordination for send-delay
 * sampling uses sampledChunkNumber_ (atomic) with acquire/release ordering
 * to safely publish sampledChunkGeneratedTime_ from the executor thread to
 * the IO thread. finished_ uses release (in finish()) / acquire (in
 * destructor) ordering to synchronize non-atomic summary fields when
 * destruction occurs on a different thread than finish().
 * sentChunkCounter_ is also atomic to avoid data races with the IO thread.
 */
class ThriftStreamLog {
 public:
  ThriftStreamLog(
      std::string_view methodName,
      IThriftServerCounters* counters,
      IThriftRequestLogging* logging);

  ~ThriftStreamLog();

  ThriftStreamLog(const ThriftStreamLog&) = delete;
  ThriftStreamLog& operator=(const ThriftStreamLog&) = delete;
  ThriftStreamLog(ThriftStreamLog&&) = delete;
  ThriftStreamLog& operator=(ThriftStreamLog&&) = delete;

  void log(const detail::StreamSubscribeEvent& event);
  void log(const detail::StreamNextEvent& event);
  void log(const detail::StreamNextSentEvent& event);
  void log(const detail::StreamCreditEvent& event);
  void log(const detail::StreamPauseEvent& event);
  void log(const detail::StreamResumeEvent& event);
  void log(const detail::StreamCompleteEvent& event);

 private:
  static constexpr uint64_t kSendDelaySampleInterval = 10;

  void handleResume();
  void finish(detail::StreamEndReason reason);

  std::string_view methodName_;
  IThriftServerCounters* counters_;
  IThriftRequestLogging* logging_;

  std::atomic<bool> finished_{false};
  std::chrono::steady_clock::time_point startTime_;

  // Stream chunk tracking
  bool isFirstChunk_{true};
  std::chrono::steady_clock::time_point lastChunkGeneratedTime_;

  // Credits tracking
  uint32_t credits_{0};
  uint32_t totalCreditsReceived_{0};

  // Pause tracking
  bool isPaused_{false};
  detail::StreamPauseReason pauseReason_{};
  std::chrono::steady_clock::time_point pauseStartTime_;
  uint32_t totalPauseEvents_{0};
  std::chrono::milliseconds totalPauseDuration_{0};
  std::chrono::milliseconds pauseDurationSinceLastChunk_{0};

  // Chunk counters
  uint64_t chunkCounter_{0};
  std::atomic<uint64_t> sentChunkCounter_{0};

  // Send delay sampling (cross-thread coordination)
  std::atomic<uint64_t> sampledChunkNumber_{0};
  std::chrono::steady_clock::time_point sampledChunkGeneratedTime_;
};

} // namespace apache::thrift
