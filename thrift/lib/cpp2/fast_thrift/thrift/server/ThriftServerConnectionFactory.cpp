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

#include <thrift/lib/cpp2/fast_thrift/thrift/server/ThriftServerConnectionFactory.h>

#include <memory>
#include <type_traits>
#include <utility>

#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Event.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/StaticPipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/frame/handler/FrameCodecHandler.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/handler/FrameDefragmentationHandler.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/BackpressurePolicy.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/FragmentCompletionTracker.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/FrameFragmentationHandler.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/FrameLengthEncoderHandler.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/IntervalBatchingFrameHandler.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/WriteCompletionTracker.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/common/RocketStreamContext.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/common/handler/RocketMetricsHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/Event.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/RocketServerEventFactory.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/common/RocketServerConnection.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerKeepAliveHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerMessageMarshalHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerRequestResponseHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerStreamStateHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerWriteCompletionHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/handler/ThriftMetricsHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/SetupResponseBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/adapter/MetadataAppAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/adapter/ThriftServerCompositeAppAdapter.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Event.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/ThriftServerChecksumHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/ThriftServerCompressionHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/ThriftServerConnectionCloseHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/ThriftServerConnectionContextHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/ThriftServerRequestContextHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/ThriftServerRequestHeadersHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/ThriftServerRequestLifecycleHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/ThriftServerSetupHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/WriteBufferBackpressureHandler.h>

