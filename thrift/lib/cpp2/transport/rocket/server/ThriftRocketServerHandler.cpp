/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <thrift/lib/cpp2/transport/rocket/server/ThriftRocketServerHandler.h>

#include <memory>
#include <utility>

#include <fmt/core.h>
#include <folly/ExceptionString.h>
#include <folly/ExceptionWrapper.h>
#include <folly/GLog.h>
#include <folly/Overload.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/Flags.h>
#include <thrift/lib/cpp2/async/AsyncProcessorHelper.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/LoggingEventHelper.h>
#include <thrift/lib/cpp2/server/MonitoringMethodNames.h>
#include <thrift/lib/cpp2/server/VisitorHelper.h>
#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>
#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketSinkClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketThriftRequests.h>
#include <thrift/lib/cpp2/util/Checksum.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_constants.h>

namespace {
const int64_t kRocketServerMaxVersion = 8;
const int64_t kRocketServerMinVersion = 6;
} // namespace

THRIFT_FLAG_DEFINE_bool(rocket_server_legacy_protocol_key, true);
THRIFT_FLAG_DEFINE_int64(rocket_server_max_version, kRocketServerMaxVersion);

THRIFT_FLAG_DEFINE_int64(monitoring_over_user_logging_sample_rate, 0);

namespace apache {
namespace thrift {
namespace rocket {

thread_local uint32_t ThriftRocketServerHandler::sample_{0};

namespace {
bool isMetadataValid(const RequestRpcMetadata& metadata) {
  return metadata.protocol_ref() && metadata.name_ref() && metadata.kind_ref();
}
} // namespace

ThriftRocketServerHandler::ThriftRocketServerHandler(
    std::shared_ptr<Cpp2Worker> worker,
    const folly::SocketAddress& clientAddress,
    const folly::AsyncTransport* transport,
    const std::vector<std::unique_ptr<SetupFrameHandler>>& handlers)
    : worker_(std::move(worker)),
      connectionGuard_(worker_->getActiveRequestsGuard()),
      connContext_(
          &clientAddress,
          transport,
          nullptr, /* eventBaseManager */
          nullptr, /* duplexChannel */
          nullptr, /* x509PeerCert */
          worker_->getServer()->getClientIdentityHook(),
          worker_.get()),
      setupFrameHandlers_(handlers),
      version_(static_cast<int32_t>(std::min(
          kRocketServerMaxVersion, THRIFT_FLAG(rocket_server_max_version)))) {
  connContext_.setTransportType(Cpp2ConnContext::TransportType::ROCKET);
  for (const auto& handler : worker_->getServer()->getEventHandlersUnsafe()) {
    handler->newConnection(&connContext_);
  }
}

ThriftRocketServerHandler::~ThriftRocketServerHandler() {
  for (const auto& handler : worker_->getServer()->getEventHandlersUnsafe()) {
    handler->connectionDestroyed(&connContext_);
  }
  // Ensure each connAccepted() call has a matching connClosed()
  if (auto* observer = worker_->getServer()->getObserver()) {
    observer->connClosed();
  }
}

apache::thrift::server::TServerObserver::SamplingStatus
ThriftRocketServerHandler::shouldSample() {
  bool isServerSamplingEnabled =
      (sampleRate_ > 0) && ((sample_++ % sampleRate_) == 0);

  // TODO: determine isClientSamplingEnabled by "client_logging_enabled" header
  return apache::thrift::server::TServerObserver::SamplingStatus(
      isServerSamplingEnabled, false);
}

void ThriftRocketServerHandler::handleSetupFrame(
    SetupFrame&& frame, RocketServerConnection& connection) {
  if (!frame.payload().hasNonemptyMetadata()) {
    return connection.close(folly::make_exception_wrapper<RocketException>(
        ErrorCode::INVALID_SETUP, "Missing required metadata in SETUP frame"));
  }

  folly::io::Cursor cursor(frame.payload().buffer());

  // Validate Thrift protocol key
  uint32_t protocolKey;
  const bool success = cursor.tryReadBE<uint32_t>(protocolKey);
  constexpr uint32_t kLegacyRocketProtocolKey = 1;
  if (!success ||
      ((!THRIFT_FLAG(rocket_server_legacy_protocol_key) ||
        protocolKey != kLegacyRocketProtocolKey) &&
       protocolKey != RpcMetadata_constants::kRocketProtocolKey())) {
    return connection.close(folly::make_exception_wrapper<RocketException>(
        ErrorCode::INVALID_SETUP, "Incompatible Thrift version"));
  }

  try {
    CompactProtocolReader reader;
    reader.setInput(cursor);
    RequestSetupMetadata meta;
    // Throws on read error
    meta.read(&reader);
    if (reader.getCursorPosition() > frame.payload().metadataSize()) {
      return connection.close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID_SETUP,
          "Error deserializing SETUP payload: underflow"));
    }
    connContext_.readSetupMetadata(meta);

