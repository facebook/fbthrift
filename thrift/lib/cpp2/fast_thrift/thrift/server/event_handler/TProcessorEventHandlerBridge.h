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
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/intrusive_ptr.hpp>

#include <glog/logging.h>

#include <folly/ExceptionWrapper.h>
#include <folly/container/F14Map.h>
#include <folly/io/async/Request.h>
#include <folly/lang/Exception.h>

#include <thrift/lib/cpp/TProcessorEventHandler.h>
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/common/allocator/EvbAllocator.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/ConnectionPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Event.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/MethodMetadata.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/ServerAppAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/event_handler/Cpp2ContextAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/event_handler/EventHandlerChain.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/ResponsePayloads.h>

namespace apache::thrift::fast_thrift::thrift::server {

/**
 * What the bridge drives, and on whose behalf. Built once and shared by every
 * connection, so nothing here changes after install.
 *
 * The handler lists are the ones an embedder would have installed on a
 * `ThriftServer`. Both may be empty; a bridge with no processor event handlers
 * forwards everything untouched.
 */
struct TProcessorEventHandlers {
  std::vector<std::shared_ptr<apache::thrift::TProcessorEventHandler>>
      processor;
  std::vector<std::shared_ptr<apache::thrift::server::TServerEventHandler>>
      server;
};

struct TProcessorEventHandlerBridgeConfig {
  // Shared rather than held by value: this config is copied into every
  // connection's handler, and one refcount is the whole cost of that copy.
  std::shared_ptr<const TProcessorEventHandlers> handlers;
  std::shared_ptr<const ThriftServerMethodMetadataRegistry> methodMetadata;
  PeerIdentityResolver identityResolver{nullptr};
};

namespace event_handler_detail {

inline std::uint32_t requestBytes(
    const ThriftServerRequestMessage& request) noexcept {
  if (!request.payload.is<ThriftRequestResponsePayload>()) {
    return 0;
  }
  const auto& rr = request.payload.get<ThriftRequestResponsePayload>();
  return rr.data == nullptr
      ? 0
      : static_cast<std::uint32_t>(rr.data->computeChainDataLength());
}

inline std::uint32_t payloadBytes(
    const std::unique_ptr<folly::IOBuf>& data) noexcept {
  return data == nullptr
      ? 0
      : static_cast<std::uint32_t>(data->computeChainDataLength());
}

inline bool isAppUnknownException(
    const ThriftInitialResponsePayload& reply) noexcept {
  if (reply.metadata == nullptr ||
      !reply.metadata->payloadMetadata().has_value()) {
    return false;
  }
  const auto& payloadMetadata = *reply.metadata->payloadMetadata();
  if (payloadMetadata.getType() !=
      apache::thrift::PayloadMetadata::Type::exceptionMetadata) {
    return false;
  }
  const auto& exceptionMetadata = *payloadMetadata.exceptionMetadata();
  return !exceptionMetadata.metadata().has_value() ||
      exceptionMetadata.metadata()->getType() ==
      apache::thrift::PayloadExceptionMetadata::Type::appUnknownException;
}

// Moves what the handlers wrote onto the outgoing reply. Only the normal reply
// carries metadata; an error frame has nowhere to put them.
inline void stampWriteHeaders(
    ThriftInitialResponsePayload& reply,
    apache::thrift::transport::THeader::StringToStringMap headers) {
  if (headers.empty() || reply.metadata == nullptr) {
    return;
  }
  auto& other = reply.metadata->otherMetadata().ensure();
  for (auto& [key, value] : headers) {
    other[key] = std::move(value);
  }
}

} // namespace event_handler_detail

/**
 * Runs `TProcessorEventHandler`s / `TServerEventHandler`s on the
 * fast_thrift pipeline.
 *
 * These handlers are written against the classic server: they expect Cpp2
 * contexts, a `THeader` to read and write, an ambient `folly::RequestContext`,
 * and callbacks in a fixed order around a request. The bridge supplies all of
 * that at its own boundary and hands the pipeline back its own message types
 * unchanged — see Cpp2ContextAdapter.h for what is and is not
 * translatable.
 *
 * Mapping, per connection:
 *   Setup request    -> TServerEventHandler::newConnection
 *   ConnectionClosed -> TServerEventHandler::connectionDestroyed
 *
 * and per request:
 *   onRead   -> preRead, onReadData, postRead across the chain, then forward
 *   onWrite  -> exception callbacks when applicable, then preWrite,
 *               onWriteData, postWrite, and forward
 *
 * A `preRead`/`postRead` that throws is the handler refusing the request: the
 * request is dropped, an application error carrying the exception's name and
 * message goes back to the client, and the write-side callbacks do NOT run —
 * matching the classic server, where an exception thrown before dispatch means
 * `postWrite` never fires. Any response headers the handler set before
 * throwing still reach the client.
 *
 * A write-side callback that throws terminates the process. The request has
 * been served by then, so there is no verdict left to honour and nothing to
 * turn the exception into; swallowing it would only hide the handler's bug.
 *
 * NOT supported, because fast_thrift has no equivalent: streams, sinks,
 * interactions (`onInteractionTerminate`), and the server-lifecycle callbacks
 * (`preStart` / `preServe` / `postStop`). A handler relying on any of those
 * will not see them.
 *
 * Requires `enableRequestHeaders` if any handler reads request headers. A
 * request arriving without a context from a manually composed pipeline is
 * refused rather than forwarded: the bridge cannot tell an authorization
 * handler from a logging one, so quietly skipping them is not safe.
 *
 * Every callback runs synchronously on the connection's EventBase. That is
 * load-bearing, not incidental: the pipeline context does not guard against a
 * closed pipeline, so a callback that deferred work and wrote later would be a
 * use-after-free.
 */
template <typename Context>
class TProcessorEventHandlerBridge {
 public:
  explicit TProcessorEventHandlerBridge(
      TProcessorEventHandlerBridgeConfig config)
      : config_(std::move(config)),
        handlers_(config_.handlers.get()),
        methodMetadata_(config_.methodMetadata.get()),
        drivesProcessorHandlers_(
            handlers_ != nullptr && !handlers_->processor.empty()) {
    if (drivesProcessorHandlers_) {
      // A pipelining client's steady-state depth. Deliberately small: a host
      // carries these by the hundred thousand, so headroom costs more memory
      // than the rehashes it saves.
      requests_.reserve(kInitialInFlight);
    }
  }

