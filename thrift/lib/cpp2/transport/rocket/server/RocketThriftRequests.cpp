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

#include <thrift/lib/cpp2/transport/rocket/server/RocketThriftRequests.h>

#include <functional>
#include <memory>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/Function.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

#include <thrift/lib/cpp/protocol/TBase64Utils.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#if FOLLY_HAS_COROUTINES
#include <thrift/lib/cpp2/async/ServerSinkBridge.h>
#endif
#include <thrift/lib/cpp2/SerializationSwitch.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketSinkClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_constants.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {
namespace rocket {

namespace {
RocketException makeResponseRpcError(
    ResponseRpcErrorCode errorCode,
    folly::StringPiece message,
    const ResponseRpcMetadata& metadata) {
  ResponseRpcError responseRpcError;
  responseRpcError.name_utf8_ref() =
      apache::thrift::TEnumTraits<ResponseRpcErrorCode>::findName(errorCode);
  responseRpcError.what_utf8_ref() = message.str();
  responseRpcError.code_ref() = errorCode;
  auto category = [&] {
    switch (errorCode) {
      case ResponseRpcErrorCode::REQUEST_PARSING_FAILURE:
      case ResponseRpcErrorCode::WRONG_RPC_KIND:
      case ResponseRpcErrorCode::UNKNOWN_METHOD:
      case ResponseRpcErrorCode::CHECKSUM_MISMATCH:
        return ResponseRpcErrorCategory::INVALID_REQUEST;
      case ResponseRpcErrorCode::OVERLOAD:
      case ResponseRpcErrorCode::QUEUE_OVERLOADED:
      case ResponseRpcErrorCode::QUEUE_TIMEOUT:
      case ResponseRpcErrorCode::APP_OVERLOAD:
        return ResponseRpcErrorCategory::LOADSHEDDING;
      case ResponseRpcErrorCode::SHUTDOWN:
        return ResponseRpcErrorCategory::SHUTDOWN;
      default:
        return ResponseRpcErrorCategory::INTERNAL_ERROR;
    }
  }();
  responseRpcError.category_ref() = category;

  if (auto loadRef = metadata.load_ref()) {
    responseRpcError.load_ref() = *loadRef;
  }

  auto rocketCategory = [&] {
    switch (category) {
      case ResponseRpcErrorCategory::INVALID_REQUEST:
        return rocket::ErrorCode::INVALID;
      case ResponseRpcErrorCategory::LOADSHEDDING:
      case ResponseRpcErrorCategory::SHUTDOWN:
        return rocket::ErrorCode::REJECTED;
      default:
        return rocket::ErrorCode::CANCELED;
    }
  }();

  return RocketException(rocketCategory, packCompact(responseRpcError));
}

void preprocessProxiedExceptionHeaders(
    ResponseRpcMetadata& metadata,
    int32_t version) {
  if (version < 4) {
    return;
  }

  auto otherMetadataRef = metadata.otherMetadata_ref();
  if (!otherMetadataRef) {
    return;
  }
  auto& otherMetadata = *otherMetadataRef;

  if (auto puexPtr = folly::get_ptr(otherMetadata, "puex")) {
    metadata.proxiedPayloadMetadata_ref() = ProxiedPayloadMetadata();

    otherMetadata.insert({"uex", std::move(*puexPtr)});
    otherMetadata.erase("puex");
    if (auto puexwPtr = folly::get_ptr(otherMetadata, "puexw")) {
      otherMetadata.insert({"uexw", std::move(*puexwPtr)});
      otherMetadata.erase("puexw");
    }
  }

  if (auto pexPtr = folly::get_ptr(otherMetadata, "pex")) {
    metadata.set_proxiedPayloadMetadata(ProxiedPayloadMetadata());

    otherMetadata.insert({"ex", std::move(*pexPtr)});
    otherMetadata.erase("pex");
  }

  if (auto proxiedErrorPtr =
          folly::get_ptr(otherMetadata, "servicerouter:sr_error")) {
    metadata.set_proxiedPayloadMetadata(ProxiedPayloadMetadata());

    otherMetadata.insert(
        {"servicerouter:sr_internal_error", std::move(*proxiedErrorPtr)});
    otherMetadata.erase("servicerouter:sr_error");
  }
}

template <typename ProtocolReader>
FOLLY_NODISCARD folly::exception_wrapper processFirstResponseHelper(
    ResponseRpcMetadata& metadata,
    std::unique_ptr<folly::IOBuf>& payload,
    int32_t version) noexcept {
  try {
    std::string methodNameIgnore;
    MessageType mtype;
    int32_t seqIdIgnore;
    ProtocolReader reader;
    reader.setInput(payload.get());
    reader.readMessageBegin(methodNameIgnore, mtype, seqIdIgnore);

    switch (mtype) {
      case T_REPLY: {
        auto prefixSize = reader.getCursorPosition();

        protocol::TType ftype;
        int16_t fid;
        reader.readStructBegin(methodNameIgnore);
        reader.readFieldBegin(methodNameIgnore, ftype, fid);

        while (payload->length() < prefixSize) {
          prefixSize -= payload->length();
          payload = payload->pop();
        }
        payload->trimStart(prefixSize);

        PayloadMetadata payloadMetadata;
        if (fid == 0) {
          payloadMetadata.set_responseMetadata(PayloadResponseMetadata());
        } else {
          preprocessProxiedExceptionHeaders(metadata, version);

          PayloadExceptionMetadataBase exceptionMetadataBase;

          if (auto otherMetadataRef = metadata.otherMetadata_ref()) {
            if (auto uexPtr = folly::get_ptr(*otherMetadataRef, "uex")) {
              exceptionMetadataBase.name_utf8_ref() = *uexPtr;
              otherMetadataRef->erase("uex");
            }
            if (auto uexwPtr = folly::get_ptr(*otherMetadataRef, "uexw")) {
              exceptionMetadataBase.what_utf8_ref() = *uexwPtr;
              otherMetadataRef->erase("uexw");
            }
          }

          PayloadExceptionMetadata exceptionMetadata;
          exceptionMetadata.set_declaredException(
              PayloadDeclaredExceptionMetadata());

          exceptionMetadataBase.metadata_ref() = std::move(exceptionMetadata);
          payloadMetadata.set_exceptionMetadata(
              std::move(exceptionMetadataBase));
        }
        metadata.payloadMetadata_ref() = std::move(payloadMetadata);
        break;
      }
      case T_EXCEPTION: {
        if (version < 2) {
          return {};
        }

        preprocessProxiedExceptionHeaders(metadata, version);

        TApplicationException ex;
        ::apache::thrift::detail::deserializeExceptionBody(&reader, &ex);

        PayloadExceptionMetadataBase exceptionMetadataBase;
        exceptionMetadataBase.what_utf8_ref() = ex.getMessage();

        auto otherMetadataRef = metadata.otherMetadata_ref();
        if (auto proxyErrorPtr = otherMetadataRef
                ? folly::get_ptr(
                      *otherMetadataRef, "servicerouter:sr_internal_error")
                : nullptr) {
          exceptionMetadataBase.name_utf8_ref() = "ProxyException";
          PayloadExceptionMetadata exceptionMetadata;
          exceptionMetadata.set_proxyException(PayloadProxyExceptionMetadata());
          exceptionMetadataBase.metadata_ref() = std::move(exceptionMetadata);

          payload = protocol::base64Decode(*proxyErrorPtr);

          otherMetadataRef->erase("servicerouter:sr_internal_error");
          otherMetadataRef->erase("ex");
        } else if (
            auto proxiedErrorPtr = otherMetadataRef
                ? folly::get_ptr(*otherMetadataRef, "servicerouter:sr_error")
                : nullptr) {
          exceptionMetadataBase.name_utf8_ref() = "ProxiedException";
          PayloadExceptionMetadata exceptionMetadata;
          exceptionMetadata.set_proxiedException(
              PayloadProxiedExceptionMetadata());
          exceptionMetadataBase.metadata_ref() = std::move(exceptionMetadata);

          payload = protocol::base64Decode(*proxiedErrorPtr);

          otherMetadataRef->erase("servicerouter:sr_error");
          otherMetadataRef->erase("ex");
        } else {
          if (version < 3) {
            return {};
          }

          auto exPtr = otherMetadataRef
              ? folly::get_ptr(*otherMetadataRef, "ex")
              : nullptr;
          if (auto errorCode = [&]() -> folly::Optional<ResponseRpcErrorCode> {
                if (exPtr) {
                  if (*exPtr == kQueueOverloadedErrorCode &&
                      ex.getType() == TApplicationException::LOADSHEDDING) {
                    return ResponseRpcErrorCode::SHUTDOWN;
                  }

                  static const auto& errorCodeMap = *new std::unordered_map<
                      std::string,
                      ResponseRpcErrorCode>(
                      {{kUnknownErrorCode, ResponseRpcErrorCode::UNKNOWN},
                       {kOverloadedErrorCode, ResponseRpcErrorCode::OVERLOAD},
                       {kAppOverloadedErrorCode,
                        ResponseRpcErrorCode::APP_OVERLOAD},
                       {kTaskExpiredErrorCode,
                        ResponseRpcErrorCode::TASK_EXPIRED},
                       {kQueueOverloadedErrorCode,
                        ResponseRpcErrorCode::QUEUE_OVERLOADED},
                       {kInjectedFailureErrorCode,
                        ResponseRpcErrorCode::INJECTED_FAILURE},
                       {kServerQueueTimeoutErrorCode,
                        ResponseRpcErrorCode::QUEUE_TIMEOUT},
                       {kResponseTooBigErrorCode,
                        ResponseRpcErrorCode::RESPONSE_TOO_BIG},
                       {kRequestTypeDoesntMatchServiceFunctionType,
                        ResponseRpcErrorCode::WRONG_RPC_KIND}});
                  if (auto errorCode = folly::get_ptr(errorCodeMap, *exPtr)) {
                    return *errorCode;
                  }
                }

                switch (ex.getType()) {
                  case TApplicationException::UNKNOWN_METHOD:
                    return ResponseRpcErrorCode::UNKNOWN_METHOD;

                  case TApplicationException::INVALID_TRANSFORM:
                  case TApplicationException::UNSUPPORTED_CLIENT_TYPE:
                    return ResponseRpcErrorCode::REQUEST_PARSING_FAILURE;

                  case TApplicationException::CHECKSUM_MISMATCH:
                    return ResponseRpcErrorCode::CHECKSUM_MISMATCH;

                  case TApplicationException::INTERRUPTION:
                    return ResponseRpcErrorCode::INTERRUPTION;

                  default:
                    return folly::none;
                }
              }()) {
            return makeResponseRpcError(*errorCode, ex.getMessage(), metadata);
          }

          if (auto uexPtr = otherMetadataRef
                  ? folly::get_ptr(*otherMetadataRef, "uex")
                  : nullptr) {
            exceptionMetadataBase.name_utf8_ref() = *uexPtr;
            otherMetadataRef->erase("uex");
          }
          PayloadExceptionMetadata exceptionMetadata;
          if (exPtr && *exPtr == kAppClientErrorCode) {
            exceptionMetadata.set_appClientException(
                PayloadAppClientExceptionMetadata());
          } else {
            exceptionMetadata.set_appServerException(
                PayloadAppServerExceptionMetadata());
          }
          exceptionMetadataBase.metadata_ref() = std::move(exceptionMetadata);

          payload->clear();

          if (otherMetadataRef) {
            otherMetadataRef->erase("ex");
            otherMetadataRef->erase("uexw");
          }
        }

        PayloadMetadata payloadMetadata;
        payloadMetadata.set_exceptionMetadata(std::move(exceptionMetadataBase));
        metadata.payloadMetadata_ref() = std::move(payloadMetadata);

        break;
      }
      default:
        if (version < 3) {
          return {};
        }
        return makeResponseRpcError(
            ResponseRpcErrorCode::UNKNOWN, "Invalid message type", metadata);
    }
  } catch (...) {
    auto message = fmt::format(
        "Invalid response payload envelope: {}",
        folly::exceptionStr(std::current_exception()).toStdString());
    if (version < 3) {
      return TApplicationException(std::move(message));
    }
    return makeResponseRpcError(
        ResponseRpcErrorCode::UNKNOWN, message, metadata);
  }
  return {};
}

FOLLY_NODISCARD folly::exception_wrapper processFirstResponse(
    ResponseRpcMetadata& metadata,
    std::unique_ptr<folly::IOBuf>& payload,
    apache::thrift::protocol::PROTOCOL_TYPES protType,
    int32_t version,
    const folly::Optional<CompressionConfig>& compressionConfig) noexcept {
  if (!payload) {
    return makeResponseRpcError(
        ResponseRpcErrorCode::UNKNOWN,
        "serialization failed for response",
        metadata);
  }

  // apply compression if client has specified compression codec
  if (compressionConfig.has_value()) {
    detail::setCompressionCodec(
        *compressionConfig, metadata, payload->computeChainDataLength());
  }

  DCHECK(version >= 0);
  if (version == 0) {
    return {};
  }

  switch (protType) {
    case protocol::T_BINARY_PROTOCOL:
      return processFirstResponseHelper<BinaryProtocolReader>(
          metadata, payload, version);
    case protocol::T_COMPACT_PROTOCOL:
      return processFirstResponseHelper<CompactProtocolReader>(
          metadata, payload, version);
    default: {
      auto message = "Invalid response payload protocol id";
      if (version < 3) {
        return TApplicationException(std::move(message));
      }
      return makeResponseRpcError(
          ResponseRpcErrorCode::UNKNOWN, message, metadata);
    }
  }
}
} // namespace

ThriftServerRequestResponse::ThriftServerRequestResponse(
    RequestsRegistry::DebugStub& debugStubToInit,
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    std::shared_ptr<folly::RequestContext> rctx,
    RequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    RocketServerFrameContext&& context,
    int32_t version)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      version_(version) {
  new (&debugStubToInit) RequestsRegistry::DebugStub(
      reqRegistry,
      *this,
      *getRequestContext(),
      std::move(rctx),
      getProtoId(),
      std::move(debugPayload));
  scheduleTimeouts();
}

void ThriftServerRequestResponse::sendThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::MessageChannel::SendCallbackPtr cb) noexcept {
  if (auto error = processFirstResponse(
          metadata, data, getProtoId(), version_, getCompressionConfig())) {
    error.handle(
        [&](RocketException& ex) {
          context_.sendError(std::move(ex), std::move(cb));
        },
        [&](...) { sendErrorWrapped(std::move(error), kUnknownErrorCode); });
    return;
  }

  context_.sendPayload(
      pack(metadata, std::move(data)),
      Flags::none().next(true).complete(true),
      std::move(cb));
}

