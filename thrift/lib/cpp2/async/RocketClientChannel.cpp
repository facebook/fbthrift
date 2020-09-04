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

#include <thrift/lib/cpp2/async/RocketClientChannel.h>

#include <memory>
#include <utility>

#include <fmt/core.h>
#include <folly/ExceptionString.h>
#include <folly/GLog.h>
#include <folly/Likely.h>
#include <folly/Memory.h>
#include <folly/Try.h>
#include <folly/compression/Compression.h>
#include <folly/fibers/FiberManager.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/Request.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/protocol/TBase64Utils.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/HeaderChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/core/EnvelopeUtil.h>
#include <thrift/lib/cpp2/transport/core/RpcMetadataUtil.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/core/TryUtil.h>
#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

using namespace apache::thrift::transport;

namespace apache {
namespace thrift {

namespace {
folly::Try<FirstResponsePayload> decodeResponseError(
    rocket::RocketException&& ex,
    uint16_t protocolId,
    folly::StringPiece methodName) noexcept {
  switch (ex.getErrorCode()) {
    case rocket::ErrorCode::CANCELED:
    case rocket::ErrorCode::INVALID:
    case rocket::ErrorCode::REJECTED:
      break;
    default:
      return folly::Try<FirstResponsePayload>(
          folly::make_exception_wrapper<TApplicationException>(fmt::format(
              "Unexpected error frame type: {}",
              static_cast<uint32_t>(ex.getErrorCode()))));
  }

  ResponseRpcError responseError;
  try {
    rocket::unpackCompact(responseError, ex.moveErrorData().get());
  } catch (...) {
    return folly::Try<FirstResponsePayload>(
        folly::make_exception_wrapper<TApplicationException>(fmt::format(
            "Error parsing error frame: {}",
            folly::exceptionStr(std::current_exception()).toStdString())));
  }

  folly::Optional<std::string> exCode;
  TApplicationException::TApplicationExceptionType exType{
      TApplicationException::UNKNOWN};
  switch (responseError.code_ref().value_or(ResponseRpcErrorCode::UNKNOWN)) {
    case ResponseRpcErrorCode::OVERLOAD:
      exCode = kOverloadedErrorCode;
      exType = TApplicationException::LOADSHEDDING;
      break;
    case ResponseRpcErrorCode::TASK_EXPIRED:
      exCode = kTaskExpiredErrorCode;
      exType = TApplicationException::TIMEOUT;
      break;
    case ResponseRpcErrorCode::QUEUE_OVERLOADED:
    case ResponseRpcErrorCode::SHUTDOWN:
      exCode = kQueueOverloadedErrorCode;
      exType = TApplicationException::LOADSHEDDING;
      break;
    case ResponseRpcErrorCode::INJECTED_FAILURE:
      exCode = kInjectedFailureErrorCode;
      exType = TApplicationException::INJECTED_FAILURE;
      break;
    case ResponseRpcErrorCode::REQUEST_PARSING_FAILURE:
      exCode = kUnknownErrorCode;
      exType = TApplicationException::UNSUPPORTED_CLIENT_TYPE;
      break;
    case ResponseRpcErrorCode::QUEUE_TIMEOUT:
      exCode = kServerQueueTimeoutErrorCode;
      exType = TApplicationException::TIMEOUT;
      break;
    case ResponseRpcErrorCode::RESPONSE_TOO_BIG:
      exCode = kResponseTooBigErrorCode;
      exType = TApplicationException::INTERNAL_ERROR;
      break;
    case ResponseRpcErrorCode::WRONG_RPC_KIND:
      exCode = kRequestTypeDoesntMatchServiceFunctionType;
      exType = TApplicationException::UNKNOWN_METHOD;
      break;
    case ResponseRpcErrorCode::UNKNOWN_METHOD:
      exCode = kMethodUnknownErrorCode;
      exType = TApplicationException::UNKNOWN_METHOD;
      break;
    case ResponseRpcErrorCode::CHECKSUM_MISMATCH:
      exCode = kUnknownErrorCode;
      exType = TApplicationException::CHECKSUM_MISMATCH;
      break;
    case ResponseRpcErrorCode::INTERRUPTION:
      exType = TApplicationException::INTERRUPTION;
      break;
    case ResponseRpcErrorCode::APP_OVERLOAD:
      exCode = kAppOverloadedErrorCode;
      exType = TApplicationException::LOADSHEDDING;
      break;
    default:
      exCode = kUnknownErrorCode;
      break;
  }

  ResponseRpcMetadata metadata;
  if (exCode) {
    metadata.otherMetadata_ref().emplace();
    (*metadata.otherMetadata_ref())["ex"] = *exCode;
  }
  if (auto loadRef = responseError.load_ref()) {
    metadata.load_ref() = *loadRef;
  }
  return folly::Try<FirstResponsePayload>(FirstResponsePayload(
      LegacySerializedResponse(
          protocolId,
          methodName,
          TApplicationException(
              exType, responseError.what_utf8_ref().value_or("")))
          .buffer,
      std::move(metadata)));
}

FOLLY_NODISCARD folly::exception_wrapper processFirstResponse(
    ResponseRpcMetadata& metadata,
    std::unique_ptr<folly::IOBuf>& payload,
    uint16_t protocolId,
    folly::StringPiece methodName) {
  if (auto payloadMetadataRef = metadata.payloadMetadata_ref()) {
    const auto isProxiedResponse =
        metadata.proxiedPayloadMetadata_ref().has_value();

    switch (payloadMetadataRef->getType()) {
      case PayloadMetadata::responseMetadata:
        payload =
            LegacySerializedResponse(
                protocolId, methodName, SerializedResponse(std::move(payload)))
                .buffer;
        break;
      case PayloadMetadata::exceptionMetadata: {
        auto& exceptionMetadataBase =
            payloadMetadataRef->get_exceptionMetadata();
        auto otherMetadataRef = metadata.otherMetadata_ref();
        if (!otherMetadataRef) {
          otherMetadataRef.emplace();
        }
        auto exceptionNameRef = exceptionMetadataBase.name_utf8_ref();
        auto exceptionWhatRef = exceptionMetadataBase.what_utf8_ref();
        if (auto exceptionMetadataRef = exceptionMetadataBase.metadata_ref()) {
          switch (exceptionMetadataRef->getType()) {
            case PayloadExceptionMetadata::declaredException:
              if (exceptionNameRef) {
                (*otherMetadataRef)[isProxiedResponse ? "puex" : "uex"] =
                    *exceptionNameRef;
              }
              if (exceptionWhatRef) {
                (*otherMetadataRef)[isProxiedResponse ? "puexw" : "uexw"] =
                    *exceptionWhatRef;
              }
              payload = LegacySerializedResponse(
                            protocolId,
                            methodName,
                            SerializedResponse(std::move(payload)))
                            .buffer;
              break;
            case PayloadExceptionMetadata::proxyException:
              (*otherMetadataRef)
                  [isProxiedResponse ? "servicerouter:sr_error"
                                     : "servicerouter:sr_internal_error"] =
                      protocol::base64Encode(payload->coalesce());
              payload =
                  LegacySerializedResponse(
                      protocolId,
                      methodName,
                      TApplicationException(exceptionWhatRef.value_or("")))
                      .buffer;
              break;
            case PayloadExceptionMetadata::proxiedException:
              (*otherMetadataRef)["servicerouter:sr_error"] =
                  protocol::base64Encode(payload->coalesce());
              payload =
                  LegacySerializedResponse(
                      protocolId,
                      methodName,
                      TApplicationException(exceptionWhatRef.value_or("")))
                      .buffer;
              break;
            case PayloadExceptionMetadata::appClientException:
              (*otherMetadataRef)[isProxiedResponse ? "pex" : "ex"] =
                  kAppClientErrorCode;
              FOLLY_FALLTHROUGH;
            default:
              if (exceptionNameRef) {
                (*otherMetadataRef)[isProxiedResponse ? "puex" : "uex"] =
                    *exceptionNameRef;
              }
              if (exceptionWhatRef) {
                (*otherMetadataRef)[isProxiedResponse ? "puexw" : "uexw"] =
                    *exceptionWhatRef;
              }
              payload =
                  LegacySerializedResponse(
                      protocolId,
                      methodName,
                      TApplicationException(exceptionWhatRef.value_or("")))
                      .buffer;
          }
        } else {
          return TApplicationException("Missing payload exception metadata");
        }
        break;
      }
      default:
        return TApplicationException("Invalid payload metadata type");
    }
  }
  return {};
}

class FirstRequestProcessorStream : public StreamClientCallback,
                                    private StreamServerCallback {
 public:
  FirstRequestProcessorStream(
      uint16_t protocolId,
      folly::StringPiece methodName,
      StreamClientCallback* clientCallback,
      folly::EventBase* evb)
      : protocolId_(protocolId),
        methodName_(methodName),
        clientCallback_(clientCallback),
        evb_(evb) {}

  FOLLY_NODISCARD bool onFirstResponse(
      FirstResponsePayload&& firstResponse,
      folly::EventBase* evb,
      StreamServerCallback* serverCallback) override {
    SCOPE_EXIT {
      delete this;
    };
    DCHECK_EQ(evb, evb_);
    if (auto error = processFirstResponse(
            firstResponse.metadata,
            firstResponse.payload,
            protocolId_,
            methodName_)) {
      serverCallback->onStreamCancel();
      clientCallback_->onFirstResponseError(std::move(error));
      return false;
    }
    serverCallback->resetClientCallback(*clientCallback_);
    return clientCallback_->onFirstResponse(
        std::move(firstResponse), evb, serverCallback);
  }
  void onFirstResponseError(folly::exception_wrapper ew) override {
    SCOPE_EXIT {
      delete this;
    };
    ew.handle(
        [&](rocket::RocketException& ex) {
          auto response =
              decodeResponseError(std::move(ex), protocolId_, methodName_);
          if (response.hasException()) {
            clientCallback_->onFirstResponseError(
                std::move(response).exception());
            return;
          }

          if (clientCallback_->onFirstResponse(
                  std::move(*response), evb_, this)) {
            DCHECK(clientCallback_);
            clientCallback_->onStreamComplete();
          }
        },
        [&](...) { clientCallback_->onFirstResponseError(std::move(ew)); });
  }

  bool onStreamNext(StreamPayload&&) override {
    std::terminate();
  }
  void onStreamError(folly::exception_wrapper) override {
    std::terminate();
  }
  void onStreamComplete() override {
    std::terminate();
  }
  void resetServerCallback(StreamServerCallback&) override {
    std::terminate();
  }

  void onStreamCancel() override {
    clientCallback_ = nullptr;
  }
  bool onStreamRequestN(uint64_t) override {
    return true;
  }
  void resetClientCallback(StreamClientCallback& clientCallback) override {
    clientCallback_ = &clientCallback;
  }

 private:
  const uint16_t protocolId_;
  const std::string methodName_;
  StreamClientCallback* clientCallback_;
  folly::EventBase* evb_;
};

class FirstRequestProcessorSink : public SinkClientCallback,
                                  private SinkServerCallback {
 public:
  FirstRequestProcessorSink(
      uint16_t protocolId,
      folly::StringPiece methodName,
      SinkClientCallback* clientCallback,
      folly::EventBase* evb)
      : protocolId_(protocolId),
        methodName_(methodName),
        clientCallback_(clientCallback),
        evb_(evb) {}

  FOLLY_NODISCARD bool onFirstResponse(
      FirstResponsePayload&& firstResponse,
      folly::EventBase* evb,
      SinkServerCallback* serverCallback) override {
    SCOPE_EXIT {
      delete this;
    };
    if (auto error = processFirstResponse(
            firstResponse.metadata,
            firstResponse.payload,
            protocolId_,
            methodName_)) {
      serverCallback->onSinkError(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::INTERRUPTION,
              "process first response error"));
      clientCallback_->onFirstResponseError(std::move(error));
      return false;
    }
    serverCallback->resetClientCallback(*clientCallback_);
    return clientCallback_->onFirstResponse(
        std::move(firstResponse), evb, serverCallback);
  }
  void onFirstResponseError(folly::exception_wrapper ew) override {
    SCOPE_EXIT {
      delete this;
    };
    ew.handle(
        [&](rocket::RocketException& ex) {
          auto response =
              decodeResponseError(std::move(ex), protocolId_, methodName_);
          if (response.hasException()) {
            clientCallback_->onFirstResponseError(
                std::move(response).exception());
            return;
          }

          if (clientCallback_->onFirstResponse(
                  std::move(*response), evb_, this)) {
            DCHECK(clientCallback_);
            // This exception will be ignored, but we have to send it to follow
            // the contract.
            clientCallback_->onFinalResponseError(
                folly::make_exception_wrapper<TApplicationException>(
                    TApplicationException::INTERRUPTION,
                    "Initial response error"));
          }
        },
        [&](...) {
          clientCallback_->onFirstResponseError(std::move(ew));
          return false;
        });
  }

