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

#include <utility>

#include <boost/intrusive_ptr.hpp>

#include <folly/ExceptionWrapper.h>

#include <folly/lang/Hint.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/context/ThriftConnContext.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/context/ThriftRequestContext.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/ResponsePayloads.h>

namespace apache::thrift::fast_thrift::thrift {

/**
 * ThriftServerRequestContextHandler — duplex pipeline handler that creates a
 * fresh ThriftRequestContext for every inbound request and populates all
 * context fields derived from the connection and request metadata.
 *
 * Outbound it does the closing half: hands the response headers accumulated on
 * that context to the outgoing metadata. The handler that opens the context
 * also drains it, and this is the last point on the write path that still can
 * — everything nearer the wire is serializing.
 *
 * Must sit upstream of any handler that wants to write into the request's
 * context (i.e. between ThriftServerTransportAdapter and any handler that
 * sets fields on ThriftRequestContext). Symmetrically, every handler and
 * extension that contributes a response header must sit downstream of it.
 */
template <typename Context>
class ThriftServerRequestContextHandler {
 public:
  // `requestExtensionLayout` is the server's request-scope slot plan, or null
  // on a server with no extensions. It outlives every connection.
  explicit ThriftServerRequestContextHandler(
      const ExtensionLayout* FOLLY_NULLABLE requestExtensionLayout) noexcept
      : requestExtensionLayout_(requestExtensionLayout) {}

  ThriftServerRequestContextHandler(
      const ExtensionLayout* FOLLY_NULLABLE requestExtensionLayout,
      boost::intrusive_ptr<ThriftConnContext> connContext,
      bool enableRequestHeaders) noexcept
      : requestExtensionLayout_(requestExtensionLayout),
        connContext_(std::move(connContext)),
        enableRequestHeaders_(enableRequestHeaders) {}

  // HandlerLifecycle
  void handlerAdded(Context& /*ctx*/) noexcept {}
  void handlerRemoved(Context& /*ctx*/) noexcept {}

  // InboundHandler
  channel_pipeline::Result onRead(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    auto& request = msg.get<ThriftServerRequestMessage>();
    // The setup message carries no request context and is answered further
    // down; nothing here applies to it. Tested positively: a valueless payload
    // is not a setup message and must still be handled.
    if (FOLLY_UNLIKELY(
            request.payload.template is<ThriftConnectionSetupPayload>())) {
      request.payload.template get<ThriftConnectionSetupPayload>()
          .setup->connContext = connContext_.get();
      return ctx.fireRead(std::move(msg));
    }
    request.requestContext = makeThriftRequestContext(*ctx.eventBase());
    request.requestContext->setConnectionContext(connContext_);
    if (requestExtensionLayout_ != nullptr) {
      request.requestContext->installExtensions(*requestExtensionLayout_);
    }
    if (request.payload.template is<ThriftServerRequestResponsePayload>()) {
      auto& metadata =
          request.payload.template get<ThriftServerRequestResponsePayload>()
              .metadata;
      if (metadata != nullptr && metadata->name().has_value()) {
        request.requestContext->setMethodName(
            std::move(*metadata->name()).str());
      }
      if (enableRequestHeaders_ && metadata != nullptr &&
          metadata->otherMetadata().has_value()) {
        request.requestContext->setHeaders(
            std::move(*metadata->otherMetadata()));
      }
    }
    return ctx.fireRead(std::move(msg));
  }

  void onReadReady(Context& /*ctx*/) noexcept {}

  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }

  void onPipelineActive(Context& /*ctx*/) noexcept {}

  // OutboundHandler
  channel_pipeline::Result onWrite(
      Context& ctx, channel_pipeline::TypeErasedBox&& msg) noexcept {
    attachResponseHeaders(msg.template get<ThriftServerResponseMessage>());
    return ctx.fireWrite(std::move(msg));
  }

  void onWriteReady(Context& /*ctx*/) noexcept {}

  void onPipelineInactive(Context& /*ctx*/) noexcept {}

 private:
  const ExtensionLayout* FOLLY_NULLABLE requestExtensionLayout_{nullptr};
  boost::intrusive_ptr<ThriftConnContext> connContext_;
  bool enableRequestHeaders_{false};
};

} // namespace apache::thrift::fast_thrift::thrift