void ThriftServerRequestResponse::sendSerializedError(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> exbuf) noexcept {
  sendThriftResponse(std::move(metadata), std::move(exbuf), nullptr);
}

ThriftServerRequestFnf::ThriftServerRequestFnf(
    RequestsRegistry::DebugStub& debugStubToInit,
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    std::shared_ptr<folly::RequestContext> rctx,
    RequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    RocketServerFrameContext&& context,
    folly::Function<void()> onComplete)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      onComplete_(std::move(onComplete)) {
  new (&debugStubToInit) RequestsRegistry::DebugStub(
      reqRegistry,
      *this,
      *getRequestContext(),
      std::move(rctx),
      getProtoId(),
      std::move(debugPayload));
  scheduleTimeouts();
}

ThriftServerRequestFnf::~ThriftServerRequestFnf() {
  if (auto f = std::move(onComplete_)) {
    f();
  }
}

void ThriftServerRequestFnf::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::MessageChannel::SendCallbackPtr) noexcept {
  LOG(FATAL) << "One-way requests cannot send responses";
}

void ThriftServerRequestFnf::sendSerializedError(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>) noexcept {}

ThriftServerRequestStream::ThriftServerRequestStream(
    RequestsRegistry::DebugStub& debugStubToInit,
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    std::shared_ptr<folly::RequestContext> rctx,
    RequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    RocketServerFrameContext&& context,
    int32_t version,
    RocketStreamClientCallback* clientCallback,
    std::shared_ptr<AsyncProcessor> cpp2Processor)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      version_(version),
      clientCallback_(clientCallback),
      cpp2Processor_(std::move(cpp2Processor)) {
  new (&debugStubToInit) RequestsRegistry::DebugStub(
      reqRegistry,
      *this,
      *getRequestContext(),
      std::move(rctx),
      getProtoId(),
      std::move(debugPayload));
  if (auto compressionConfig = getCompressionConfig()) {
    clientCallback_->setCompressionConfig(*compressionConfig);
  }
  scheduleTimeouts();
}

