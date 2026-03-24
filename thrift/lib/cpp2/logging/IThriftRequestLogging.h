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

#include <chrono>
#include <cstdint>
#include <string_view>

#include <thrift/lib/cpp2/logging/ThriftEvent.h>

namespace apache::thrift {

/**
 * Pure virtual interface for completion-time structured logging of Thrift
 * stream/sink requests.
 *
 * Unlike IThriftServerCounters (called on every event), this interface is
 * called once per stream/sink at termination with accumulated summary data.
 * Implementations can emit structured log records, scuba rows, etc.
 *
 * All methods default to noop so that partial implementations are possible.
 */
class IThriftRequestLogging {
 public:
  virtual ~IThriftRequestLogging() = default;

  /**
   * Summary of a completed stream's lifecycle.
   */
  struct StreamSummary {
    std::string_view methodName;
    detail::StreamEndReason endReason;
    uint64_t chunksGenerated;
    uint64_t chunksSent;
    uint32_t totalCreditsReceived;
    uint32_t totalPauseEvents;
    std::chrono::milliseconds totalPauseDuration;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    uint64_t totalBytes{0};
    uint64_t minChunkSize{0};
    uint64_t maxChunkSize{0};
  };

  /**
   * Summary of a completed sink's lifecycle.
   */
  struct SinkSummary {
    std::string_view methodName;
    detail::SinkEndReason endReason;
    uint64_t chunksReceived;
    uint64_t chunksConsumed;
    uint32_t totalCreditsSent;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    uint64_t totalBytes{0};
    uint64_t minChunkSize{0};
    uint64_t maxChunkSize{0};
  };

  virtual void onStreamComplete(const StreamSummary& /*summary*/) {}

  virtual void onSinkComplete(const SinkSummary& /*summary*/) {}
};

} // namespace apache::thrift