  void onFinalResponse(StreamPayload&&) override {
    std::terminate();
  }
  void onFinalResponseError(folly::exception_wrapper) override {
    std::terminate();
  }
  FOLLY_NODISCARD bool onSinkRequestN(uint64_t) override {
    std::terminate();
  }
  void resetServerCallback(SinkServerCallback&) override {
    std::terminate();
  }

  bool onSinkNext(StreamPayload&&) override {
    return true;
  }
  void onSinkError(folly::exception_wrapper) override {
    clientCallback_ = nullptr;
  }
  bool onSinkComplete() override {
    return true;
  }
  void resetClientCallback(SinkClientCallback& clientCallback) override {
    clientCallback_ = &clientCallback;
  }

 private:
  const uint16_t protocolId_;
  const std::string methodName_;
  SinkClientCallback* clientCallback_;
  folly::EventBase* evb_;
};

void setCompression(RequestRpcMetadata& metadata, ssize_t payloadSize) {
  if (auto compressionConfig = metadata.compressionConfig_ref()) {
    if (auto codecRef = compressionConfig->codecConfig_ref()) {
      if (payloadSize >
          compressionConfig->compressionSizeLimit_ref().value_or(0)) {
        switch (codecRef->getType()) {
          case CodecConfig::zlibConfig:
            metadata.compression_ref() = CompressionAlgorithm::ZLIB;
            break;
          case CodecConfig::zstdConfig:
            metadata.compression_ref() = CompressionAlgorithm::ZSTD;
            break;
          default:
            break;
        }
      }
    }
  }
}
} // namespace

class RocketClientChannel::SingleRequestSingleResponseCallback final
    : public rocket::RocketClient::RequestResponseCallback {
  using InflightGuardT =
      decltype(std::declval<RocketClientChannel>().inflightGuard());

 public:
  SingleRequestSingleResponseCallback(
      RequestClientCallback::Ptr cb,
      InflightGuardT g,
      uint16_t protocolId,
      std::string methodName,
      size_t requestSerializedSize,
      size_t requestWireSize)
      : cb_(std::move(cb)),
        g_(std::move(g)),
        protocolId_(protocolId),
        methodName_(std::move(methodName)),
        requestSerializedSize_(requestSerializedSize),
        requestWireSize_(requestWireSize) {}

  void onWriteSuccess() noexcept override {
    cb_->onRequestSent();
  }

  void onResponsePayload(
      folly::Try<rocket::Payload>&& payload) noexcept override {
    folly::Try<FirstResponsePayload> response;
    RpcSizeStats stats;
    stats.requestSerializedSizeBytes = requestSerializedSize_;
    stats.requestWireSizeBytes = requestWireSize_;
    if (payload.hasException()) {
      if (!payload.exception().with_exception<rocket::RocketException>(
              [&](auto& ex) {
                response = decodeResponseError(
                    std::move(ex), protocolId_, methodName_);
              })) {
        cb_.release()->onResponseError(std::move(payload.exception()));
        return;
      }
      if (response.hasException()) {
        cb_.release()->onResponseError(std::move(response.exception()));
        return;
      }
    } else {
      stats.responseWireSizeBytes =
          payload->metadataAndDataSize() - payload->metadataSize();

      response = rocket::unpack<FirstResponsePayload>(std::move(*payload));
      if (response.hasException()) {
        cb_.release()->onResponseError(std::move(response.exception()));
        return;
      }
      if (auto error = processFirstResponse(
              response->metadata,
              response->payload,
              protocolId_,
              methodName_)) {
        cb_.release()->onResponseError(std::move(error));
        return;
      }
    }

    stats.responseSerializedSizeBytes =
        response->payload->computeChainDataLength();

    auto tHeader = std::make_unique<transport::THeader>();
    tHeader->setClientType(THRIFT_ROCKET_CLIENT_TYPE);

    detail::fillTHeaderFromResponseRpcMetadata(response->metadata, *tHeader);
    cb_.release()->onResponse(ClientReceiveState(
        static_cast<uint16_t>(-1),
        std::move(response->payload),
        std::move(tHeader),
        nullptr, /* ctx */
        stats));
  }

 private:
  RequestClientCallback::Ptr cb_;
  InflightGuardT g_;
  const uint16_t protocolId_;
  std::string methodName_;
  const size_t requestSerializedSize_;
  const size_t requestWireSize_;
};

class RocketClientChannel::SingleRequestNoResponseCallback final
    : public rocket::RocketClient::RequestFnfCallback {
  using InflightGuardT =
      decltype(std::declval<RocketClientChannel>().inflightGuard());

 public:
  SingleRequestNoResponseCallback(
      RequestClientCallback::Ptr cb,
      InflightGuardT g)
      : cb_(std::move(cb)), g_(std::move(g)) {}

  void onWrite(folly::Try<void> writeResult) noexcept override {
    auto* cbPtr = cb_.release();
    if (writeResult.hasException()) {
      cbPtr->onResponseError(std::move(writeResult).exception());
    } else {
      cbPtr->onRequestSent();
    }
  }

 private:
  RequestClientCallback::Ptr cb_;
  InflightGuardT g_;
};

rocket::SetupFrame RocketClientChannel::makeSetupFrame(
    RequestSetupMetadata meta) {
  meta.maxVersion_ref() = 4;
  CompactProtocolWriter compactProtocolWriter;
  folly::IOBufQueue paramQueue;
  compactProtocolWriter.setOutput(&paramQueue);
  meta.write(&compactProtocolWriter);

  // Serialize RocketClient's major/minor version (which is separate from the
  // rsocket protocol major/minor version) into setup metadata.
  auto buf = folly::IOBuf::createCombined(
      sizeof(int32_t) + meta.serializedSize(&compactProtocolWriter));
  folly::IOBufQueue queue;
  queue.append(std::move(buf));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  // Serialize RocketClient's major/minor version (which is separate from the
  // rsocket protocol major/minor version) into setup metadata.
  appender.writeBE<uint16_t>(0); // Thrift RocketClient major version
  appender.writeBE<uint16_t>(1); // Thrift RocketClient minor version
  // Append serialized setup parameters to setup frame metadata
  appender.insert(paramQueue.move());

  return rocket::SetupFrame(
      rocket::Payload::makeFromMetadataAndData(queue.move(), {}));
}

RocketClientChannel::RocketClientChannel(
    folly::AsyncTransport::UniquePtr socket,
    RequestSetupMetadata meta)
    : evb_(socket->getEventBase()),
      rclient_(rocket::RocketClient::create(
          *evb_,
          std::move(socket),
          std::make_unique<rocket::SetupFrame>(
              makeSetupFrame(std::move(meta))))) {}

RocketClientChannel::~RocketClientChannel() {
  unsetOnDetachable();
  closeNow();
}

void RocketClientChannel::setFlushList(FlushList* flushList) {
  if (rclient_) {
    rclient_->setFlushList(flushList);
  }
}

void RocketClientChannel::setNegotiatedCompressionAlgorithm(
    CompressionAlgorithm compressionAlgo) {
  if (rclient_) {
    rclient_->setNegotiatedCompressionAlgorithm(compressionAlgo);
  }
}

void RocketClientChannel::setAutoCompressSizeLimit(int32_t size) {
  if (rclient_) {
    rclient_->setAutoCompressSizeLimit(size);
  }
}

RocketClientChannel::Ptr RocketClientChannel::newChannel(
    folly::AsyncTransport::UniquePtr socket,
    RequestSetupMetadata meta) {
  return RocketClientChannel::Ptr(
      new RocketClientChannel(std::move(socket), std::move(meta)));
}

void RocketClientChannel::sendRequestResponse(
    const RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cb) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE,
      methodName,
      std::move(request),
      std::move(header),
      std::move(cb));
}

