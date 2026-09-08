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
#include <memory>
#include <optional>
#include <string>

#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>
#include <thrift/lib/cpp2/fast_thrift/frame/ErrorCode.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/SetupParameters.h>
#include <thrift/lib/cpp2/fast_thrift/transport/WriteCompletion.h>

namespace apache::thrift::fast_thrift::rocket::server {

/**
 * `TransportWriteCompleteEvent` reports the outcome of one
 * socket-level writev.
 */
struct TransportWriteCompleteEvent
    : channel_pipeline::EventTag<TransportWriteCompleteEvent> {
  apache::thrift::fast_thrift::transport::WriteCompletionStatus status;
  size_t bytes;
};

/**
 * `BatchWriteCompleteEvent` reports the completion of one
 * batch as the batcher saw it. `frameCount` is the number of rocket frames in
 * that batch (> 0).
 */
struct BatchWriteCompleteEvent
    : channel_pipeline::EventTag<BatchWriteCompleteEvent> {
  apache::thrift::fast_thrift::transport::WriteCompletionStatus status;
  size_t frameCount;
  size_t bytes;
  // Whether this completion left the connection's egress idle: nothing further
  // handed to the socket, and nothing buffered awaiting a flush. Intended for
  // per-connection admission control, as the point at which the server has
  // caught up on what it owed this client; nothing consumes it yet. Reported
  // only as an edge on a completion, and a connection torn down mid-write never
  // reports it, so a consumer needs its own teardown path.
  bool quiesced;
};

/**
 * `FrameWriteCompleteEvent` reports the completion of one
 * original outbound frame, whether it went out whole or in fragments. The
 * streamId is the one the fragmentation handler recorded at write time: below
 * it the frame is serialized bytes and the stream is no longer recoverable.
 */
struct FrameWriteCompleteEvent
    : channel_pipeline::EventTag<FrameWriteCompleteEvent> {
  uint32_t streamId;
  apache::thrift::fast_thrift::transport::WriteCompletionStatus status;
  // See BatchWriteCompleteEvent::quiesced, narrowed by whatever the
  // fragmentation handler was still holding back. Set on at most one frame per
  // batch — it describes the connection, not this frame.
  bool quiesced;
};

/**
 * `RocketWriteCompleteEvent` reports one outbound rocket
 * frame reached the socket. The streamId identifies it; unlike the client there
 * is no request context to resolve against, because the server's is carried on
 * the request message rather than held in a per-stream table.
 */
struct RocketWriteCompleteEvent
    : channel_pipeline::EventTag<RocketWriteCompleteEvent> {
  uint32_t streamId;
  apache::thrift::fast_thrift::transport::WriteCompletionStatus status;
  // See FrameWriteCompleteEvent::quiesced — relayed through unchanged.
  bool quiesced;
};

/**
 * A refusal of the setup exchange: the frame-layer code to close with, and a
 * human-readable reason for it. The reason reaches the client in the ERROR
 * frame's body, so whoever refuses can say why — the rocket layer never has to
 * understand it, it only carries it.
 */
struct SetupRejection {
  apache::thrift::fast_thrift::frame::ErrorCode code;
  std::string reason;
};

/**
 * Payload carried by `SetupReceivedEvent` for a validated SETUP frame and
 * the slots for the answer to it.
 *
 * This event is a question, not a notification: the emitter reads the response
 * slots back once dispatch returns, so a subscriber fills them in place rather
 * than replying through some other channel. Exactly one of `metadataPush` and
 * `reject` should be set; leaving both empty accepts the connection without
 * pushing anything back, which is what an unsubscribed pipeline does.
 *
 * The event stays thrift-free: only opaque bytes and a frame-layer error code
 * cross the rocket boundary, so interpreting the setup metadata remains the
 * upper layer's business.
 */
struct RocketSetupEvent {
  // In: parameters read off the frame, and the client's opaque setup metadata
  // (null when the frame carried none).
  const SetupParameters* params{nullptr};
  std::unique_ptr<folly::IOBuf> metadata;
  // Out: the response to push back to the client, or the refusal to close with.
  std::unique_ptr<folly::IOBuf> metadataPush;
  std::optional<SetupRejection> reject;
};

/**
 * Payload carried by `SetupCompleteEvent` for the second decision point,
 * once the SETUP response is on the write path and before any request can be
 * dispatched. `reject` is an out-slot: setting it refuses a connection that
 * has already been answered, which the client sees as an error frame following
 * the setup response.
 */
struct RocketSetupCompleteEvent {
  std::optional<SetupRejection> reject;
};

/** Carries the mutable setup request and response slots to subscribers. */
struct SetupReceivedEvent : channel_pipeline::EventTag<RocketSetupEvent*> {};
/** Carries the post-response setup decision to subscribers. */
struct SetupCompleteEvent
    : channel_pipeline::EventTag<RocketSetupCompleteEvent*> {};

/** Signals that buffered frames must reach the socket before teardown. */
struct FlushWritesEvent : channel_pipeline::EventTag<> {};

} // namespace apache::thrift::fast_thrift::rocket::server
