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
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
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
      clientAddress_(clientAddress),
      connContext_(
          &clientAddress_,
          transport,
          nullptr, /* eventBaseManager */
          nullptr, /* duplexChannel */
          nullptr, /* x509PeerCert */
          worker_->getServer()->getClientIdentityHook()),
      setupFrameHandlers_(handlers) {
  if (auto* handler = worker_->getServer()->getEventHandlerUnsafe()) {
    handler->newConnection(&connContext_);
  }
}

ThriftRocketServerHandler::~ThriftRocketServerHandler() {
  if (auto* handler = worker_->getServer()->getEventHandlerUnsafe()) {
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
    SetupFrame&& frame,
    RocketServerConnection& connection) {
  if (!frame.payload().hasNonemptyMetadata()) {
    return connection.close(folly::make_exception_wrapper<RocketException>(
        ErrorCode::INVALID_SETUP, "Missing required metadata in SETUP frame"));
  }

  folly::io::Cursor cursor(frame.payload().buffer());

  // Validate Thrift major/minor version
  int16_t majorVersion;
  int16_t minorVersion;
  const bool success = cursor.tryReadBE<int16_t>(majorVersion) &&
      cursor.tryReadBE<int16_t>(minorVersion);
  if (!success || majorVersion != 0 || minorVersion != 1) {
    return connection.close(folly::make_exception_wrapper<RocketException>(
        ErrorCode::INVALID_SETUP, "Incompatible Thrift version"));
  }

  try {
    CompactProtocolReader reader;
    reader.setInput(cursor);
    auto meta = std::make_unique<RequestSetupMetadata>();
    // Throws on read error
    meta->read(&reader);
    if (reader.getCursorPosition() > frame.payload().metadataSize()) {
      return connection.close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID_SETUP,
          "Error deserializing SETUP payload: underflow"));
    }

    auto minVersion = meta->minVersion_ref().value_or(0);
    auto maxVersion = meta->maxVersion_ref().value_or(0);

    if (minVersion > version_) {
      return connection.close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID_SETUP, "Incompatible Rocket version"));
    }

    if (maxVersion < 0) {
      return connection.close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID_SETUP, "Incompatible Rocket version"));
    }
    version_ = std::min(version_, maxVersion);

    eventBase_ = connContext_.getTransport()->getEventBase();
    for (const auto& h : setupFrameHandlers_) {
      auto processorInfo = h->tryHandle(*meta);
      if (processorInfo) {
        bool valid = true;
        valid &= !!(cpp2Processor_ = std::move(processorInfo->cpp2Processor_));
        valid &= !!(threadManager_ = std::move(processorInfo->threadManager_));
        valid &= !!(serverConfigs_ = &processorInfo->serverConfigs_);
        valid &= !!(requestsRegistry_ = processorInfo->requestsRegistry_);
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
    cpp2Processor_ = worker_->getServer()->getCpp2Processor();
    threadManager_ = worker_->getServer()->getThreadManager();
    serverConfigs_ = worker_->getServer();
    requestsRegistry_ = worker_->getRequestsRegistry();
    // add sampleRate
    if (serverConfigs_) {
      if (auto* observer = serverConfigs_->getObserver()) {
        sampleRate_ = observer->getSampleRate();
      }
    }
  } catch (const std::exception& e) {
    return connection.close(folly::make_exception_wrapper<RocketException>(
        ErrorCode::INVALID_SETUP,
        fmt::format(
            "Error deserializing SETUP payload: {}",
            folly::exceptionStr(e).toStdString())));
  }
}

void ThriftRocketServerHandler::handleRequestResponseFrame(
    RequestResponseFrame&& frame,
    RocketServerFrameContext&& context) {
  auto makeRequestResponse = [&](RequestRpcMetadata&& md,
                                 std::unique_ptr<folly::IOBuf> debugPayload,
                                 std::shared_ptr<folly::RequestContext> ctx) {
    serverConfigs_->incActiveRequests();
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
    RequestFnfFrame&& frame,
    RocketServerFrameContext&& context) {
  auto makeRequestFnf = [&](RequestRpcMetadata&& md,
                            std::unique_ptr<folly::IOBuf> debugPayload,
                            std::shared_ptr<folly::RequestContext> ctx) {
    serverConfigs_->incActiveRequests();
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
        [keepAlive = cpp2Processor_] {});
  };

  handleRequestCommon(std::move(frame.payload()), std::move(makeRequestFnf));
}

void ThriftRocketServerHandler::handleRequestStreamFrame(
    RequestStreamFrame&& frame,
    RocketServerFrameContext&& context,
    RocketStreamClientCallback* clientCallback) {
  auto makeRequestStream = [&](RequestRpcMetadata&& md,
                               std::unique_ptr<folly::IOBuf> debugPayload,
                               std::shared_ptr<folly::RequestContext> ctx) {
    serverConfigs_->incActiveRequests();
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
        cpp2Processor_);
  };

  handleRequestCommon(std::move(frame.payload()), std::move(makeRequestStream));
}

void ThriftRocketServerHandler::handleRequestChannelFrame(
    RequestChannelFrame&& frame,
    RocketServerFrameContext&& context,
    RocketSinkClientCallback* clientCallback) {
  auto makeRequestSink = [&](RequestRpcMetadata&& md,
                             std::unique_ptr<folly::IOBuf> debugPayload,
                             std::shared_ptr<folly::RequestContext> ctx) {
    serverConfigs_->incActiveRequests();
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
        cpp2Processor_);
  };

  handleRequestCommon(std::move(frame.payload()), std::move(makeRequestSink));
}