  TProcessorEventHandlerBridge(const TProcessorEventHandlerBridge&) = delete;
  TProcessorEventHandlerBridge& operator=(const TProcessorEventHandlerBridge&) =
      delete;
  TProcessorEventHandlerBridge(TProcessorEventHandlerBridge&&) = delete;
  TProcessorEventHandlerBridge& operator=(TProcessorEventHandlerBridge&&) =
      delete;

  ~TProcessorEventHandlerBridge() {
    for (auto& [streamId, state] : requests_) {
      (void)streamId;
      releaseState(std::move(state));
    }
  }

  channel_pipeline::Result onRead(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    auto& request = msg.get<ThriftServerRequestMessage>();

    // The setup exchange is not a request. Latch the connection context it
    // carries — the only place the pipeline offers one — and publish the
    // classic context before downstream connection extensions run.
    if (FOLLY_UNLIKELY(request.payload.is<ThriftConnectionSetupPayload>())) {
      auto& setup = *request.payload.get<ThriftConnectionSetupPayload>().setup;
      ftConnContext_ = setup.connContext;
      if (handlers_ != nullptr && ftConnContext_ != nullptr &&
          connectionContext_ == nullptr) {
        if (auto metadata = setup.clientSetup.clientMetadata()) {
          ftConnContext_->setClientMetadata(*metadata);
        }
        connectionContext_ = std::make_unique<Cpp2ConnContextAdapter>(
            boost::intrusive_ptr<ThriftConnContext>(ftConnContext_),
            config_.identityResolver);
        for (const auto& handler : handlers_->server) {
          handler->newConnection(&connectionContext_->get());
        }
      }
      return ctx.fireRead(std::move(msg));
    }

    // No processor handlers means no per-request work, and so no classic
    // request context for anything downstream to read. A server configured
    // this way has no per-request security layer either, so a consumer that
    // finds nothing resolves nothing, which is the right answer. Note this is
    // narrower than the connection context, which is built for any handler at
    // all.
    if (!drivesProcessorHandlers_) {
      return ctx.fireRead(std::move(msg));
    }

    const auto routing = getServerRequestRoutingMetadata(request);
    if (FOLLY_UNLIKELY(routing.status != ServerRequestRoutingStatus::Ready)) {
      return ctx.fireRead(std::move(msg));
    }

    const uint32_t streamId = request.streamId;
    if (FOLLY_UNLIKELY(methodMetadata_ == nullptr)) {
      return ctx.fireWrite(
          channel_pipeline::erase_and_box(makeAppErrorMessage(
              streamId,
              "TProcessorEventHandlerBridgeMisconfigured",
              "event handlers are installed but the server has no method "
              "metadata registry")));
    }
    const auto* method = methodMetadata_->find(routing.methodName);
    if (FOLLY_UNLIKELY(method == nullptr)) {
      return ctx.fireRead(std::move(msg));
    }

    // Fail closed. A handler may be the thing authorizing this request, and
    // there is no way to tell from here, so a request the bridge cannot run
    // them for is refused rather than let through.
    if (FOLLY_UNLIKELY(
            request.requestContext == nullptr ||
            connectionContext_ == nullptr)) {
      return ctx.fireWrite(
          channel_pipeline::erase_and_box(makeAppErrorMessage(
              streamId,
              "TProcessorEventHandlerBridgeMisconfigured",
              "event handlers are installed but the pipeline provided no "
              "request or connection context")));
    }

    auto state = acquireState(*ctx.eventBase());
    const auto& requestResponse =
        request.payload.get<ThriftRequestResponsePayload>();
    state->protocolType = static_cast<apache::thrift::protocol::PROTOCOL_TYPES>(
        requestResponse.metadata->protocol().value_or(0));
    state->cpp2Request.emplace(&connectionContext_->get(), &state->header);
    state->context.emplace(
        *state->cpp2Request, state->header, *request.requestContext);

    // Scoped across the forward as well as the callbacks: handlers stamp
    // identity onto the ambient context for the service to read, and an
    // executor hop downstream captures whatever is installed here.
    folly::RequestContextScopeGuard guard(state->context->ambientContext());

    // Binding is inside the refusal path with the callbacks: it runs
    // getServiceContext, which is handler code like any other, and an escape
    // out of a noexcept pipeline callback would take the process with it.
    auto refusal = folly::try_and_catch([&] {
      state->chain.bind(
          state->context->get(),
          method->serviceName,
          method->qualifiedMethodName);
      state->chain.preRead();
      state->chain.onReadData(
          apache::thrift::SerializedMessage{
              .protocolType = state->protocolType,
              .buffer = requestResponse.data.get(),
              .methodName = method->qualifiedMethodName,
          });
      state->chain.postRead(
          state->context->header(),
          event_handler_detail::requestBytes(request));
    });
    // A handler may have installed a private child RequestContext while
    // binding. Retain the effective top before the read-side scope unwinds;
    // its guards retain every predecessor below it.
    state->context->captureAmbientContext();
    if (refusal) {
      auto response = makeRejectionMessage(streamId, refusal);
      // A handler that set a response header before refusing meant it for the
      // client; the refusal is the only response there will be. A rejection is
      // always a reply payload, so it has somewhere to carry them.
      event_handler_detail::stampWriteHeaders(
          response.payload.template get<ThriftInitialResponsePayload>(),
          state->context->takeWriteHeaders());
      auto boxed = channel_pipeline::erase_and_box(std::move(response));
      releaseState(std::move(state));
      return ctx.fireWrite(std::move(boxed));
    }

    // Always an insert: the rocket layer errors a stream id that is already in
    // flight, so the bridge never sees the same one twice. Assigning over a
    // live entry would drop a bound chain, so it is not offered.
    [[maybe_unused]] const bool inserted =
        requests_.try_emplace(streamId, std::move(state)).second;
    DCHECK(inserted);
    return ctx.fireRead(std::move(msg));
  }