void RocketClientChannel::sendRequestNoResponse(
    const RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cb) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_NO_RESPONSE,
      methodName,
      std::move(request),
      std::move(header),
      std::move(cb));
}

void RocketClientChannel::sendRequestStream(
    const RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<THeader> header,
    StreamClientCallback* clientCallback) {
  DestructorGuard dg(this);

  auto metadata = detail::makeRequestRpcMetadata(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE,
      static_cast<ProtocolId>(protocolId_),
      methodName,
      timeout_,
      *header,
      getPersistentWriteHeaders());

  std::chrono::milliseconds firstResponseTimeout;
  if (!preSendValidation(
          metadata, rpcOptions, clientCallback, firstResponseTimeout)) {
    return;
  }

  auto buf = std::move(request.buffer);
  setCompression(metadata, buf->computeChainDataLength());

  return rclient_->sendRequestStream(
      rocket::pack(metadata, std::move(buf)),
      firstResponseTimeout,
      rpcOptions.getChunkTimeout(),
      rpcOptions.getChunkBufferSize(),
      new FirstRequestProcessorStream(
          protocolId_, methodName, clientCallback, evb_));
}

void RocketClientChannel::sendRequestSink(
    const RpcOptions& rpcOptions,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    SinkClientCallback* clientCallback) {
  DestructorGuard dg(this);

  auto metadata = detail::makeRequestRpcMetadata(
      rpcOptions,
      RpcKind::SINK,
      static_cast<ProtocolId>(protocolId_),
      methodName,
      timeout_,
      *header,
      getPersistentWriteHeaders());

  std::chrono::milliseconds firstResponseTimeout;
  if (!preSendValidation(
          metadata, rpcOptions, clientCallback, firstResponseTimeout)) {
    return;
  }

  auto buf = std::move(request.buffer);
  setCompression(metadata, buf->computeChainDataLength());

  return rclient_->sendRequestSink(
      rocket::pack(metadata, std::move(buf)),
      firstResponseTimeout,
      new FirstRequestProcessorSink(
          protocolId_, methodName, clientCallback, evb_),
      rpcOptions.getEnablePageAlignment(),
      header->getDesiredCompressionConfig());
}