template <class F>
void ThriftRocketServerHandler::handleRequestCommon(
    Payload&& payload,
    F&& makeRequest) {
  // setup request sampling for counters and stats
  auto samplingStatus = shouldSample();
  std::chrono::steady_clock::time_point readEnd;
  if (UNLIKELY(samplingStatus.isEnabled())) {
    readEnd = std::chrono::steady_clock::now();
  }

  auto baseReqCtx = cpp2Processor_->getBaseContextForRequest();
  auto rootid = requestsRegistry_->genRootId();
  auto reqCtx = baseReqCtx
      ? folly::RequestContext::copyAsRoot(*baseReqCtx, rootid)
      : std::make_shared<folly::RequestContext>(rootid);

  folly::RequestContextScopeGuard rctx(reqCtx);

  auto requestPayloadTry = unpack<RequestPayload>(std::move(payload));

  if (requestPayloadTry.hasException()) {
    handleDecompressionFailure(
        makeRequest(RequestRpcMetadata(), {}, std::move(reqCtx)),
        requestPayloadTry.exception().what().toStdString());
    return;
  }

  auto& data = requestPayloadTry->payload;
  auto& metadata = requestPayloadTry->metadata;

  auto debugPayload = data->clone();

  if (!isMetadataValid(metadata)) {
    handleRequestWithBadMetadata(makeRequest(
        std::move(metadata), std::move(debugPayload), std::move(reqCtx)));
    return;
  }

  if (worker_->isStopping()) {
    handleServerShutdown(makeRequest(
        std::move(metadata), std::move(debugPayload), std::move(reqCtx)));
    return;
  }
  // check if server is overloaded
  auto errorCode = serverConfigs_->checkOverload(
      metadata.otherMetadata_ref() ? &*metadata.otherMetadata_ref() : nullptr,
      &*metadata.name_ref());
  if (UNLIKELY(errorCode.has_value())) {
    handleRequestOverloadedServer(
        makeRequest(
            std::move(metadata), std::move(debugPayload), std::move(reqCtx)),
        errorCode.value());
    return;
  }
  auto preprocessResult = serverConfigs_->preprocess(
      metadata.otherMetadata_ref() ? &*metadata.otherMetadata_ref() : nullptr,
      &*metadata.name_ref());
  if (UNLIKELY(preprocessResult.has_value())) {
    auto req = makeRequest(
        std::move(metadata), std::move(debugPayload), std::move(reqCtx));
    preprocessResult->apply_visitor(
        apache::thrift::detail::VisitorHelper()
            .with([&](AppClientException& ace) {
              handleAppError(
                  std::move(req), ace.name(), ace.getMessage(), true);
            })
            .with([&](const AppServerException& ase) {
              handleAppError(
                  std::move(req), ase.name(), ase.getMessage(), false);
            }));
    return;
  }

  // check the checksum
  const bool badChecksum = metadata.crc32c_ref() &&
      (*metadata.crc32c_ref() != checksum::crc32c(*data));

  if (badChecksum) {
    handleRequestWithBadChecksum(makeRequest(
        std::move(metadata), std::move(debugPayload), std::move(reqCtx)));
    return;
  }

  auto request = makeRequest(
      std::move(metadata), std::move(debugPayload), std::move(reqCtx));
  auto* cpp2ReqCtx = request->getRequestContext();
  auto& timestamps = cpp2ReqCtx->getTimestamps();
  timestamps.setStatus(samplingStatus);
  if (UNLIKELY(samplingStatus.isEnabled())) {
    timestamps.readEnd = readEnd;
    timestamps.processBegin = std::chrono::steady_clock::now();
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
  try {
    cpp2Processor_->processSerializedRequest(
        std::move(request),
        SerializedRequest(std::move(data)),
        protocolId,
        cpp2ReqCtx,
        eventBase_,
        threadManager_.get());
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
      "Corrupted metadata in rsocket request");
}

void ThriftRocketServerHandler::handleRequestWithBadChecksum(
    ThriftRequestCoreUniquePtr request) {
  if (auto* observer = serverConfigs_->getObserver()) {
    observer->taskKilled();
  }
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::CHECKSUM_MISMATCH, "Checksum mismatch"),
      "Corrupted request");
}

void ThriftRocketServerHandler::handleDecompressionFailure(
    ThriftRequestCoreUniquePtr request,
    std::string&& reason) {
  if (auto* observer = serverConfigs_->getObserver()) {
    observer->taskKilled();
  }
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::INVALID_TRANSFORM,
          fmt::format("decompression failure: {}", std::move(reason))),
      "decompression failure");
}

void ThriftRocketServerHandler::handleRequestOverloadedServer(
    ThriftRequestCoreUniquePtr request,
    const std::string& errorCode) {
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

void ThriftRocketServerHandler::requestComplete() {
  serverConfigs_->decActiveRequests();
}

void ThriftRocketServerHandler::terminateInteraction(int64_t) {
  LOG(ERROR) << "Not implemented";
}
} // namespace rocket
} // namespace thrift
} // namespace apache