  channel_pipeline::Result onWrite(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    auto& response = msg.get<ThriftServerResponseMessage>();

    // Resolved once: the stream this answers, and the reply body if it carries
    // one. A frame that names no stream belongs to no request.
    ThriftInitialResponsePayload* reply = nullptr;
    uint32_t streamId = 0;
    if (response.payload.is<ThriftInitialResponsePayload>()) {
      reply = &response.payload.get<ThriftInitialResponsePayload>();
      streamId = reply->streamId;
    } else if (response.payload.is<ThriftErrorPayload>()) {
      streamId = response.payload.get<ThriftErrorPayload>().streamId;
    }

    auto it = requests_.find(streamId);
    if (it == requests_.end()) {
      // Setup responses, connection-level frames, and anything the bridge
      // refused — none of which a handler saw on the way in.
      return ctx.fireWrite(std::move(msg));
    }
    auto state = std::move(it->second);
    requests_.erase(it);

    {
      // Responses may arrive after an executor hop, on an EventBase carrying
      // a different request. Reinstall the effective context captured after
      // all read-side handlers bound for the callbacks and cleanup only;
      // downstream forwarding resumes under the caller's context.
      const auto ambient = state->context->ambientContext();
      folly::RequestContextScopeGuard guard(ambient);

      // The write-side callbacks bracket a successful reply body. Classic
      // handlers do not receive them when the service or a downstream
      // interceptor rejects the request. The handlers' contexts are still
      // returned below, which is the pairing they are promised.
      if (reply != nullptr) {
        if (!event_handler_detail::isAppUnknownException(*reply)) {
          const auto* exception = state->context->exception();
          if (exception != nullptr) {
            state->chain.userExceptionWrapped(
                exception->declared, exception->exception);
            if (!exception->declared) {
              state->chain.handlerErrorWrapped(exception->exception);
            }
          }

          // Deliberately not caught: these run after the request has been
          // served, so there is no verdict left to honour and swallowing would
          // hide a handler bug. An escape terminates, as it would on a classic
          // server.
          if (exception == nullptr || exception->declared) {
            state->chain.preWrite();
            state->chain.onWriteData(state->protocolType, reply->data.get());
            state->chain.postWrite(
                event_handler_detail::payloadBytes(reply->data));
          }
        }

        // After postWrite: handlers write response headers there, and the reply
        // has not been serialized yet.
        event_handler_detail::stampWriteHeaders(
            *reply, state->context->takeWriteHeaders());
      }
      releaseState(std::move(state));
    }

    return ctx.fireWrite(std::move(msg));
  }