    auto minVersion = meta.minVersion_ref().value_or(0);
    auto maxVersion = meta.maxVersion_ref().value_or(0);

    THRIFT_CONNECTION_EVENT(rocket.setup).log(connContext_, [&] {
      return folly::dynamic::object("client_min_version", minVersion)(
          "client_max_version", maxVersion)("server_version", version_);
    });

    if (minVersion > version_) {
      return connection.close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID_SETUP, "Incompatible Rocket version"));
    }

    if (maxVersion < kRocketServerMinVersion) {
      return connection.close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID_SETUP, "Incompatible Rocket version"));
    }
    version_ = std::min(version_, maxVersion);
    eventBase_ = connContext_.getTransport()->getEventBase();
    for (const auto& h : setupFrameHandlers_) {
      auto processorInfo = h->tryHandle(meta);
      if (processorInfo) {
        bool valid = true;
        processorFactory_ = std::addressof(processorInfo->processorFactory_);
        serviceMetadata_ =
            std::addressof(worker_->getMetadataForService(*processorFactory_));
        valid &= !!(processor_ = processorFactory_->getProcessor());
        valid &= !!(threadManager_ = std::move(processorInfo->threadManager_));
        valid &= !!(serverConfigs_ = &processorInfo->serverConfigs_);
        requestsRegistry_ = processorInfo->requestsRegistry_ != nullptr
            ? processorInfo->requestsRegistry_
            : worker_->getRequestsRegistry();
        if (!valid) {
          return connection.close(
              folly::make_exception_wrapper<RocketException>(
                  ErrorCode::INVALID_SETUP,
                  "Error in implementation of custom connection handler."));
        }
        return;
      }
    }
    // no custom frame handler was found, do the default
    processorFactory_ = worker_->getServer()->getProcessorFactory().get();
    serviceMetadata_ =
        std::addressof(worker_->getMetadataForService(*processorFactory_));
    processor_ = processorFactory_->getProcessor();
    threadManager_ = worker_->getServer()->getThreadManager();
    serverConfigs_ = worker_->getServer();
    requestsRegistry_ = worker_->getRequestsRegistry();

    // add sampleRate
    if (serverConfigs_) {
      if (auto* observer = serverConfigs_->getObserver()) {
        sampleRate_ = observer->getSampleRate();
      }
    }

    if (meta.dscpToReflect_ref() || meta.markToReflect_ref()) {
      connection.applyDscpAndMarkToSocket(meta);
    }

    DCHECK_GE(version_, 5);
    ServerPushMetadata serverMeta;
    serverMeta.set_setupResponse();
    serverMeta.setupResponse_ref()->version_ref() = version_;
    CompactProtocolWriter compactProtocolWriter;
    folly::IOBufQueue queue;
    compactProtocolWriter.setOutput(&queue);
    serverMeta.write(&compactProtocolWriter);
    connection.sendMetadataPush(std::move(queue).move());

  } catch (const std::exception& e) {
    return connection.close(folly::make_exception_wrapper<RocketException>(
        ErrorCode::INVALID_SETUP,
        fmt::format(
            "Error deserializing SETUP payload: {}",
            folly::exceptionStr(e).toStdString())));
  }
}