void ThriftServerRequestStream::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::MessageChannel::SendCallbackPtr) noexcept {
  LOG(FATAL) << "Stream requests must respond via sendStreamThriftResponse";
}

bool ThriftServerRequestStream::sendStreamThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    StreamServerCallbackPtr stream) noexcept {
  if (!stream) {
    sendSerializedError(std::move(metadata), std::move(data));
    return false;
  }
  if (auto error = processFirstResponse(
          metadata, data, getProtoId(), version_, getCompressionConfig())) {
    error.handle(
        [&](RocketException& ex) {
          std::exchange(clientCallback_, nullptr)
              ->onFirstResponseError(std::move(ex));
        },
        [&](...) { sendErrorWrapped(std::move(error), kUnknownErrorCode); });
    return false;
  }
  context_.unsetMarkRequestComplete();
  stream->resetClientCallback(*clientCallback_);
  clientCallback_->setProtoId(getProtoId());
  return clientCallback_->onFirstResponse(
      FirstResponsePayload{std::move(data), std::move(metadata)},
      nullptr /* evb */,
      stream.release());
}

void ThriftServerRequestStream::sendStreamThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::detail::ServerStreamFactory&& stream) noexcept {
  if (auto error = processFirstResponse(
          metadata, data, getProtoId(), version_, getCompressionConfig())) {
    error.handle(
        [&](RocketException& ex) {
          std::exchange(clientCallback_, nullptr)
              ->onFirstResponseError(std::move(ex));
        },
        [&](...) { sendErrorWrapped(std::move(error), kUnknownErrorCode); });
    return;
  }
  context_.unsetMarkRequestComplete();
  clientCallback_->setProtoId(getProtoId());
  stream(
      apache::thrift::FirstResponsePayload{std::move(data),
                                           std::move(metadata)},
      clientCallback_,
      &evb_);
}

