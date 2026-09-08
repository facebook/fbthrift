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
#include <optional>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/ConnectionPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/transport/WriteCompletion.h>

namespace apache::thrift::fast_thrift::thrift {

/** Published once the close state machine has fully settled. */
struct ThriftServerConnectionClosedEvent : channel_pipeline::EventTag<> {};

/**
 * `ThriftServerCloseConnectionEvent` carries what the client is told
 * on the way out. Empty means an orderly shutdown, answered with
 * CONNECTION_CLOSE; a rejection means the connection is refused, answered with
 * that error code and reason.
 */
struct ThriftServerCloseConnectionEvent
    : channel_pipeline::EventTag<ThriftServerCloseConnectionEvent> {
  std::optional<SetupRejection> rejection;
};

/**
 * `ThriftServerWriteCompleteEvent` reports one outbound rocket frame
 * reached the socket, relayed up from the rocket pipeline. See
 * RocketWriteCompleteEvent; relayed through unchanged.
 */
struct ThriftServerWriteCompleteEvent
    : channel_pipeline::EventTag<ThriftServerWriteCompleteEvent> {
  uint32_t streamId;
  apache::thrift::fast_thrift::transport::WriteCompletionStatus status;
  // Whether this completion left the connection's egress idle. See
  // RocketWriteCompleteEvent::quiesced — relayed through unchanged.
  bool quiesced;
};

/**
 * `ThriftServerSetupCompleteEvent` is fired by pointer so a
 * subscriber can fill the out-slot in place; the emitter reads it back once
 * dispatch returns.
 *
 * Refusing here is a different act from refusing during the setup exchange:
 * the client has already been answered, so it sees an error frame following
 * the SETUP response rather than instead of it.
 */
struct ThriftServerSetupCompleteEvent
    : channel_pipeline::EventTag<ThriftServerSetupCompleteEvent*> {
  std::optional<SetupRejection> reject;
};

} // namespace apache::thrift::fast_thrift::thrift