void ThriftRocketServerHandler::handleRequestResponseFrame(
    RequestResponseFrame&& frame, RocketServerFrameContext&& context) {
  auto makeRequestResponse = [&](RequestRpcMetadata&& md,
                                 rocket::Payload&& debugPayload,
                                 std::shared_ptr<folly::RequestContext> ctx) {
    // Note, we're passing connContext by reference and rely on the next
    // chain of ownership to keep it alive: ThriftServerRequestResponse
    // stores RocketServerFrameContext, which keeps refcount on
    // RocketServerConnection, which in turn keeps ThriftRocketServerHandler
    // alive, which in turn keeps connContext_ alive.
    return RequestsRegistry::makeRequest<ThriftServerRequestResponse>(
        *eventBase_,
        *serverConfigs_,
        std::move(md),
        connContext_,
        std::move(ctx),
        *requestsRegistry_,
        std::move(debugPayload),
        std::move(context),
        version_);
  };

  handleRequestCommon(
      std::move(frame.payload()), std::move(makeRequestResponse));
}

void ThriftRocketServerHandler::handleRequestFnfFrame(
    RequestFnfFrame&& frame, RocketServerFrameContext&& context) {
  auto makeRequestFnf = [&](RequestRpcMetadata&& md,
                            rocket::Payload&& debugPayload,
                            std::shared_ptr<folly::RequestContext> ctx) {
    // Note, we're passing connContext by reference and rely on a complex
    // chain of ownership (see handleRequestResponseFrame for detailed
    // explanation).
    return RequestsRegistry::makeRequest<ThriftServerRequestFnf>(
        *eventBase_,
        *serverConfigs_,
        std::move(md),
        connContext_,
        std::move(ctx),
        *requestsRegistry_,
        std::move(debugPayload),
        std::move(context),
        [keepAlive = processor_] {});
  };

  handleRequestCommon(std::move(frame.payload()), std::move(makeRequestFnf));
}

void ThriftRocketServerHandler::handleRequestStreamFrame(
    RequestStreamFrame&& frame,
    RocketServerFrameContext&& context,
    RocketStreamClientCallback* clientCallback) {
  auto makeRequestStream = [&](RequestRpcMetadata&& md,
                               rocket::Payload&& debugPayload,
                               std::shared_ptr<folly::RequestContext> ctx) {
    return RequestsRegistry::makeRequest<ThriftServerRequestStream>(
        *eventBase_,
        *serverConfigs_,
        std::move(md),
        connContext_,
        std::move(ctx),
        *requestsRegistry_,
        std::move(debugPayload),
        std::move(context),
        version_,
        clientCallback,
        processor_);
  };

  handleRequestCommon(std::move(frame.payload()), std::move(makeRequestStream));
}

void ThriftRocketServerHandler::handleRequestChannelFrame(
    RequestChannelFrame&& frame,
    RocketServerFrameContext&& context,
    RocketSinkClientCallback* clientCallback) {
  auto makeRequestSink = [&](RequestRpcMetadata&& md,
                             rocket::Payload&& debugPayload,
                             std::shared_ptr<folly::RequestContext> ctx) {
    return RequestsRegistry::makeRequest<ThriftServerRequestSink>(
        *eventBase_,
        *serverConfigs_,
        std::move(md),
        connContext_,
        std::move(ctx),
        *requestsRegistry_,
        std::move(debugPayload),
        std::move(context),
        version_,
        clientCallback,
        processor_);
  };

  handleRequestCommon(std::move(frame.payload()), std::move(makeRequestSink));
}

void ThriftRocketServerHandler::connectionClosing() {
  connContext_.connectionClosed();
  if (processor_) {
    processor_->destroyAllInteractions(connContext_, *eventBase_);
  }
}

