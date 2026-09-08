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
#include <cstddef>
#include <cstdint>
#include <utility>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/client/Event.h>
#include <thrift/lib/cpp2/fast_thrift/transport/WriteCompletion.h>

namespace apache::thrift::fast_thrift::rocket::client {

/**
 * Per-pipeline event factory for the rocket-client pipeline.
 *
 * `make(status, bytes)` satisfies the WriteCompleteEventFactory concept used
 * by TransportHandlerT — produces a TransportWriteComplete event per writev.
 *
 * `makeBatchWriteComplete(status, frameCount, bytes, quiesced)` is used by
 * WriteCompletionTrackerT to fire the enriched per-rocket-batch event upstream
 * after popping its frame-count FIFO.
 */
struct RocketClientEventFactory {
  using TransportWriteCompleteEventType = TransportWriteCompleteEvent;
  using BatchWriteCompleteEventType = BatchWriteCompleteEvent;
  using FrameWriteCompleteEventType = FrameWriteCompleteEvent;
  using FirstResponseFrameEventType = FirstResponseFrameEvent;
  using FlushWritesEventType = void;
  using PublishedEvents =
      channel_pipeline::Events<TransportWriteCompleteEventType>;

  static TransportWriteCompleteEvent make(
      transport::WriteCompletionStatus status, size_t bytes) noexcept {
    return {.status = status, .bytes = bytes};
  }

  static BatchWriteCompleteEvent makeBatchWriteComplete(
      transport::WriteCompletionStatus status,
      size_t frameCount,
      size_t bytes,
      bool quiesced) noexcept {
    return {
        .status = status,
        .frameCount = frameCount,
        .bytes = bytes,
        .quiesced = quiesced,
    };
  }

  static FrameWriteCompleteEvent makeFrameWriteComplete(
      transport::WriteCompletionStatus status,
      uint32_t streamId,
      bool quiesced) noexcept {
    return {
        .streamId = streamId,
        .status = status,
        .quiesced = quiesced,
    };
  }

  static FirstResponseFrameEvent makeFirstResponseFrame(
      uint32_t streamId,
      std::chrono::steady_clock::time_point arrivalTime) noexcept {
    return {.streamId = streamId, .arrivalTime = arrivalTime};
  }
};

static_assert(
    apache::thrift::fast_thrift::transport::WriteCompleteEventFactory<
        RocketClientEventFactory>,
    "RocketClientEventFactory must satisfy WriteCompleteEventFactory concept");

} // namespace apache::thrift::fast_thrift::rocket::client