namespace apache::thrift::fast_thrift::thrift::server {

namespace {
using channel_pipeline::PipelineBuilder;
using channel_pipeline::PipelineImpl;
using channel_pipeline::PipelineOwner;
using channel_pipeline::SimpleBufferAllocator;

// Both write-path handlers always carry their completion tracker; backpressure
// participation is the one axis that varies. The trackers form a chain — the
// transport's per-writev completion becomes the batcher's per-batch event,
// which the fragmenter resolves into the per-frame completion the rocket layer
// consumes — and each stage's only subscriber is the next one, so a NoOp
// tracker anywhere strands every event below it with nowhere to go. Both
// batching aliases inherit their FlushWritesEvent subscription from the
// tracker, so either backpressure policy still flushes before teardown.
using ServerBatchingFrameHandler =
    frame::write::handler::IntervalBatchingFrameHandlerT<
        frame::write::handler::WriteCompletionTrackerT<
            rocket::server::RocketServerEventFactory>>;
using ServerBatchingFrameHandlerNoBackpressure =
    frame::write::handler::IntervalBatchingFrameHandlerT<
        frame::write::handler::WriteCompletionTrackerT<
            rocket::server::RocketServerEventFactory>,
        frame::write::handler::BackpressureDisabled>;
using ServerFragmentationFrameHandler =
    frame::write::handler::FrameFragmentationHandlerT<
        frame::write::handler::FragmentCompletionTrackerT<
            rocket::server::RocketServerEventFactory>>;
using ServerFragmentationFrameHandlerNoBackpressure =
    frame::write::handler::FrameFragmentationHandlerT<
        frame::write::handler::FragmentCompletionTrackerT<
            rocket::server::RocketServerEventFactory>,
        frame::write::handler::BackpressureDisabled>;

// Catch an accidentally disabled fragment completion tracker at the pipeline
// assembly seam, where it would otherwise strand batch completion events.
static_assert(
    !std::same_as<
        typename ServerFragmentationFrameHandler::SubscribedEvents,
        channel_pipeline::Events<>> &&
        !std::same_as<
            typename ServerFragmentationFrameHandlerNoBackpressure::
                SubscribedEvents,
            channel_pipeline::Events<>>,
    "the fragmenter is the batcher's only BatchWriteComplete subscriber in "
    "both backpressure configurations; a NoOp tracker here would strand it");

HANDLER_TAG(batching_frame_handler);
HANDLER_TAG(frame_length_encoder_handler);
HANDLER_TAG(frame_codec_handler);
HANDLER_TAG(frame_defragmentation_handler);
HANDLER_TAG(frame_fragmentation_handler);
HANDLER_TAG(server_write_completion_handler);
HANDLER_TAG(rocket_server_message_marshal_handler);
HANDLER_TAG(server_setup_frame_handler);
HANDLER_TAG(server_keepalive_handler);
HANDLER_TAG(server_request_response_frame_handler);
HANDLER_TAG(server_stream_state_handler);
HANDLER_TAG(thrift_server_request_context_handler);
HANDLER_TAG(thrift_server_connection_context_handler);
HANDLER_TAG(thrift_server_request_headers_handler);
HANDLER_TAG(thrift_server_request_lifecycle_handler);
HANDLER_TAG(thrift_server_compression_handler);
HANDLER_TAG(thrift_server_checksum_handler);
HANDLER_TAG(thrift_server_connection_close_handler);
HANDLER_TAG(write_buffer_backpressure_handler);
HANDLER_TAG(thrift_server_setup_handler);

template <bool Backpressure, bool WithStats>
PipelineOwner buildStaticRocketPipeline(
    const ThriftServerConnectionFactoryConfig& config,
    SimpleBufferAllocator* allocator,
    folly::EventBase* evb,
    rocket::server::RocketServerTransportHandler* transportHandler,
    rocket::server::RocketServerAppAdapter* appAdapter,
    ServerStatsShard* FOLLY_NULLABLE statsShard) {
  using BatchingHandler = std::conditional_t<
      Backpressure,
      ServerBatchingFrameHandler,
      ServerBatchingFrameHandlerNoBackpressure>;
  using FragmentationHandler = std::conditional_t<
      Backpressure,
      ServerFragmentationFrameHandler,
      ServerFragmentationFrameHandlerNoBackpressure>;

  auto builder =
      channel_pipeline::StaticPipelineBuilder<
          rocket::server::RocketServerTransportHandler,
          rocket::server::RocketServerAppAdapter,
          SimpleBufferAllocator>()
          .setEventBase(evb)
          .setHead(transportHandler)
          .setTail(appAdapter)
          .setAllocator(allocator)
          .addState<rocket::RocketStreamContexts>()
          .template addNextOutbound<BatchingHandler>(
              batching_frame_handler_tag, config.batchingConfig)
          .template addNextOutbound<
              frame::write::handler::FrameLengthEncoderHandler>(
              frame_length_encoder_handler_tag)
          .template addNextDuplex<frame::handler::FrameCodecHandler>(
              frame_codec_handler_tag)
          .template addNextInbound<
              frame::read::handler::FrameDefragmentationHandler>(
              frame_defragmentation_handler_tag)
          .template addNextOutbound<FragmentationHandler>(
              frame_fragmentation_handler_tag, config.fragmentationConfig)
          .template addNextDuplex<
              rocket::server::handler::RocketServerWriteCompletionHandler>(
              server_write_completion_handler_tag)
          .template addNextDuplex<
              rocket::server::handler::RocketServerMessageMarshalHandler>(
              rocket_server_message_marshal_handler_tag)
          .template addNextDuplex<
              rocket::server::handler::RocketServerSetupFrameHandler>(
              server_setup_frame_handler_tag)
          .template addNextDuplex<
              rocket::server::handler::RocketServerKeepAliveHandler>(
              server_keepalive_handler_tag)
          .template addNextDuplex<
              rocket::server::handler::RocketServerRequestResponseHandler>(
              server_request_response_frame_handler_tag)
          .template addNextDuplex<
              rocket::server::handler::RocketServerStreamStateHandler>(
              server_stream_state_handler_tag, config.enableCancellation);
  if constexpr (WithStats) {
    return PipelineOwner(
        std::move(builder)
            .template addNextDuplex<
                RocketMetricsHandler<Direction::Server, ServerStatsShard>>(
                rocket_metrics_handler_tag, CHECK_NOTNULL(statsShard))
            .build());
  } else {
    return PipelineOwner(std::move(builder).build());
  }
}

template <bool Backpressure>
PipelineOwner selectStaticRocketStats(
    const ThriftServerConnectionFactoryConfig& config,
    SimpleBufferAllocator* allocator,
    folly::EventBase* evb,
    rocket::server::RocketServerTransportHandler* transportHandler,
    rocket::server::RocketServerAppAdapter* appAdapter,
    ServerStatsShard* FOLLY_NULLABLE statsShard) {
  if (statsShard != nullptr) {
    return buildStaticRocketPipeline<Backpressure, true>(
        config, allocator, evb, transportHandler, appAdapter, statsShard);
  }
  return buildStaticRocketPipeline<Backpressure, false>(
      config, allocator, evb, transportHandler, appAdapter, statsShard);
}

template <typename Builder>
PipelineOwner finishStaticThriftPipeline(
    Builder&& builder, bool enableCancellation) {
  if (enableCancellation) {
    return PipelineOwner(
        std::forward<Builder>(builder)
            .template addNextDuplexTemplate<ThriftServerSetupHandler>(
                thrift_server_setup_handler_tag)
            .template addNextDuplexTemplate<
                ThriftServerRequestLifecycleHandler>(
                thrift_server_request_lifecycle_handler_tag)
            .build());
  }
  return PipelineOwner(
      std::forward<Builder>(builder)
          .template addNextDuplexTemplate<ThriftServerSetupHandler>(
              thrift_server_setup_handler_tag)
          .build());
}

template <bool WithWriteBuffer, bool WithExtensions, typename Builder>
PipelineOwner addStaticWriteBuffer(
    Builder&& builder,
    const ThriftServerConnectionFactoryConfig& config,
    ExtensionStateStore&) {
  if constexpr (WithWriteBuffer) {
    return finishStaticThriftPipeline(
        std::forward<Builder>(builder)
            .template addNextDuplexTemplate<WriteBufferBackpressureHandler>(
                write_buffer_backpressure_handler_tag),
        config.enableCancellation);
  } else {
    return finishStaticThriftPipeline(
        std::forward<Builder>(builder), config.enableCancellation);
  }
}

template <
    bool WithChecksum,
    bool WithWriteBuffer,
    bool WithExtensions,
    typename Builder>
PipelineOwner addStaticChecksum(
    Builder&& builder,
    const ThriftServerConnectionFactoryConfig& config,
    ExtensionStateStore& extensionStates) {
  if constexpr (WithChecksum) {
    return addStaticWriteBuffer<WithWriteBuffer, WithExtensions>(
        std::forward<Builder>(builder)
            .template addNextDuplexTemplate<ThriftServerChecksumHandler>(
                thrift_server_checksum_handler_tag)
            .template addNextDuplexTemplate<ThriftServerConnectionCloseHandler>(
                thrift_server_connection_close_handler_tag,
                config.drainTimeout,
                config.reapTimeout),
        config,
        extensionStates);
  } else {
    return addStaticWriteBuffer<WithWriteBuffer, WithExtensions>(
        std::forward<Builder>(builder)
            .template addNextDuplexTemplate<ThriftServerConnectionCloseHandler>(
                thrift_server_connection_close_handler_tag,
                config.drainTimeout,
                config.reapTimeout),
        config,
        extensionStates);
  }
}

template <
    bool WithHeaders,
    bool WithChecksum,
    bool WithWriteBuffer,
    bool WithExtensions,
    typename Builder>
PipelineOwner addStaticHeaders(
    Builder&& builder,
    const ThriftServerConnectionFactoryConfig& config,
    ExtensionStateStore& extensionStates) {
  if constexpr (WithHeaders) {
    return addStaticChecksum<WithChecksum, WithWriteBuffer, WithExtensions>(
        std::forward<Builder>(builder)
            .template addNextInboundTemplate<ThriftServerRequestHeadersHandler>(
                thrift_server_request_headers_handler_tag)
            .template addNextDuplexTemplate<ThriftServerCompressionHandler>(
                thrift_server_compression_handler_tag),
        config,
        extensionStates);
  } else {
    return addStaticChecksum<WithChecksum, WithWriteBuffer, WithExtensions>(
        std::forward<Builder>(builder)
            .template addNextDuplexTemplate<ThriftServerCompressionHandler>(
                thrift_server_compression_handler_tag),
        config,
        extensionStates);
  }
}

template <
    bool WithStats,
    bool WithHeaders,
    bool WithChecksum,
    bool WithWriteBuffer,
    bool WithExtensions,
    typename TailAdapter>
PipelineOwner buildStaticThriftPipeline(
    folly::EventBase* evb,
    ThriftServerTransportAdapter* transportAdapter,
    TailAdapter* tailAdapter,
    SimpleBufferAllocator* allocator,
    boost::intrusive_ptr<ThriftConnContext> connContext,
    ExtensionStateStore& extensionStates,
    const ThriftServerConnectionFactoryConfig& config,
    ServerStatsShard* statsShard) {
  channel_pipeline::StaticPipelineBuilder<
      ThriftServerTransportAdapter,
      TailAdapter,
      SimpleBufferAllocator>
      builder;
  builder.setEventBase(evb)
      .setHead(transportAdapter)
      .setTail(tailAdapter)
      .setAllocator(allocator);
  if constexpr (WithStats) {
    return addStaticHeaders<
        WithHeaders,
        WithChecksum,
        WithWriteBuffer,
        WithExtensions>(
        std::move(builder)
            .template addNextDuplex<
                ThriftMetricsHandler<Direction::Server, ServerStatsShard>>(
                thrift_metrics_handler_tag, statsShard)
            .template addNextDuplexTemplate<ThriftServerRequestContextHandler>(
                thrift_server_request_context_handler_tag,
                config.requestExtensionLayout.get())
            .template addNextInboundTemplate<
                ThriftServerConnectionContextHandler>(
                thrift_server_connection_context_handler_tag,
                std::move(connContext)),
        config,
        extensionStates);
  } else {
    return addStaticHeaders<
        WithHeaders,
        WithChecksum,
        WithWriteBuffer,
        WithExtensions>(
        std::move(builder)
            .template addNextDuplexTemplate<ThriftServerRequestContextHandler>(
                thrift_server_request_context_handler_tag,
                config.requestExtensionLayout.get())
            .template addNextInboundTemplate<
                ThriftServerConnectionContextHandler>(
                thrift_server_connection_context_handler_tag,
                std::move(connContext)),
        config,
        extensionStates);
  }
}

template <
    bool WithStats,
    bool WithHeaders,
    bool WithChecksum,
    bool WithWriteBuffer,
    typename TailAdapter>
PipelineOwner selectStaticExtensions(
    bool withExtensions,
    folly::EventBase* evb,
    ThriftServerTransportAdapter* transportAdapter,
    TailAdapter* tailAdapter,
    SimpleBufferAllocator* allocator,
    boost::intrusive_ptr<ThriftConnContext> connContext,
    ExtensionStateStore& extensionStates,
    const ThriftServerConnectionFactoryConfig& config,
    ServerStatsShard* statsShard) {
  DCHECK(!withExtensions);
  return buildStaticThriftPipeline<
      WithStats,
      WithHeaders,
      WithChecksum,
      WithWriteBuffer,
      false>(
      evb,
      transportAdapter,
      tailAdapter,
      allocator,
      std::move(connContext),
      extensionStates,
      config,
      statsShard);
}

template <
    bool WithStats,
    bool WithHeaders,
    bool WithChecksum,
    typename TailAdapter>
PipelineOwner selectStaticWriteBuffer(
    const ThriftServerConnectionFactoryConfig& config,
    folly::EventBase* evb,
    ThriftServerTransportAdapter* transportAdapter,
    TailAdapter* tailAdapter,
    SimpleBufferAllocator* allocator,
    boost::intrusive_ptr<ThriftConnContext> connContext,
    ExtensionStateStore& extensionStates,
    ServerStatsShard* statsShard) {
  const bool withExtensions = !config.thriftPipelineHandlerFactories.empty();
  if (config.enableWriteBufferBackpressure) {
    return selectStaticExtensions<WithStats, WithHeaders, WithChecksum, true>(
        withExtensions,
        evb,
        transportAdapter,
        tailAdapter,
        allocator,
        std::move(connContext),
        extensionStates,
        config,
        statsShard);
  }
  return selectStaticExtensions<WithStats, WithHeaders, WithChecksum, false>(
      withExtensions,
      evb,
      transportAdapter,
      tailAdapter,
      allocator,
      std::move(connContext),
      extensionStates,
      config,
      statsShard);
}

template <bool WithStats, bool WithHeaders, typename TailAdapter>
PipelineOwner selectStaticChecksum(
    const ThriftServerConnectionFactoryConfig& config,
    folly::EventBase* evb,
    ThriftServerTransportAdapter* transportAdapter,
    TailAdapter* tailAdapter,
    SimpleBufferAllocator* allocator,
    boost::intrusive_ptr<ThriftConnContext> connContext,
    ExtensionStateStore& extensionStates,
    ServerStatsShard* statsShard) {
  if (config.enableChecksum) {
    return selectStaticWriteBuffer<WithStats, WithHeaders, true>(
        config,
        evb,
        transportAdapter,
        tailAdapter,
        allocator,
        std::move(connContext),
        extensionStates,
        statsShard);
  }
  return selectStaticWriteBuffer<WithStats, WithHeaders, false>(
      config,
      evb,
      transportAdapter,
      tailAdapter,
      allocator,
      std::move(connContext),
      extensionStates,
      statsShard);
}

template <bool WithStats, typename TailAdapter>
PipelineOwner selectStaticHeaders(
    const ThriftServerConnectionFactoryConfig& config,
    folly::EventBase* evb,
    ThriftServerTransportAdapter* transportAdapter,
    TailAdapter* tailAdapter,
    SimpleBufferAllocator* allocator,
    boost::intrusive_ptr<ThriftConnContext> connContext,
    ExtensionStateStore& extensionStates,
    ServerStatsShard* statsShard) {
  if (config.enableRequestHeaders) {
    return selectStaticChecksum<WithStats, true>(
        config,
        evb,
        transportAdapter,
        tailAdapter,
        allocator,
        std::move(connContext),
        extensionStates,
        statsShard);
  }
  return selectStaticChecksum<WithStats, false>(
      config,
      evb,
      transportAdapter,
      tailAdapter,
      allocator,
      std::move(connContext),
      extensionStates,
      statsShard);
}

template <typename TailAdapter>
PipelineOwner selectStaticThriftStats(
    const ThriftServerConnectionFactoryConfig& config,
    folly::EventBase* evb,
    ThriftServerTransportAdapter* transportAdapter,
    TailAdapter* tailAdapter,
    SimpleBufferAllocator* allocator,
    boost::intrusive_ptr<ThriftConnContext> connContext,
    ExtensionStateStore& extensionStates,
    ServerStatsShard* statsShard) {
  if (statsShard != nullptr) {
    return selectStaticHeaders<true>(
        config,
        evb,
        transportAdapter,
        tailAdapter,
        allocator,
        std::move(connContext),
        extensionStates,
        statsShard);
  }
  return selectStaticHeaders<false>(
      config,
      evb,
      transportAdapter,
      tailAdapter,
      allocator,
      std::move(connContext),
      extensionStates,
      statsShard);
}
} // namespace

ThriftServerConnectionFactory::ThriftServerConnectionFactory(
    ThriftServerConnectionFactoryConfig config)
    : config_(std::move(config)),
      needsComposite_(
          static_cast<bool>(config_.monitoringHandler) ||
          static_cast<bool>(config_.statusHandler) ||
          static_cast<bool>(config_.debugHandler) ||
          static_cast<bool>(config_.controlHandler) ||
          static_cast<bool>(config_.securityHandler) ||
          static_cast<bool>(config_.metadataResponse)) {
  CHECK(config_.handler)
      << "ThriftServerConnectionFactory requires a non-null handler";
  if (!needsComposite_) {
    return;
  }

  std::vector<std::shared_ptr<const ThriftServerMethodDispatchTable>> tables;
  bool allTablesAvailable = true;
  auto appendTable = [&](const auto& factory) {
    if (!factory) {
      return;
    }
    auto table = factory->getMethodDispatchTable();
    if (!table) {
      allTablesAvailable = false;
      return;
    }
    tables.push_back(std::move(table));
  };
  appendTable(config_.handler);
  appendTable(config_.monitoringHandler);
  appendTable(config_.statusHandler);
  appendTable(config_.debugHandler);
  appendTable(config_.controlHandler);
  appendTable(config_.securityHandler);
  if (config_.metadataResponse) {
    tables.push_back(MetadataAppAdapter::methodDispatchTable());
  }
  if (allTablesAvailable) {
    compositeRoutes_ =
        ThriftServerCompositeRoutingTable::create(std::move(tables));
  }
}

ThriftServerConnection ThriftServerConnectionFactory::getConnection(
    folly::AsyncTransport::UniquePtr socket,
    const folly::SocketAddress& clientAddr,
    const std::shared_ptr<const connection::PeerSecurityInfo>& peerSecurity) {
  boost::intrusive_ptr<ThriftConnContext> connContext{new ThriftConnContext()};
  if (config_.connExtensionLayout != nullptr) {
    connContext->installExtensions(*config_.connExtensionLayout);
  }
  connContext->setPeerAddress(clientAddr);
  // Non-owning. The transport is owned by the transport adapter, which
  // ThriftServerConnection tears down after the pipeline that reads this —
  // so it outlives every handler holding the context. Security is snapshotted
  // separately because this is already the post-StopTLS transport when one was
  // negotiated, and reports nothing about the peer.
  connContext->setTransport(socket.get());
  if (peerSecurity != nullptr) {
    connContext->setPeerCertificate(peerSecurity->peerCertificate);
    connContext->setSecurityProtocol(peerSecurity->securityProtocol);
  }

  auto conn = needsComposite_
      ? buildCompositeConnection(std::move(socket), connContext)
      : buildSimpleConnection(std::move(socket), connContext);

  // Expose the context on the connection so the connection-layer accept
  // callback can reach it post-construction.
  conn.connContext = std::move(connContext);

  // Note: the connection is fully wired but inert. Reading is started
  // separately via ThriftServerConnection::start() once the connection
  // layer has run its accept-time setup (e.g. onConnectionAccepted hook,
  // registration in the connection-manager map). Starting here would race
  // those steps: setReadCB can synchronously drain pre-received bytes
  // (post-StopTLS handoff) and dispatch the first request before the
  // accept hook has populated per-connection state.
  return conn;
}

void ThriftServerConnectionFactory::attachCPUExecutor(
    ThriftServerAppAdapter& adapter) const {
  if (config_.cpuExecutor) {
    adapter.setCPUExecutor(config_.cpuExecutor);
  }
}

ThriftServerConnection ThriftServerConnectionFactory::buildSimpleConnection(
    folly::AsyncTransport::UniquePtr socket,
    boost::intrusive_ptr<ThriftConnContext> connContext) {
  ThriftServerConnection::SimpleTail tail{
      .adapter = config_.handler->getAppAdapter(config_.handler)};
  auto* tailAdapter = tail.adapter.get();
  attachCPUExecutor(*tailAdapter);
  return buildConnectionImpl<ThriftServerAppAdapter>(
      std::move(socket), std::move(tail), tailAdapter, std::move(connContext));
}

ThriftServerConnection ThriftServerConnectionFactory::buildCompositeConnection(
    folly::AsyncTransport::UniquePtr socket,
    boost::intrusive_ptr<ThriftConnContext> connContext) {
  // Build the composite tail: user adapter + each wired aux + metadata.
  // children must outlive the composite (composite borrows raw T* into
  // them); ThriftServerConnection::CompositeTail field declaration order
  // guarantees this.
  ThriftServerConnection::CompositeTail tail;
  tail.children.push_back(config_.handler->getAppAdapter(config_.handler));
  attachCPUExecutor(*tail.children.back());
  // Aux interfaces offload like the user handler. Methods that must stay on
  // the EventBase are pinned per-method in their IDLs via
  // @cpp.ProcessInEbThreadUnsafe, not by withholding the executor here.
  if (config_.monitoringHandler) {
    tail.children.push_back(
        config_.monitoringHandler->getAppAdapter(config_.monitoringHandler));
    attachCPUExecutor(*tail.children.back());
  }
  if (config_.statusHandler) {
    tail.children.push_back(
        config_.statusHandler->getAppAdapter(config_.statusHandler));
    attachCPUExecutor(*tail.children.back());
  }
  if (config_.debugHandler) {
    tail.children.push_back(
        config_.debugHandler->getAppAdapter(config_.debugHandler));
    attachCPUExecutor(*tail.children.back());
  }
  if (config_.controlHandler) {
    tail.children.push_back(
        config_.controlHandler->getAppAdapter(config_.controlHandler));
    attachCPUExecutor(*tail.children.back());
  }
  if (config_.securityHandler) {
    tail.children.push_back(
        config_.securityHandler->getAppAdapter(config_.securityHandler));
    attachCPUExecutor(*tail.children.back());
  }
  // Deliberately not offloaded: MetadataAppAdapter is hand-written rather
  // than generated, so it never consults cpuExecutor() and completes through
  // the EventBase-only writeResponse overload. Attaching an executor would
  // compile and do nothing. Its body only re-serializes an immutable
  // prebuilt response, so there is nothing to move off the EventBase.
  if (config_.metadataResponse) {
    tail.children.push_back(
        ThriftServerAppAdapter::Ptr{
            new MetadataAppAdapter(config_.metadataResponse)});
  }
  tail.adapter = ThriftServerCompositeAppAdapter::Ptr{
      new ThriftServerCompositeAppAdapter(compositeRoutes_)};
  for (auto& child : tail.children) {
    tail.adapter->addChild(child.get());
  }
  auto* compositeAdapter = tail.adapter.get();
  return buildConnectionImpl<ThriftServerCompositeAppAdapter>(
      std::move(socket),
      std::move(tail),
      compositeAdapter,
      std::move(connContext));
}

template <typename TailAdapter>
ThriftServerConnection ThriftServerConnectionFactory::buildConnectionImpl(
    folly::AsyncTransport::UniquePtr socket,
    std::variant<
        std::monostate,
        ThriftServerConnection::SimpleTail,
        ThriftServerConnection::CompositeTail> tail,
    TailAdapter* tailAdapter,
    boost::intrusive_ptr<ThriftConnContext> connContext) {
  auto* evb = socket->getEventBase();

  // Accept runs on the IO thread that will own this connection, which is the
  // thread holding the shard these counts belong in — both to look it up here
  // and to increment it from the pipeline later.
  ServerStatsShard* statsShard = nullptr;
  if (config_.stats) {
    DCHECK(evb->isInEventBaseThread());
    statsShard = &config_.stats->currentThreadShard();
  }

  auto transportHandler =
      rocket::server::RocketServerTransportHandler::create(std::move(socket));

  ThriftServerConnection conn;
  conn.tail = std::move(tail);

  // Construct the transport adapter early around a fresh (empty) rocket
  // connection so the rocket pipeline's SETUP callback below can capture a
  // stable pointer into it. The rocket connection's appAdapter is
  // default-initialized; transportHandler / pipeline are populated after
  // buildRocketPipeline runs.
  conn.thriftTransportAdapter = std::make_unique<ThriftServerTransportAdapter>(
      std::make_unique<rocket::server::RocketServerConnection>());
  auto& rocketConn = conn.thriftTransportAdapter->rocketConnection();
  auto* transportAdapterPtr = conn.thriftTransportAdapter.get();

  auto rocketPipeline = buildRocketPipeline(
      evb, transportHandler.get(), rocketConn.appAdapter.get(), statsShard);
  rocketConn.appAdapter->setPipeline(rocketPipeline.get());
  transportHandler->setPipeline(rocketPipeline.get());

  if (config_.zeroCopyThreshold > 0) {
    if (!transportHandler->setZeroCopy(true)) {
      XLOG(WARN) << "MSG_ZEROCOPY not supported on this socket";
    }
    transportHandler->setZeroCopyEnableThreshold(config_.zeroCopyThreshold);
  }
  rocketConn.transportHandler = std::move(transportHandler);
  rocketConn.pipeline = std::move(rocketPipeline);

  // Thrift pipeline templated on the tail adapter type. For the simple case
  // this works because generated adapters use the base adapter's shared
  // dispatch implementation; the composite tail also fans setPipeline out to
  // every child.
  using ReqCtxHandler =
      ThriftServerRequestContextHandler<channel_pipeline::detail::ContextImpl>;
  using ConnCtxHandler = ThriftServerConnectionContextHandler<
      channel_pipeline::detail::ContextImpl>;
  using ReqHeadersHandler =
      ThriftServerRequestHeadersHandler<channel_pipeline::detail::ContextImpl>;
  using CompressionHandler =
      ThriftServerCompressionHandler<channel_pipeline::detail::ContextImpl>;
  using ChecksumHandler =
      ThriftServerChecksumHandler<channel_pipeline::detail::ContextImpl>;
  using CloseHandler =
      ThriftServerConnectionCloseHandler<channel_pipeline::detail::ContextImpl>;
  using RequestLifecycleHandler = ThriftServerRequestLifecycleHandler<
      channel_pipeline::detail::ContextImpl>;
  using WriteBufferHandler =
      WriteBufferBackpressureHandler<channel_pipeline::detail::ContextImpl>;
  using SetupHandler =
      ThriftServerSetupHandler<channel_pipeline::detail::ContextImpl>;
  PipelineOwner thriftPipeline;
  if (config_.channelPipelineMode == ChannelPipelineMode::Static &&
      config_.thriftPipelineHandlerFactories.empty()) {
    thriftPipeline = selectStaticThriftStats(
        config_,
        evb,
        transportAdapterPtr,
        tailAdapter,
        &conn.thriftAllocator,
        std::move(connContext),
        conn.extensionStates,
        statsShard);
  } else {
    PipelineBuilder<
        ThriftServerTransportAdapter,
        TailAdapter,
        SimpleBufferAllocator>
        thriftPipelineBuilder;
    thriftPipelineBuilder.setEventBase(evb)
        .setHead(transportAdapterPtr)
        .setTail(tailAdapter)
        .setAllocator(&conn.thriftAllocator);
    // Sits closest to the head so it sees every message crossing the thrift
    // layer, before any handler below can absorb or synthesize one.
    if (statsShard != nullptr) {
      thriftPipelineBuilder.template addNextDuplex<
          ThriftMetricsHandler<Direction::Server, ServerStatsShard>>(
          thrift_metrics_handler_tag, statsShard);
    }
    // Duplex: inbound it creates the per-request context, outbound it hands
    // that context's response headers to the outgoing metadata. Sitting
    // closest to the head on the write path makes it the last contributor
    // downstream of every handler and extension that can add one.
    thriftPipelineBuilder
        .template addNextDuplex<ReqCtxHandler>(
            thrift_server_request_context_handler_tag,
            config_.requestExtensionLayout.get())
        .template addNextInbound<ConnCtxHandler>(
            thrift_server_connection_context_handler_tag,
            std::move(connContext));
    // Stamps RequestRpcMetadata.otherMetadata onto each request's
    // ThriftRequestContext.
    if (config_.enableRequestHeaders) {
      thriftPipelineBuilder.template addNextInbound<ReqHeadersHandler>(
          thrift_server_request_headers_handler_tag);
    }
    // Inbound decompression precedes checksum verification. Outbound traverses
    // the handlers in reverse, so the checksum is computed on the uncompressed
    // response before compression.
    thriftPipelineBuilder.template addNextDuplex<CompressionHandler>(
        thrift_server_compression_handler_tag);
    // The checksum handler is added after the context handlers so inbound it
    // runs once the per-request ThriftRequestContext exists (it records the
    // algorithm there for the response to echo).
    if (config_.enableChecksum) {
      thriftPipelineBuilder.template addNextDuplex<ChecksumHandler>(
          thrift_server_checksum_handler_tag);
    }
    // Connection-close handler sits immediately upstream of the tail.
    // ThriftServerConnection::close() fires
    // ThriftServerCloseConnectionEvent through the pipeline; the handler
    // handles it through its typed callback and drives the terminal state
    // machine.
    thriftPipelineBuilder.template addNextDuplex<CloseHandler>(
        thrift_server_connection_close_handler_tag,
        config_.drainTimeout,
        config_.reapTimeout);
    // Write-buffer handler sits between the context handlers and the drain
    // handler. Placed above drain (closer to head) so its inbound
    // Backpressure signal propagates upstream toward the transport, and
    // outbound responses from the tail traverse drain → write-buffer →
    // head.
    if (config_.enableWriteBufferBackpressure) {
      thriftPipelineBuilder.template addNextDuplex<WriteBufferHandler>(
          write_buffer_backpressure_handler_tag);
    }
    // Embedder-registered handlers go after all built-ins, in registration
    // order — the first sits closest to the head, the last immediately above
    // the tail adapter. Each factory constructs a fresh per-connection
    // instance.
    for (const auto& factory : config_.thriftPipelineHandlerFactories) {
      thriftPipelineBuilder.addErasedHandler(factory(conn.extensionStates));
    }
    // Last before the tail, and deliberately after the embedder handlers: this
    // terminates the connection-lifecycle messages, so everything that might
    // answer one has to run first. The application tail then only ever sees
    // requests.
    thriftPipelineBuilder.template addNextDuplex<SetupHandler>(
        thrift_server_setup_handler_tag);
    if (config_.enableCancellation) {
      thriftPipelineBuilder.template addNextDuplex<RequestLifecycleHandler>(
          thrift_server_request_lifecycle_handler_tag);
    }
    thriftPipeline = thriftPipelineBuilder.build();
  }
  transportAdapterPtr->setPipeline(thriftPipeline.get());
  tailAdapter->setPipeline(thriftPipeline.get());
  conn.thriftPipeline = std::move(thriftPipeline);

  return conn;
}

PipelineOwner ThriftServerConnectionFactory::buildRocketPipeline(
    folly::EventBase* evb,
    rocket::server::RocketServerTransportHandler* transportHandler,
    rocket::server::RocketServerAppAdapter* appAdapter,
    ServerStatsShard* FOLLY_NULLABLE statsShard) {
  if (config_.channelPipelineMode == ChannelPipelineMode::Static) {
    if (config_.enableBackpressure) {
      return selectStaticRocketStats<true>(
          config_,
          &rocketAllocator_,
          evb,
          transportHandler,
          appAdapter,
          statsShard);
    }
    return selectStaticRocketStats<false>(
        config_,
        &rocketAllocator_,
        evb,
        transportHandler,
        appAdapter,
        statsShard);
  }

  // addState rebinds: it returns a builder of an extended type and leaves the
  // original moved-from, so the chain up to and including it must be bound
  // here rather than continued on a pre-declared builder.
  auto builder = PipelineBuilder<
                     rocket::server::RocketServerTransportHandler,
                     rocket::server::RocketServerAppAdapter,
                     SimpleBufferAllocator>()
                     .setEventBase(evb)
                     .setHead(transportHandler)
                     .setTail(appAdapter)
                     .setAllocator(&rocketAllocator_)
                     .addState<rocket::RocketStreamContexts>();
  // Batching and fragmentation are always present, but which specialization
  // is spliced depends on enableBackpressure. The no-backpressure variants
  // batch and fragment identically; they simply carry no write-ready hook, so
  // makeHandlerNode never registers them and the pipeline's writeReadyList_
  // stays empty. The choice costs one branch per connection, not per message.
  if (config_.enableBackpressure) {
    builder
        // Batcher composed with the write-completion tracker: the tracker turns
        // the transport's per-writev completions into one event per
        // rocket-frame batch. The fragmentation handler below turns that into
        // the rocket-level completion the app adapter relays up to the thrift
        // pipeline.
        .addNextOutbound<ServerBatchingFrameHandler>(
            batching_frame_handler_tag, config_.batchingConfig);
  } else {
    builder.addNextOutbound<ServerBatchingFrameHandlerNoBackpressure>(
        batching_frame_handler_tag, config_.batchingConfig);
  }
  builder
      .addNextOutbound<frame::write::handler::FrameLengthEncoderHandler>(
          frame_length_encoder_handler_tag)
      .addNextDuplex<frame::handler::FrameCodecHandler>(frame_codec_handler_tag)
      .addNextInbound<frame::read::handler::FrameDefragmentationHandler>(
          frame_defragmentation_handler_tag);
  // Fragmenter composed with the fragment-completion tracker. It records each
  // frame's streamId on the way down — below this handler the frame is
  // serialized bytes and the stream is no longer recoverable — and fans the
  // batcher's batch event back out into one completion per original frame. It
  // is also the only handler that can narrow the batcher's quiescence verdict,
  // since the frames it is still holding are invisible from below.
  if (config_.enableBackpressure) {
    builder.addNextOutbound<ServerFragmentationFrameHandler>(
        frame_fragmentation_handler_tag, config_.fragmentationConfig);
  } else {
    builder.addNextOutbound<ServerFragmentationFrameHandlerNoBackpressure>(
        frame_fragmentation_handler_tag, config_.fragmentationConfig);
  }
  builder
      // Restates the fragmenter's frame-layer completion as the rocket layer's
      // own, so nothing above the rocket boundary subscribes to a frame event.
      .addNextDuplex<
          rocket::server::handler::RocketServerWriteCompletionHandler>(
          server_write_completion_handler_tag)
      .addNextDuplex<
          rocket::server::handler::RocketServerMessageMarshalHandler>(
          rocket_server_message_marshal_handler_tag)
      .addNextDuplex<rocket::server::handler::RocketServerSetupFrameHandler>(
          server_setup_frame_handler_tag)
      .addNextDuplex<rocket::server::handler::RocketServerKeepAliveHandler>(
          server_keepalive_handler_tag)
      .addNextDuplex<rocket::server::handler::RocketServerStreamStateHandler>(
          server_stream_state_handler_tag, config_.enableCancellation)
      .addNextDuplex<
          rocket::server::handler::RocketServerRequestResponseHandler>(
          server_request_response_frame_handler_tag);
  // Sits closest to the tail, so inbound it counts frames that survived
  // parsing/defragmentation and outbound it counts frames as the app emits
  // them, before batching or fragmentation can change the frame count.
  if (statsShard != nullptr) {
    builder.addNextDuplex<
        RocketMetricsHandler<Direction::Server, ServerStatsShard>>(
        rocket_metrics_handler_tag, statsShard);
  }
  return PipelineOwner(builder.build());
}

} // namespace apache::thrift::fast_thrift::thrift::server