template <class F>
void ThriftRocketServerHandler::handleRequestCommon(
    Payload&& payload, F&& makeRequest) {
  // setup request sampling for counters and stats
  auto samplingStatus = shouldSample();
  std::chrono::steady_clock::time_point readEnd;
  if (UNLIKELY(samplingStatus.isEnabled())) {
    readEnd = std::chrono::steady_clock::now();
  }

  rocket::Payload debugPayload = payload.clone();
  auto requestPayloadTry =
      unpackAsCompressed<RequestPayload>(std::move(payload));

  auto makeActiveRequest = [&](auto&& md, auto&& payload, auto&& reqCtx) {
    serverConfigs_->incActiveRequests();
    return makeRequest(std::move(md), std::move(payload), std::move(reqCtx));
  };

  auto createDefaultRequestContext = [&] {
    return std::make_shared<folly::RequestContext>(
        requestsRegistry_->genRootId());
  };

  if (requestPayloadTry.hasException()) {
    handleDecompressionFailure(
        makeActiveRequest(
            RequestRpcMetadata(),
            rocket::Payload{},
            createDefaultRequestContext()),
        requestPayloadTry.exception().what().toStdString());
    return;
  }

  auto& data = requestPayloadTry->payload;
  auto& metadata = requestPayloadTry->metadata;

  if (!isMetadataValid(metadata)) {
    handleRequestWithBadMetadata(makeActiveRequest(
        std::move(metadata),
        std::move(debugPayload),
        createDefaultRequestContext()));
    return;
  }

  if (worker_->isStopping()) {
    handleServerShutdown(makeActiveRequest(
        std::move(metadata),
        std::move(debugPayload),
        createDefaultRequestContext()));
    return;
  }

  THRIFT_APPLICATION_EVENT(server_read_headers).log([&] {
    auto size =
        metadata.otherMetadata_ref() ? metadata.otherMetadata_ref()->size() : 0;
    std::vector<folly::dynamic> keys;
    if (size) {
      keys.reserve(size);
      for (auto& [k, v] : *metadata.otherMetadata_ref()) {
        keys.push_back(k);
      }
    }
    return folly::dynamic::object("size", size) //
        ("keys", folly::dynamic::array(std::move(keys)));
  });

  if (metadata.crc32c_ref()) {
    try {
      if (auto compression = metadata.compression_ref()) {
        data = uncompressBuffer(std::move(data), *compression);
      }
    } catch (...) {
      handleDecompressionFailure(
          makeActiveRequest(
              std::move(metadata),
              rocket::Payload{},
              createDefaultRequestContext()),
          folly::exceptionStr(std::current_exception()).toStdString());
      return;
    }
  }

  // check the checksum
  const bool badChecksum = metadata.crc32c_ref() &&
      (*metadata.crc32c_ref() != checksum::crc32c(*data));

  if (badChecksum) {
    handleRequestWithBadChecksum(makeActiveRequest(
        std::move(metadata),
        std::move(debugPayload),
        createDefaultRequestContext()));
    return;
  }

  if (auto injectedFailure = worker_->getServer()->maybeInjectFailure();
      injectedFailure != ThriftServer::InjectedFailure::NONE) {
    InjectedFault injectedFault;
    switch (injectedFailure) {
      case ThriftServer::InjectedFailure::NONE:
        folly::assume_unreachable();
      case ThriftServer::InjectedFailure::ERROR:
        injectedFault = InjectedFault::ERROR;
        break;
      case ThriftServer::InjectedFailure::DROP:
        injectedFault = InjectedFault::DROP;
        break;
      case ThriftServer::InjectedFailure::DISCONNECT:
        injectedFault = InjectedFault::DISCONNECT;
        break;
    }
    handleInjectedFault(
        makeActiveRequest(
            std::move(metadata),
            std::move(debugPayload),
            createDefaultRequestContext()),
        injectedFault);
    return;
  }

  using PerServiceMetadata = Cpp2Worker::PerServiceMetadata;
  const PerServiceMetadata::FindMethodResult methodMetadataResult =
      serviceMetadata_->findMethod(
          metadata.name_ref()
              ? metadata.name_ref()->view()
              : std::string_view{}); // need to call with empty string_view
                                     // because we still distinguish
                                     // between NotImplemented and
                                     // MetadataNotFound

  auto rootid = requestsRegistry_->genRootId();
  auto baseReqCtx =
      serviceMetadata_->getBaseContextForRequest(methodMetadataResult);
  auto reqCtx = baseReqCtx
      ? folly::RequestContext::copyAsRoot(*baseReqCtx, rootid)
      : std::make_shared<folly::RequestContext>(rootid);
  folly::RequestContextScopeGuard rctx(reqCtx);

  // A request should not be active until the overload checking is done.
  auto request = makeRequest(
      std::move(metadata), std::move(debugPayload), std::move(reqCtx));

  // check if server is overloaded
  const auto& headers = request->getTHeader().getHeaders();
  const auto& name = request->getMethodName();
  auto errorCode = serverConfigs_->checkOverload(&headers, &name);
  serverConfigs_->incActiveRequests();
  if (UNLIKELY(errorCode.has_value())) {
    handleRequestOverloadedServer(std::move(request), errorCode.value());
    return;
  }

  if (!serverConfigs_->getEnabled() ||
      (serverConfigs_->getRejectRequestsUntilStarted() &&
       !serverConfigs_->getStarted())) {
    if (!serverConfigs_->getInternalMethods().count(name)) {
      handleServerNotReady(std::move(request));
      return;
    }
  }

  auto preprocessResult =
      serverConfigs_->preprocess({headers, name, connContext_, request.get()});
  if (UNLIKELY(preprocessResult.has_value())) {
    preprocessResult->apply_visitor(
        apache::thrift::detail::VisitorHelper()
            .with([&](AppClientException& ace) {
              handleAppError(
                  std::move(request), ace.name(), ace.getMessage(), true);
            })
            .with([&](const AppServerException& ase) {
              handleAppError(
                  std::move(request), ase.name(), ase.getMessage(), false);
            }));
    return;
  }

  logSetupConnectionEventsOnce(setupLoggingFlag_, connContext_);

  auto* cpp2ReqCtx = request->getRequestContext();
  auto& timestamps = cpp2ReqCtx->getTimestamps();
  timestamps.setStatus(samplingStatus);
  if (UNLIKELY(samplingStatus.isEnabled())) {
    timestamps.readEnd = readEnd;
    timestamps.processBegin = std::chrono::steady_clock::now();
  }

  // Log monitoring methods that are called over non-monitoring interface so
  // that they can be migrated.
  LoggingSampler monitoringLogSampler{
      THRIFT_FLAG(monitoring_over_user_logging_sample_rate)};
  if (UNLIKELY(monitoringLogSampler.isSampled())) {
    if (LIKELY(connContext_.getInterfaceKind() != InterfaceKind::MONITORING)) {
      auto& methodName = request->getMethodName();
      if (UNLIKELY(isMonitoringMethodName(methodName))) {
        THRIFT_CONNECTION_EVENT(monitoring_over_user)
            .logSampled(connContext_, monitoringLogSampler, [&] {
              return folly::dynamic::object("method_name", methodName);
            });
      }
    }
  }

  if (serverConfigs_) {
    if (auto* observer = serverConfigs_->getObserver()) {
      // Expensive operations; happens only when sampling is enabled
      if (samplingStatus.isEnabledByServer()) {
        observer->queuedRequests(threadManager_->pendingUpstreamTaskCount());
        observer->activeRequests(serverConfigs_->getActiveRequests());
      }
    }
  }
  const auto protocolId = request->getProtoId();
  if (auto interactionId = metadata.interactionId_ref()) {
    cpp2ReqCtx->setInteractionId(*interactionId);
  }
  if (auto interactionCreate = metadata.interactionCreate_ref()) {
    cpp2ReqCtx->setInteractionCreate(*interactionCreate);
    DCHECK_EQ(cpp2ReqCtx->getInteractionId(), 0);
    cpp2ReqCtx->setInteractionId(*interactionCreate->interactionId_ref());
  }

  auto serializedCompressedRequest = SerializedCompressedRequest(
      std::move(data),
      metadata.crc32c_ref()
          ? CompressionAlgorithm::NONE
          : metadata.compression_ref().value_or(CompressionAlgorithm::NONE));

  try {
    folly::variant_match(
        methodMetadataResult,
        [&](PerServiceMetadata::MetadataNotImplemented) {
          // The AsyncProcessorFactory does not implement createMethodMetadata
          // so we need to fallback to processSerializedCompressedRequest.
          processor_->processSerializedCompressedRequest(
              std::move(request),
              std::move(serializedCompressedRequest),
              protocolId,
              cpp2ReqCtx,
              eventBase_,
              threadManager_.get());
        },
        [&](PerServiceMetadata::MetadataNotFound) {
          std::string_view methodName = request->getMethodName();
          AsyncProcessorHelper::sendUnknownMethodError(
              std::move(request), methodName);
        },
        [&](const PerServiceMetadata::MetadataFound& found) {
          processor_->processSerializedCompressedRequestWithMetadata(
              std::move(request),
              std::move(serializedCompressedRequest),
              found.metadata,
              protocolId,
              cpp2ReqCtx,
              eventBase_,
              threadManager_.get());
        });
  } catch (...) {
    LOG(DFATAL) << "AsyncProcessor::process exception: "
                << folly::exceptionStr(std::current_exception());
  }
}