void RocketClientChannel::sendThriftRequest(
    const RpcOptions& rpcOptions,
    RpcKind kind,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cb) {
  DestructorGuard dg(this);

  auto metadata = detail::makeRequestRpcMetadata(
      rpcOptions,
      kind,
      static_cast<ProtocolId>(protocolId_),
      methodName,
      timeout_,
      *header,
      getPersistentWriteHeaders());

  std::chrono::milliseconds timeout;
  if (!preSendValidation(metadata, rpcOptions, cb, timeout)) {
    return;
  }

  auto buf = std::move(request.buffer);
  setCompression(metadata, buf->computeChainDataLength());

  switch (kind) {
    case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
      sendSingleRequestNoResponse(metadata, std::move(buf), std::move(cb));
      break;

    case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
      sendSingleRequestSingleResponse(
          metadata, timeout, std::move(buf), std::move(cb));
      break;

    case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
      // should no longer reach here anymore, use sendRequestStream
      DCHECK(false);
      break;

    default:
      folly::assume_unreachable();
  }
}

void RocketClientChannel::sendSingleRequestNoResponse(
    const RequestRpcMetadata& metadata,
    std::unique_ptr<folly::IOBuf> buf,
    RequestClientCallback::Ptr cb) {
  auto requestPayload = rocket::pack(metadata, std::move(buf));
  const bool isSync = cb->isSync();
  SingleRequestNoResponseCallback callback(std::move(cb), inflightGuard());

  if (isSync && folly::fibers::onFiber()) {
    callback.onWrite(rclient_->sendRequestFnfSync(std::move(requestPayload)));
  } else {
    rclient_->sendRequestFnf(
        std::move(requestPayload),
        folly::copy_to_unique_ptr(std::move(callback)));
  }
}

