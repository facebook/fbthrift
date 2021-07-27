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
#include <thrift/lib/cpp2/server/LoggingEvent.h>
#include <thrift/lib/cpp2/transport/core/RpcMetadataUtil.h>
#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
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
      case ResponseRpcErrorCode::UNKNOWN_INTERACTION_ID:
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
    ResponseRpcMetadata& metadata, int32_t version) {
  DCHECK_GE(version, 4);

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
    metadata.proxiedPayloadMetadata_ref() = ProxiedPayloadMetadata();

    otherMetadata.insert({"ex", std::move(*pexPtr)});
    otherMetadata.erase("pex");
  }

  if (auto proxiedErrorPtr =
          folly::get_ptr(otherMetadata, "servicerouter:sr_error")) {
    metadata.proxiedPayloadMetadata_ref() = ProxiedPayloadMetadata();

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
      case MessageType::T_REPLY: {
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
          PayloadDeclaredExceptionMetadata declaredExceptionMetadata;
          if (auto otherMetadataRef = metadata.otherMetadata_ref()) {
            // defined in sync with
            // thrift/lib/cpp2/transport/core/RpcMetadataUtil.h

            // Setting user exception name and content
            static const auto uex =
                std::string(apache::thrift::detail::kHeaderUex);
            if (auto uexPtr = folly::get_ptr(*otherMetadataRef, uex)) {
              exceptionMetadataBase.name_utf8_ref() = *uexPtr;
              otherMetadataRef->erase(uex);
            }
            static const auto uexw =
                std::string(apache::thrift::detail::kHeaderUexw);
            if (auto uexwPtr = folly::get_ptr(*otherMetadataRef, uexw)) {
              exceptionMetadataBase.what_utf8_ref() = *uexwPtr;
              otherMetadataRef->erase(uexw);
            }

            // Setting user declared exception classification
            static const auto exMeta =
                std::string(apache::thrift::detail::kHeaderExMeta);
            if (auto metaPtr = folly::get_ptr(*otherMetadataRef, exMeta)) {
              ErrorClassification errorClassification =
                  apache::thrift::detail::deserializeErrorClassification(
                      *metaPtr);
              declaredExceptionMetadata.errorClassification_ref() =
                  std::move(errorClassification);
            }
          }
          PayloadExceptionMetadata exceptionMetadata;
          exceptionMetadata.set_declaredException(
              std::move(declaredExceptionMetadata));
          exceptionMetadataBase.metadata_ref() = std::move(exceptionMetadata);
          payloadMetadata.set_exceptionMetadata(
              std::move(exceptionMetadataBase));
        }
        metadata.payloadMetadata_ref() = std::move(payloadMetadata);
        break;
      }
      case MessageType::T_EXCEPTION: {
        DCHECK_GE(version, 2);

        preprocessProxiedExceptionHeaders(metadata, version);

        TApplicationException ex;
        ::apache::thrift::detail::deserializeExceptionBody(&reader, &ex);

        PayloadExceptionMetadataBase exceptionMetadataBase;
        exceptionMetadataBase.what_utf8_ref() = ex.getMessage();

        auto otherMetadataRef = metadata.otherMetadata_ref();
        DCHECK(
            !otherMetadataRef ||
            !folly::get_ptr(*otherMetadataRef, "servicerouter:sr_error"));
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
        } else {
          DCHECK_GE(version, 3);

          auto exPtr = otherMetadataRef
              ? folly::get_ptr(*otherMetadataRef, "ex")
              : nullptr;
          auto uexPtr = otherMetadataRef
              ? folly::get_ptr(*otherMetadataRef, "uex")
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
                       {kMethodUnknownErrorCode,
                        ResponseRpcErrorCode::UNKNOWN_METHOD},
                       {kRequestTypeDoesntMatchServiceFunctionType,
                        ResponseRpcErrorCode::WRONG_RPC_KIND},
                       {kInteractionIdUnknownErrorCode,
                        ResponseRpcErrorCode::UNKNOWN_INTERACTION_ID},
                       {kInteractionConstructorErrorErrorCode,
                        ResponseRpcErrorCode::INTERACTION_CONSTRUCTOR_ERROR},
                       {kRequestParsingErrorCode,
                        ResponseRpcErrorCode::REQUEST_PARSING_FAILURE},
                       {kChecksumMismatchErrorCode,
                        ResponseRpcErrorCode::CHECKSUM_MISMATCH}});
                  if (auto errorCode = folly::get_ptr(errorCodeMap, *exPtr)) {
                    return *errorCode;
                  }
                }

                return folly::none;
              }()) {
            return makeResponseRpcError(*errorCode, ex.getMessage(), metadata);
          }
          if (uexPtr) {
            exceptionMetadataBase.name_utf8_ref() = *uexPtr;
            otherMetadataRef->erase("uex");
          }
          PayloadExceptionMetadata exceptionMetadata;
          if (exPtr && *exPtr == kAppClientErrorCode) {
            if (version < 8) {
              exceptionMetadata.set_DEPRECATED_appClientException(
                  PayloadAppClientExceptionMetadata());
            } else {
              PayloadAppUnknownExceptionMetdata aue;
              aue.errorClassification_ref().ensure().blame_ref() =
                  ErrorBlame::CLIENT;
              exceptionMetadata.set_appUnknownException(std::move(aue));
            }
          } else {
            if (version < 8) {
              exceptionMetadata.set_DEPRECATED_appServerException(
                  PayloadAppServerExceptionMetadata());
            } else {
              PayloadAppUnknownExceptionMetdata aue;
              aue.errorClassification_ref().ensure().blame_ref() =
                  ErrorBlame::SERVER;
              exceptionMetadata.set_appUnknownException(std::move(aue));
            }
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
        DCHECK_GE(version, 3);
        return makeResponseRpcError(
            ResponseRpcErrorCode::UNKNOWN, "Invalid message type", metadata);
    }
  } catch (...) {
    DCHECK_GE(version, 3);
    return makeResponseRpcError(
        ResponseRpcErrorCode::UNKNOWN,
        fmt::format(
            "Invalid response payload envelope: {}",
            folly::exceptionStr(std::current_exception()).toStdString()),
        metadata);
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

  THRIFT_APPLICATION_EVENT(server_write_headers).log([&] {
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

  // apply compression if client has specified compression codec
  if (compressionConfig.has_value()) {
    rocket::detail::setCompressionCodec(
        *compressionConfig, metadata, payload->computeChainDataLength());
  }

  DCHECK_GE(version, 1);
  switch (protType) {
    case protocol::T_BINARY_PROTOCOL:
      return processFirstResponseHelper<BinaryProtocolReader>(
          metadata, payload, version);
    case protocol::T_COMPACT_PROTOCOL:
      return processFirstResponseHelper<CompactProtocolReader>(
          metadata, payload, version);
    default: {
      DCHECK_GE(version, 3);
      return makeResponseRpcError(
          ResponseRpcErrorCode::UNKNOWN,
          "Invalid response payload protocol id",
          metadata);
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
    rocket::Payload&& debugPayload,
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
      std::move(debugPayload),
      stateMachine_);
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

void ThriftServerRequestResponse::sendThriftException(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::MessageChannel::SendCallbackPtr cb) noexcept {
  sendThriftResponse(std::move(metadata), std::move(data), std::move(cb));
}

void ThriftServerRequestResponse::sendSerializedError(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> exbuf) noexcept {
  sendThriftResponse(std::move(metadata), std::move(exbuf), nullptr);
}

void ThriftServerRequestResponse::closeConnection(
    folly::exception_wrapper ew) noexcept {
  context_.connection().close(std::move(ew));
}

ThriftServerRequestFnf::ThriftServerRequestFnf(
    RequestsRegistry::DebugStub& debugStubToInit,
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    std::shared_ptr<folly::RequestContext> rctx,
    RequestsRegistry& reqRegistry,
    rocket::Payload&& debugPayload,
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
      std::move(debugPayload),
      stateMachine_);
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

void ThriftServerRequestFnf::sendThriftException(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::MessageChannel::SendCallbackPtr cb) noexcept {
  sendThriftResponse(std::move(metadata), std::move(data), std::move(cb));
}

void ThriftServerRequestFnf::sendSerializedError(
    ResponseRpcMetadata&&, std::unique_ptr<folly::IOBuf>) noexcept {}

void ThriftServerRequestFnf::closeConnection(
    folly::exception_wrapper ew) noexcept {
  context_.connection().close(std::move(ew));
}

ThriftServerRequestStream::ThriftServerRequestStream(
    RequestsRegistry::DebugStub& debugStubToInit,
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    std::shared_ptr<folly::RequestContext> rctx,
    RequestsRegistry& reqRegistry,
    rocket::Payload&& debugPayload,
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
      std::move(debugPayload),
      stateMachine_);
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

void ThriftServerRequestStream::sendThriftException(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::MessageChannel::SendCallbackPtr) noexcept {
  sendSerializedError(std::move(metadata), std::move(data));
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
  if (!stream) {
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
  clientCallback_->setProtoId(getProtoId());
  stream(
      apache::thrift::FirstResponsePayload{
          std::move(data), std::move(metadata)},
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

void ThriftServerRequestStream::closeConnection(
    folly::exception_wrapper ew) noexcept {
  context_.connection().close(std::move(ew));
}

ThriftServerRequestSink::ThriftServerRequestSink(
    RequestsRegistry::DebugStub& debugStubToInit,
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    std::shared_ptr<folly::RequestContext> rctx,
    RequestsRegistry& reqRegistry,
    rocket::Payload&& debugPayload,
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
      std::move(debugPayload),
      stateMachine_);
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

void ThriftServerRequestSink::sendThriftException(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::MessageChannel::SendCallbackPtr) noexcept {
  sendSerializedError(std::move(metadata), std::move(data));
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
      &apache::thrift::detail::ServerSinkBridge::start,
      std::move(serverCallback))
      .scheduleOn(executor)
      .start();
}

bool ThriftServerRequestSink::sendSinkThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    SinkServerCallbackPtr serverCallback) noexcept {
  if (!serverCallback) {
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
  serverCallback->resetClientCallback(*clientCallback_);
  clientCallback_->setProtoId(getProtoId());
  return clientCallback_->onFirstResponse(
      FirstResponsePayload{std::move(data), std::move(metadata)},
      nullptr, /* evb */
      serverCallback.release());
}
#endif

void ThriftServerRequestSink::closeConnection(
    folly::exception_wrapper ew) noexcept {
  context_.connection().close(std::move(ew));
}

} // namespace rocket
} // namespace thrift
} // namespace apache
