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
#include <cstdint>
#include <utility>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>

namespace apache::thrift::fast_thrift::transport {

/**
 * Outcome of a single socket-level writeChain on TransportHandler.
 * Carried in the per-write event a WriteCompleteEventFactory produces.
 */
enum class WriteCompletionStatus : uint8_t {
  Success,
  Error,
};

struct NoOpWriteCompleteEvent : channel_pipeline::EventTag<> {};

/** Factory for the typed write-completion event emitted by TransportHandler. */
template <typename T>
concept WriteCompleteEventFactory =
    channel_pipeline::PipelineEvent<
        typename T::TransportWriteCompleteEventType> &&
    channel_pipeline::kIsEventSet<typename T::PublishedEvents> &&
    requires(WriteCompletionStatus status, size_t bytes) {
      {
        T::make(status, bytes)
      } noexcept -> std::same_as<typename T::TransportWriteCompleteEventType>;
    };

/** Compile-time-elided default factory. */
struct NoOpWriteCompleteEventFactory {
  using TransportWriteCompleteEventType = NoOpWriteCompleteEvent;
  using PublishedEvents = channel_pipeline::Events<>;

  static NoOpWriteCompleteEvent make(WriteCompletionStatus, size_t) noexcept {
    return {};
  }
};

static_assert(
    WriteCompleteEventFactory<NoOpWriteCompleteEventFactory>,
    "NoOpWriteCompleteEventFactory must satisfy WriteCompleteEventFactory concept");

} // namespace apache::thrift::fast_thrift::transport
