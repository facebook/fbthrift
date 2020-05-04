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

#include <rsocket/RSocketParameters.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
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

#include <thrift/lib/cpp2/transport/rsocket/gen-cpp2/Config_types.h>
#include <thrift/lib/cpp2/util/Checksum.h>

namespace apache {
namespace thrift {
namespace rocket {

thread_local uint32_t ThriftRocketServerHandler::sample_{0};

namespace {
bool deserializeMetadata(const Payload& p, RequestRpcMetadata& metadata) {
  try {
    CompactProtocolReader reader;
    reader.setInput(p.buffer());
    metadata.read(&reader);
    return reader.getCursorPosition() <= p.metadataSize();
  } catch (const std::exception& ex) {
    FB_LOG_EVERY_MS(ERROR, 10000)
        << "Exception on deserializing metadata: " << folly::exceptionStr(ex);
    return false;
  }
}

bool isMetadataValid(const RequestRpcMetadata& metadata) {
  return metadata.protocol_ref() && metadata.name_ref() && metadata.kind_ref();
}
} // namespace

ThriftRocketServerHandler::ThriftRocketServerHandler(
    std::shared_ptr<Cpp2Worker> worker,
    const folly::SocketAddress& clientAddress,
    const folly::AsyncTransportWrapper* transport,
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
      setupFrameHandlers_(handlers) {}

ThriftRocketServerHandler::~ThriftRocketServerHandler() {
  if (serverConfigs_) {
    if (auto* observer = serverConfigs_->getObserver()) {
      observer->connClosed();
    }
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
                                 const folly::RequestContext* ctx) {
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
        *requestsRegistry_,
        std::move(debugPayload),
        ctx->getRootId(),
        std::move(context));
  };

  handleRequestCommon(
      std::move(frame.payload()), std::move(makeRequestResponse));
}

void ThriftRocketServerHandler::handleRequestFnfFrame(
    RequestFnfFrame&& frame,
    RocketServerFrameContext&& context) {
  auto makeRequestFnf = [&](RequestRpcMetadata&& md,
                            std::unique_ptr<folly::IOBuf> debugPayload,
                            const folly::RequestContext* ctx) {
    serverConfigs_->incActiveRequests();
    // Note, we're passing connContext by reference and rely on a complex
    // chain of ownership (see handleRequestResponseFrame for detailed
    // explanation).
    return RequestsRegistry::makeRequest<ThriftServerRequestFnf>(
        *eventBase_,
        *serverConfigs_,
        std::move(md),
        connContext_,
        *requestsRegistry_,
        std::move(debugPayload),
        ctx->getRootId(),
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
                               const folly::RequestContext* ctx) {
    serverConfigs_->incActiveRequests();
    return RequestsRegistry::makeRequest<ThriftServerRequestStream>(
        *eventBase_,
        *serverConfigs_,
        std::move(md),
        connContext_,
        *requestsRegistry_,
        std::move(debugPayload),
        ctx->getRootId(),
        std::move(context),
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
                             const folly::RequestContext* ctx) {
    serverConfigs_->incActiveRequests();
    return RequestsRegistry::makeRequest<ThriftServerRequestSink>(
        *eventBase_,
        *serverConfigs_,
        std::move(md),
        connContext_,
        *requestsRegistry_,
        std::move(debugPayload),
        ctx->getRootId(),
        std::move(context),
        clientCallback,
        cpp2Processor_);
  };

  handleRequestCommon(std::move(frame.payload()), std::move(makeRequestSink));
}

template <class F>
void ThriftRocketServerHandler::handleRequestCommon(
    Payload&& payload,
    F&& makeRequest) {
  auto baseReqCtx = cpp2Processor_->getBaseContextForRequest();
  auto rootid = requestsRegistry_->genRootId();
  auto reqCtx = baseReqCtx
      ? folly::RequestContext::copyAsRoot(*baseReqCtx, rootid)
      : std::make_shared<folly::RequestContext>(rootid);
  folly::RequestContextScopeGuard rctx(reqCtx);

  RequestRpcMetadata metadata;
  const bool parseOk = payload.metadataSize() <= payload.buffer()->length() &&
      deserializeMetadata(payload, metadata);
  bool validMetadata = parseOk && isMetadataValid(metadata);

  // it is not safe get debug payload if metadata is not valid, the metadata
  // size may be unbounded.
  auto debugPayload = validMetadata
      ? Payload::makeCombined(payload.buffer()->clone(), payload.metadataSize())
            .data()
      : nullptr;

  if (!validMetadata) {
    handleRequestWithBadMetadata(makeRequest(
        std::move(metadata), std::move(debugPayload), reqCtx.get()));
    return;
  }

  if (worker_->isStopping()) {
    handleServerShutdown(makeRequest(
        std::move(metadata), std::move(debugPayload), reqCtx.get()));
    return;
  }
  // check if server is overloaded
  auto errorCode = serverConfigs_->checkOverload(
      metadata.otherMetadata_ref() ? &*metadata.otherMetadata_ref() : nullptr,
      &*metadata.name_ref());
  if (UNLIKELY(errorCode.has_value())) {
    handleRequestOverloadedServer(
        makeRequest(std::move(metadata), std::move(debugPayload), reqCtx.get()),
        errorCode.value());
    return;
  }

  auto data = std::move(payload).data();
  // uncompress the request if it's compressed
  if (auto compression = metadata.compression_ref()) {
    auto result =
        rocket::uncompressPayload(*metadata.compression_ref(), std::move(data));
    if (!result) {
      handleDecompressionFailure(
          makeRequest(
              std::move(metadata), std::move(debugPayload), reqCtx.get()),
          std::move(result).error());
      return;
    }
    data = std::move(result.value());
  }

  // check the checksum
  const bool badChecksum = metadata.crc32c_ref() &&
      (*metadata.crc32c_ref() != checksum::crc32c(*data));

  if (badChecksum) {
    handleRequestWithBadChecksum(makeRequest(
        std::move(metadata), std::move(debugPayload), reqCtx.get()));
    return;
  }

  auto request =
      makeRequest(std::move(metadata), std::move(debugPayload), reqCtx.get());
  const auto protocolId = request->getProtoId();
  auto* const cpp2ReqCtx = request->getRequestContext();
  if (serverConfigs_) {
    if (auto* observer = serverConfigs_->getObserver()) {
      auto samplingStatus = shouldSample();
      if (UNLIKELY(
              samplingStatus.isEnabled() &&
              samplingStatus.isEnabledByServer())) {
        observer->queuedRequests(threadManager_->pendingTaskCount());
        observer->activeRequests(serverConfigs_->getActiveRequests());
      }
    }
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
    const std::string& /*errorCode*/) {
  if (auto* observer = serverConfigs_->getObserver()) {
    observer->serverOverloaded();
  }
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::LOADSHEDDING, "Loadshedding request"),
      kOverloadedErrorCode);
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
} // namespace rocket
} // namespace thrift
} // namespace apache