void ThriftRocketServerHandler::handleRequestWithBadMetadata(
    ThriftRequestCoreUniquePtr request) {
  if (auto* observer = serverConfigs_->getObserver()) {
    observer->taskKilled();
  }
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::UNSUPPORTED_CLIENT_TYPE,
          "Invalid metadata object"),
      kRequestParsingErrorCode);
}

void ThriftRocketServerHandler::handleRequestWithBadChecksum(
    ThriftRequestCoreUniquePtr request) {
  if (auto* observer = serverConfigs_->getObserver()) {
    observer->taskKilled();
  }
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::CHECKSUM_MISMATCH, "Checksum mismatch"),
      kChecksumMismatchErrorCode);
}

void ThriftRocketServerHandler::handleDecompressionFailure(
    ThriftRequestCoreUniquePtr request, std::string&& reason) {
  if (auto* observer = serverConfigs_->getObserver()) {
    observer->taskKilled();
  }
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::INVALID_TRANSFORM,
          fmt::format("decompression failure: {}", std::move(reason))),
      kRequestParsingErrorCode);
}

void ThriftRocketServerHandler::handleRequestOverloadedServer(
    ThriftRequestCoreUniquePtr request, const std::string& errorCode) {
  if (auto* observer = serverConfigs_->getObserver()) {
    observer->serverOverloaded();
  }
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::LOADSHEDDING, "loadshedding request"),
      errorCode);
}