void RocketClientChannel::sendSingleRequestSingleResponse(
    const RequestRpcMetadata& metadata,
    std::chrono::milliseconds timeout,
    std::unique_ptr<folly::IOBuf> buf,
    RequestClientCallback::Ptr cb) {
  const auto requestSerializedSize = buf->computeChainDataLength();
  auto requestPayload = rocket::pack(metadata, std::move(buf));
  const auto requestWireSize = requestPayload.dataSize();
  const bool isSync = cb->isSync();
  SingleRequestSingleResponseCallback callback(
      std::move(cb),
      inflightGuard(),
      static_cast<uint16_t>(metadata.protocol_ref().value_unchecked()),
      metadata.name_ref().value_or({}),
      requestSerializedSize,
      requestWireSize);

  if (isSync && folly::fibers::onFiber()) {
    callback.onResponsePayload(rclient_->sendRequestResponseSync(
        std::move(requestPayload), timeout, &callback));
  } else {
    rclient_->sendRequestResponse(
        std::move(requestPayload),
        timeout,
        folly::copy_to_unique_ptr(std::move(callback)));
  }
}

void onResponseError(
    RequestClientCallback::Ptr& cb,
    folly::exception_wrapper ew) {
  cb.release()->onResponseError(std::move(ew));
}

void onResponseError(StreamClientCallback* cb, folly::exception_wrapper ew) {
  cb->onFirstResponseError(std::move(ew));
}