  using SubscribedEvents =
      channel_pipeline::Events<ThriftServerConnectionClosedEvent>;

  template <channel_pipeline::PipelineEvent E>
    requires std::same_as<E, ThriftServerConnectionClosedEvent>
  void on(Context&) noexcept {
    if (handlers_ == nullptr || connectionContext_ == nullptr ||
        connectionDestroyed_) {
      return;
    }
    // Announce closure once even if the event arrives twice: each handler is
    // promised one connectionDestroyed for the newConnection it observed.
    connectionDestroyed_ = true;
    // Requests still outstanding never produced a response, so their
    // write-side callbacks do not run. Keep their state until bridge
    // destruction because a service may still hold a request backed by this
    // connection context.
    for (const auto& handler : handlers_->server) {
      handler->connectionDestroyed(&connectionContext_->get());
    }
  }

  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }

  void onReadReady(Context&) noexcept {}
  void onWriteReady(Context&) noexcept {}
  void onPipelineActive(Context&) noexcept {}
  void onPipelineInactive(Context&) noexcept {}
  void handlerAdded(Context&) noexcept {}
  void handlerRemoved(Context&) noexcept {}

 private:
  static constexpr std::size_t kInitialInFlight = 8;
  // One request's adapted state, from the point it enters the pipeline until
  // its response leaves.
  struct RequestState {
    // The two small members every acquire and release touches, kept at the
    // head so the bookkeeping does not reach past the bulk of the state.
    EventHandlerChain chain;
    // Engaged only while a request is in flight.
    std::optional<Cpp2RequestContextAdapter> context;
    // Pointed at by the classic context below, so it is declared ahead of it
    // and outlives it.
    apache::thrift::transport::THeader header;
    std::optional<apache::thrift::Cpp2RequestContext> cpp2Request;
    apache::thrift::protocol::PROTOCOL_TYPES protocolType{};
    explicit RequestState(const EventHandlerChain::HandlerList& handlers)
        : chain(handlers) {}

    ~RequestState() {
      // The adapter clears the header view, so it must die while the pooled
      // header is still alive.
      context.reset();
    }
  };

  using RequestStatePtr =
      apache::thrift::fast_thrift::mem::evb_local_ptr<RequestState>;

  RequestStatePtr acquireState(folly::EventBase& eventBase) {
    return apache::thrift::fast_thrift::mem::evb_make_local<RequestState>(
        eventBase, handlers_->processor);
  }

  void releaseState(RequestStatePtr state) {
    const auto ambient = state->context->ambientContext();
    folly::RequestContextSaverScopeGuard guard;
    state->chain.unbind([&] {
      if (folly::RequestContext::try_get() != ambient.get()) {
        folly::RequestContext::setContext(ambient);
      }
    });
    // Destroying the adapter empties the request's extension slot rather than
    // leaving it pointing at storage the next request rebuilds.
    state->context.reset();
    state->cpp2Request.reset();
  }

  const TProcessorEventHandlerBridgeConfig config_;
  // Resolved once: the answer cannot change, and reaching it through the
  // shared_ptr on every request is two dependent loads into memory that every
  // connection shares.
  const TProcessorEventHandlers* const handlers_;
  const ThriftServerMethodMetadataRegistry* const methodMetadata_;
  const bool drivesProcessorHandlers_;
  // Sits here to land in the padding the flag above leaves, rather than widen
  // the connection by a word of its own.
  bool connectionDestroyed_{false};

  // Declared before the requests: their contexts borrow this one, and members
  // are destroyed in reverse.
  std::unique_ptr<Cpp2ConnContextAdapter> connectionContext_;
  // Latched from the setup message, the first thing the connection sends.
  // Non-owning; the connection-context handler outlives this pipeline.
  ThriftConnContext* ftConnContext_{nullptr};

  // Keyed by stream id rather than by the request context, because a
  // framework-generated error response carries no context to key on.
  folly::F14FastMap<uint32_t, RequestStatePtr> requests_;
};

} // namespace apache::thrift::fast_thrift::thrift::server