void ThriftServerRequestStream::sendSerializedError(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> exbuf) noexcept {
  if (auto error = processFirstResponse(
          metadata, exbuf, getProtoId(), version_, getCompressionConfig())) {
    error.handle(
        [&](RocketException& ex) {
          std::exchange(clientCallback_, nullptr)
              ->onFirstResponseError(std::move(ex));
        },
        [&](...) { sendErrorWrapped(std::move(error), kUnknownErrorCode); });
    return;
  }
  std::exchange(clientCallback_, nullptr)
      ->onFirstResponseError(folly::make_exception_wrapper<
                             thrift::detail::EncodedFirstResponseError>(
          FirstResponsePayload(std::move(exbuf), std::move(metadata))));
}

ThriftServerRequestSink::ThriftServerRequestSink(
    RequestsRegistry::DebugStub& debugStubToInit,
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    std::shared_ptr<folly::RequestContext> rctx,
    RequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    RocketServerFrameContext&& context,
    int32_t version,
    RocketSinkClientCallback* clientCallback,
    std::shared_ptr<AsyncProcessor> cpp2Processor)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      version_(version),
      clientCallback_(clientCallback),
      cpp2Processor_(std::move(cpp2Processor)) {
  new (&debugStubToInit) RequestsRegistry::DebugStub(
      reqRegistry,
      *this,
      *getRequestContext(),
      std::move(rctx),
      getProtoId(),
      std::move(debugPayload));
  if (auto compressionConfig = getCompressionConfig()) {
    clientCallback_->setCompressionConfig(*compressionConfig);
  }
  scheduleTimeouts();
}