void ThriftRocketServerHandler::handleAppError(
    ThriftRequestCoreUniquePtr request,
    const std::string& name,
    const std::string& message,
    bool isClientError) {
  static const std::string headerEx = "uex";
  static const std::string headerExWhat = "uexw";
  auto header = request->getRequestContext()->getHeader();
  header->setHeader(headerEx, name);
  header->setHeader(headerExWhat, message);
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::UNKNOWN, std::move(message)),
      isClientError ? kAppClientErrorCode : kAppServerErrorCode);
}

void ThriftRocketServerHandler::handleServerNotReady(
    ThriftRequestCoreUniquePtr request) {
  if (auto* observer = serverConfigs_->getObserver()) {
    observer->taskKilled();
  }
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::LOADSHEDDING, "server not ready"),
      kQueueOverloadedErrorCode);
}

void ThriftRocketServerHandler::handleServerShutdown(
    ThriftRequestCoreUniquePtr request) {
  if (auto* observer = serverConfigs_->getObserver()) {
    observer->taskKilled();
  }
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::LOADSHEDDING, "server shutting down"),
      kQueueOverloadedErrorCode);
}

void ThriftRocketServerHandler::handleInjectedFault(
    ThriftRequestCoreUniquePtr request, InjectedFault fault) {
  switch (fault) {
    case InjectedFault::ERROR:
      if (auto* observer = serverConfigs_->getObserver()) {
        observer->taskKilled();
      }
      request->sendErrorWrapped(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::INJECTED_FAILURE, "injected failure"),
          kInjectedFailureErrorCode);
      return;
    case InjectedFault::DROP:
      VLOG(1) << "ERROR: injected drop: "
              << connContext_.getPeerAddress()->getAddressStr();
      return;
    case InjectedFault::DISCONNECT:
      return request->closeConnection(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::INJECTED_FAILURE, "injected failure"));
      return;
  }
}

void ThriftRocketServerHandler::requestComplete() {
  serverConfigs_->decActiveRequests();
}

void ThriftRocketServerHandler::terminateInteraction(int64_t id) {
  if (processor_) {
    processor_->terminateInteraction(id, connContext_, *eventBase_);
  }
}

void ThriftRocketServerHandler::onBeforeHandleFrame() {
  worker_->getServer()->touchRequestTimestamp();
}
} // namespace rocket
} // namespace thrift
} // namespace apache
