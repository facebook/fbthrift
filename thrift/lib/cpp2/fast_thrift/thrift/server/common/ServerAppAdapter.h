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
#include <ranges>
#include <string_view>
#include <utility>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/EndpointAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineImpl.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Messages.h>

namespace apache::thrift::fast_thrift::thrift {

enum class ServerRequestRoutingStatus {
  MissingMetadata,
  UnsupportedRpcKind,
  Ready,
};

struct ServerRequestRoutingMetadata {
  std::string_view methodName;
  apache::thrift::RpcKind kind{static_cast<apache::thrift::RpcKind>(-1)};
  ServerRequestRoutingStatus status{
      ServerRequestRoutingStatus::MissingMetadata};
};

inline ServerRequestRoutingMetadata getServerRequestRoutingMetadata(
    const ThriftServerRequestMessage& request) noexcept {
  const auto* metadata = request.payload.getRequestRpcMetadata();
  if (metadata == nullptr) {
    return {};
  }
  const auto kind =
      metadata->kind().value_or(static_cast<apache::thrift::RpcKind>(-1));
  if (kind != apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE) {
    return {{}, kind, ServerRequestRoutingStatus::UnsupportedRpcKind};
  }
  auto name = metadata->name();
  return {
      name.has_value() ? name->view() : std::string_view{},
      kind,
      ServerRequestRoutingStatus::Ready};
}

/**
 * ServerInboundAppAdapter — server-side inbound endpoint concept. The
 * pipeline calls these methods on an adapter when a request arrives.
 * Mirrors ClientInboundAppAdapter on the client side.
 */
template <typename T>
concept ServerInboundAppAdapter = channel_pipeline::TailEndpointHandler<T>;

/**
 * ServerOutboundAppAdapter — server-side outbound endpoint concept.
 * Codegen builds a ThriftServerResponseMessage via the helpers in
 * util/ResponsePayloads.h and hands it to writeResponse(). The adapter
 * also owns the close-initiation API: close() fires a
 * ThriftServerCloseConnectionEvent through the pipeline so the
 * resident ThriftServerConnectionCloseHandler can drive the terminal
 * state machine. Mirrors ClientOutboundAppAdapter on the client side.
 */
template <typename T>
concept ServerOutboundAppAdapter =
    requires(T& t, ThriftServerResponseMessage&& message) {
      { t.writeResponse(std::move(message)) } noexcept -> std::same_as<void>;
      { t.close() } noexcept;
    };

/**
 * ServerComposableAppAdapter — wiring + routing-metadata concept.
 * An adapter satisfies this when it can be plugged into a routing fabric
 * (e.g. ThriftServerCompositeAppAdapter): it publishes the methods it owns
 * and accepts a pipeline pointer at setup time. Orthogonal to
 * inbound/outbound data flow — purely about composition wiring.
 *
 * methodTable() pairs each owned method name with a resolved method containing
 * the owning adapter and its typed process function. The routing fabric stores
 * that value beside the name, so the name is hashed once for the whole request
 * instead of once per layer.
 *
 * Each resolved method binds its owning adapter to the process function and
 * exposes the resolved-dispatch entry point. This is deliberately distinct
 * from the adapter's unresolved onRead required by channel_pipeline's
 * TailEndpointHandler.
 */
template <typename T>
concept ServerComposableAppAdapter = requires(
    T& t,
    channel_pipeline::detail::ContextImpl& ctx,
    channel_pipeline::TypeErasedBox&& msg,
    typename T::ResolvedMethod method,
    channel_pipeline::PipelineImpl* pipe) {
  { t.methodTable() } -> std::ranges::range;
  {
    method.onRead(ctx, std::move(msg))
  } noexcept -> std::same_as<channel_pipeline::Result>;
  { t.setPipeline(pipe) } noexcept;
};

} // namespace apache::thrift::fast_thrift::thrift