void onResponseError(SinkClientCallback* cb, folly::exception_wrapper ew) {
  cb->onFirstResponseError(std::move(ew));
}

template <typename CallbackPtr>
bool RocketClientChannel::preSendValidation(
    RequestRpcMetadata& metadata,
    const RpcOptions& rpcOptions,
    CallbackPtr& cb,
    std::chrono::milliseconds& firstResponseTimeout) {
  metadata.seqId_ref().reset();
  DCHECK(metadata.kind_ref().has_value());

  if (!rclient_) {
    // Channel destroyed by explicit closeNow() call.
    onResponseError(
        cb,
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::NOT_OPEN,
            "Connection not open: client destroyed due to closeNow."));
    return false;
  }

  if (!rclient_->isAlive()) {
    // Channel is not in connected state due to some pre-existing transport
    // exception, pass it back for some breadcrumbs.
    onResponseError(
        cb,
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::NOT_OPEN,
            folly::sformat(
                "Connection not open: {}",
                rclient_->getLastTransportError().what())));
    return false;
  }

  if (inflightRequestsAndStreams() >= maxInflightRequestsAndStreams_) {
    TTransportException ex(
        TTransportException::NETWORK_ERROR,
        "Too many active requests on connection");
    // Might be able to create another transaction soon
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);
    onResponseError(cb, std::move(ex));
    return false;
  }

  firstResponseTimeout =
      std::chrono::milliseconds(metadata.clientTimeoutMs_ref().value_or(0));
  if (rpcOptions.getClientOnlyTimeouts()) {
    metadata.clientTimeoutMs_ref().reset();
    metadata.queueTimeoutMs_ref().reset();
  }

  if (auto interactionId = rpcOptions.getInteractionId()) {
    evb_->dcheckIsInEventBaseThread();
    if (auto* name = folly::get_ptr(pendingInteractions_, interactionId)) {
      InteractionCreate create;
      create.set_interactionId(interactionId);
      create.set_interactionName(std::move(*name));
      metadata.interactionCreate_ref() = std::move(create);
      pendingInteractions_.erase(interactionId);
    } else {
      metadata.interactionId_ref() = interactionId;
    }
  }

  return true;
}