void ThriftServerRequestSink::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::MessageChannel::SendCallbackPtr) noexcept {
  LOG(FATAL) << "Sink requests must respond via sendSinkThriftResponse";
}

void ThriftServerRequestSink::sendSerializedError(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> exbuf) noexcept {
  if (auto error = processFirstResponse(
          metadata, exbuf, getProtoId(), version_, getCompressionConfig())) {
    error.handle(
        [&](RocketException& ex) {
          std::exchange(clientCallback_, nullptr)
              ->onFirstResponseError(std::move(ex));
        },
        [&](...) { sendErrorWrapped(std::move(error), kUnknownErrorCode); });
    return;
  }
  std::exchange(clientCallback_, nullptr)
      ->onFirstResponseError(folly::make_exception_wrapper<
                             thrift::detail::EncodedFirstResponseError>(
          FirstResponsePayload(std::move(exbuf), std::move(metadata))));
}

#if FOLLY_HAS_COROUTINES
void ThriftServerRequestSink::sendSinkThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::detail::SinkConsumerImpl&& sinkConsumer) noexcept {
  if (!sinkConsumer) {
    sendSerializedError(std::move(metadata), std::move(data));
    return;
  }
  if (auto error = processFirstResponse(
          metadata, data, getProtoId(), version_, getCompressionConfig())) {
    error.handle(
        [&](RocketException& ex) {
          std::exchange(clientCallback_, nullptr)
              ->onFirstResponseError(std::move(ex));
        },
        [&](...) { sendErrorWrapped(std::move(error), kUnknownErrorCode); });
    return;
  }
  context_.unsetMarkRequestComplete();
  auto* executor = sinkConsumer.executor.get();
  clientCallback_->setProtoId(getProtoId());
  clientCallback_->setChunkTimeout(sinkConsumer.chunkTimeout);
  auto serverCallback = apache::thrift::detail::ServerSinkBridge::create(
      std::move(sinkConsumer), *getEventBase(), clientCallback_);
  clientCallback_->onFirstResponse(
      FirstResponsePayload{std::move(data), std::move(metadata)},
      nullptr /* evb */,
      serverCallback.get());

  folly::coro::co_invoke(
      [serverCallback =
           std::move(serverCallback)]() -> folly::coro::Task<void> {
        co_return co_await serverCallback->start();
      })
      .scheduleOn(executor)
      .start();
}
#endif

} // namespace rocket
} // namespace thrift
} // namespace apache
