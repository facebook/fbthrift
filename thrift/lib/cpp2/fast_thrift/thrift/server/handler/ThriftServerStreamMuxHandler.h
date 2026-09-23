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

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <fmt/core.h>
#include <glog/logging.h>
#include <folly/ExceptionWrapper.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/ContextImpl.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/DirectStreamMap.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/StreamPayloadMetadata.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftControlPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/StreamResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/ResponseMetadata.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/ProducerPipeline.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>

namespace apache::thrift::fast_thrift::thrift {

/**
 * ThriftServerStreamMuxHandler — a duplex handler in the thrift server pipeline
 * that owns and multiplexes per-stream producing-end sub-pipelines.
 *
 * Placement: just head-ward of the tail app adapter. Outbound stream chunks it
 * injects then traverse every outbound handler (checksum/compression/transport)
 * like a unary response; inbound RequestN/Cancel reach it after the upper
 * handlers and are routed here rather than forwarded to the app.
 *
 * Outbound (onWrite): a `ThriftServerStreamOpenPayload` opens a stream — the
 * mux runs its `ProducerPipeline::ConfigFunc` to build and own a sub-pipeline
 * (keyed by streamId), wires the sub-pipeline head's sink back onto this
 * pipeline's write path, and forwards the initial response chunk upward. Every
 * other outbound message passes through.
 *
 * Inbound (onRead): `RequestN`/`Cancel` are looked up by streamId and injected
 * into the matching sub-pipeline (not forwarded to the app); unknown/terminated
 * streams drop the frame. Unary requests pass through.
 *
 * Teardown: a terminal (`Complete`/`Error` out, or `Cancel` in) surfaces while
 * we are executing inside the sub-pipeline's `fireRead` —
 * `fireRead`/`fireWrite` hold no DestructorGuard, so erasing the entry there
 * would destroy the pipeline mid-call. The terminal only marks the streamId for
 * removal; the mux erases marked entries after the driving `onRead` returns.
 * (Loop-deferred teardown for producers that emit terminals asynchronously —
 * outside an inbound drive — is a follow-up; today's producers emit
 * synchronously under demand.)
 */
template <typename Context>
class ThriftServerStreamMuxHandler {
 public:
  void handlerAdded(Context& ctx) noexcept {
    channel_pipeline::detail::ContextImpl& base = ctx;
    mainCtx_ = &base;
  }
  void handlerRemoved(Context& /*ctx*/) noexcept {}
  void onPipelineActive(Context& /*ctx*/) noexcept {}
  void onReadReady(Context& /*ctx*/) noexcept {}
  void onWriteReady(Context& /*ctx*/) noexcept {}

  void onPipelineInactive(Context& /*ctx*/) noexcept {
    // No drive is in flight at deactivation, so tearing every sub-pipeline down
    // directly is safe. DelayedDestruction defers any child-internal cleanup.
    streams_.clear();
  }

  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }

  channel_pipeline::Result onWrite(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    auto& response = msg.get<ThriftServerResponseMessage>();
    if (!response.payload.is<ThriftServerStreamOpenPayload>()) {
      return ctx.fireWrite(std::move(msg));
    }
    auto& open = response.payload.get<ThriftServerStreamOpenPayload>();
    const uint32_t streamId = open.initialResponse.streamId;
    auto configFunc = std::move(open.configFunc);
    auto initialResponse = std::move(open.initialResponse);
    openStream(streamId, std::move(configFunc));
    // Forward the initial chunk upward as an ordinary stream response.
    response.payload = std::move(initialResponse);
    return ctx.fireWrite(std::move(msg));
  }

  channel_pipeline::Result onRead(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    auto& request = msg.get<ThriftServerRequestMessage>();
    if (request.payload.is<ThriftRequestNPayload>()) {
      const uint32_t n = request.payload.get<ThriftRequestNPayload>().requestN;
      const bool routed = routeInbound(
          request.streamId,
          stream::ThriftStreamMessage{.payload = stream::RequestN{.n = n}});
      drainPendingRemoval();
      if (FOLLY_UNLIKELY(!routed)) {
        return fireUnknownStream(ctx, request.streamId);
      }
      return channel_pipeline::Result::Success;
    }
    if (request.payload.is<ThriftCancelPayload>()) {
      const bool routed = routeInbound(
          request.streamId,
          stream::ThriftStreamMessage{.payload = stream::Cancel{}});
      markTerminated(request.streamId);
      drainPendingRemoval();
      if (FOLLY_UNLIKELY(!routed)) {
        return fireUnknownStream(ctx, request.streamId);
      }
      return channel_pipeline::Result::Success;
    }
    return ctx.fireRead(std::move(msg));
  }

 private:
  // One established stream's producing-end sub-pipeline, owned by the mux and
  // stored by value in `streams_`. No external raw pointer aims at the entry:
  // the head's sink captures (this, streamId) by value rather than the entry,
  // so the map is free to relocate entries on grow/erase. `ProducerPipeline` is
  // noexcept-movable behind stable heap pointers, so relocating an entry only
  // moves a few pointers while the pipeline it owns stays put. The
  // ProducerPipeline owns its endpoints and pipeline and tears them down in the
  // correct order internally.
  struct StreamEntry {
    std::optional<stream::ProducerPipeline> pipeline;
    bool terminated{false};
  };