ClientChannel::SaturationStatus RocketClientChannel::getSaturationStatus() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  return ClientChannel::SaturationStatus(
      inflightRequestsAndStreams(), maxInflightRequestsAndStreams_);
}

void RocketClientChannel::closeNow() {
  DCHECK(!evb_ || evb_->isInEventBaseThread());
  rclient_.reset();
}

void RocketClientChannel::setCloseCallback(CloseCallback* closeCallback) {
  if (rclient_) {
    rclient_->setCloseCallback([closeCallback] {
      if (closeCallback) {
        closeCallback->channelClosed();
      }
    });
  }
}

folly::AsyncTransport* FOLLY_NULLABLE RocketClientChannel::getTransport() {
  if (!rclient_) {
    return nullptr;
  }

  auto* transportWrapper = rclient_->getTransportWrapper();
  return transportWrapper
      ? transportWrapper->getUnderlyingTransport<folly::AsyncTransport>()
      : nullptr;
}

bool RocketClientChannel::good() {
  DCHECK(!evb_ || evb_->isInEventBaseThread());
  return rclient_ && rclient_->isAlive();
}

size_t RocketClientChannel::inflightRequestsAndStreams() const {
  return shared_->inflightRequests + (rclient_ ? rclient_->streams() : 0);
}

void RocketClientChannel::setTimeout(uint32_t timeoutMs) {
  DCHECK(!evb_ || evb_->isInEventBaseThread());
  if (auto* transport = getTransport()) {
    transport->setSendTimeout(timeoutMs);
  }
  timeout_ = std::chrono::milliseconds(timeoutMs);
}

void RocketClientChannel::attachEventBase(folly::EventBase* evb) {
  DCHECK(evb->isInEventBaseThread());
  if (rclient_) {
    rclient_->attachEventBase(*evb);
  }
  evb_ = evb;
}

void RocketClientChannel::detachEventBase() {
  DCHECK(isDetachable());
  DCHECK(getDestructorGuardCount() == 0);

  if (rclient_) {
    rclient_->detachEventBase();
  }
  evb_ = nullptr;
}

bool RocketClientChannel::isDetachable() {
  DCHECK(!evb_ || evb_->isInEventBaseThread());
  auto* transport = getTransport();
  return !evb_ || !transport || !rclient_ || rclient_->isDetachable();
}

void RocketClientChannel::setOnDetachable(
    folly::Function<void()> onDetachable) {
  DCHECK(rclient_);
  ClientChannel::setOnDetachable(std::move(onDetachable));
  rclient_->setOnDetachable([this] {
    if (isDetachable()) {
      notifyDetachable();
    }
  });
}

void RocketClientChannel::unsetOnDetachable() {
  ClientChannel::unsetOnDetachable();
  if (rclient_) {
    rclient_->setOnDetachable(nullptr);
  }
}

void RocketClientChannel::terminateInteraction(InteractionId id) {
  evb_->dcheckIsInEventBaseThread();
  auto pending = pendingInteractions_.find(id);
  if (pending != pendingInteractions_.end()) {
    pendingInteractions_.erase(pending);
    releaseInteractionId(std::move(id));
    return;
  }
  // guard needed for onDetachable callback implementation
  auto guard = std::make_unique<SingleRequestNoResponseCallback>(
      nullptr, inflightGuard());
  rclient_->terminateInteraction(id, std::move(guard));
  releaseInteractionId(std::move(id));
}

void RocketClientChannel::registerInteraction(
    folly::StringPiece name,
    int64_t id) {
  CHECK(!name.empty());
  CHECK_GT(id, 0);
  evb_->dcheckIsInEventBaseThread();

  auto res = pendingInteractions_.insert({id, name.str()});
  DCHECK(res.second);
}

constexpr std::chrono::seconds RocketClientChannel::kDefaultRpcTimeout;
} // namespace thrift
} // namespace apache