  void openStream(
      uint32_t streamId,
      std::unique_ptr<stream::ProducerPipeline::ConfigFunc>
          configFunc) noexcept {
    // The framework wires the endpoints/allocator/executor and the head's
    // output sink; the configFunc only composes the application's producer
    // between them. The sink captures (this, streamId) by value — never the
    // entry — so the entry needs no stable address.
    stream::ProducerPipeline::Builder builder(
        mainCtx_->eventBase(),
        stream::StreamSink{
            [this, streamId](stream::ThriftStreamMessage&& msg) noexcept {
              return emitOutbound(streamId, std::move(msg));
            }});
    (*configFunc)(builder);
    // A live streamId is opened exactly once: the client assigns fresh ids and
    // a duplicate REQUEST_STREAM is expected to be rejected before it reaches
    // the mux. `emplace` is a no-op on collision, so a duplicate would silently
    // drop the freshly built sub-pipeline while the stale entry keeps driving —
    // a framework bug, not client input. Guard it in debug.
    const bool inserted =
        streams_.emplace(streamId, StreamEntry{.pipeline = builder.build()})
            .second;
    DCHECK(inserted) << "duplicate stream id " << streamId;
  }

  // Route an inbound flow-control message into the matching sub-pipeline.
  // Returns false when no active stream owns `streamId` (unknown, or already
  // terminated) — the caller surfaces that as an error rather than dropping it
  // silently.
  bool routeInbound(
      uint32_t streamId, stream::ThriftStreamMessage&& msg) noexcept {
    auto* slot = streams_.find(streamId);
    if (slot == nullptr || slot->second.terminated) {
      return false;
    }
    (void)slot->second.pipeline->fireRead(
        channel_pipeline::erase_and_box(std::move(msg)));
    return true;
  }

  // A RequestN/Cancel arrived for a streamId this mux has no active stream for
  // (never opened, or already torn down). Rather than drop it silently, surface
  // it up the pipeline so the connection sees the protocol violation.
  channel_pipeline::Result fireUnknownStream(
      Context& ctx, uint32_t streamId) noexcept {
    ctx.fireException(
        folly::make_exception_wrapper<apache::thrift::TApplicationException>(
            apache::thrift::TApplicationException::PROTOCOL_ERROR,
            fmt::format(
                "stream flow-control frame for unknown stream {}", streamId)));
    return channel_pipeline::Result::Success;
  }

  // Convert a producer-emitted stream message to an outbound response chunk and
  // inject it on this pipeline's write path (head-ward from the mux). Marks the
  // stream for teardown on a terminal.
  channel_pipeline::Result emitOutbound(
      uint32_t streamId, stream::ThriftStreamMessage&& msg) noexcept {
    ThriftServerResponseMessage response;
    bool terminal = false;
    auto& payload = msg.payload;
    if (payload.is<stream::Payload>()) {
      response.payload = ThriftStreamPayload{
          .data = std::move(payload.get<stream::Payload>().data),
          .streamId = streamId,
          .complete = false,
          .next = true};
    } else if (payload.is<stream::Complete>()) {
      response.payload = ThriftStreamPayload{
          .streamId = streamId, .complete = true, .next = false};
      terminal = true;
    } else if (payload.is<stream::Error>()) {
      // Match regular Thrift's undeclared-exception encoding: a terminal
      // PAYLOAD chunk carrying appUnknownException metadata (name/what/blame),
      // not an rsocket ERROR frame (that channel is for framework/connection
      // errors). The mux is method-generic, so every producer exception is
      // surfaced as undeclared; declared-exception fidelity needs the
      // per-method producing-end adapter and is a follow-up.
      auto& ex = payload.get<stream::Error>().ex;
      auto metadata = std::make_unique<apache::thrift::StreamPayloadMetadata>();
      metadata->payloadMetadata() = buildAppUnknownExceptionPayloadMetadata(
          ex.class_name().toStdString(), ex.what().toStdString());
      response.payload = ThriftStreamPayload{
          .metadata = std::move(metadata),
          .streamId = streamId,
          .complete = true,
          .next = true};
      terminal = true;
    } else {
      // RequestN/Cancel never leave the producer head (guarded there).
      return channel_pipeline::Result::Success;
    }
    auto result = mainCtx_->fireWrite(
        channel_pipeline::erase_and_box(std::move(response)));
    if (terminal) {
      markTerminated(streamId);
    }
    return result;
  }

  void markTerminated(uint32_t streamId) noexcept {
    auto* slot = streams_.find(streamId);
    if (slot == nullptr || slot->second.terminated) {
      return;
    }
    slot->second.terminated = true;
    pendingRemoval_.push_back(streamId);
  }

  // Erase entries marked terminal during the just-finished inbound drive. Safe
  // here: the sub-pipeline's fireRead has returned, so no entry is on the stack
  // and the erase's backshift can freely relocate the remaining entries.
  void drainPendingRemoval() noexcept {
    for (uint32_t streamId : pendingRemoval_) {
      streams_.erase(streamId);
    }
    pendingRemoval_.clear();
  }

  channel_pipeline::detail::ContextImpl* mainCtx_{nullptr};
  frame::read::DirectStreamMap<StreamEntry> streams_;
  std::vector<uint32_t> pendingRemoval_;
};

} // namespace apache::thrift::fast_thrift::thrift
