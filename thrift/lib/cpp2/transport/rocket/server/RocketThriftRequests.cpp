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

#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#if FOLLY_HAS_COROUTINES
#include <thrift/lib/cpp2/async/ServerSinkBridge.h>
#endif
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
template <typename ProtocolReader>
FOLLY_NODISCARD folly::exception_wrapper processFirstResponseHelper(
    ResponseRpcMetadata& metadata,
    std::unique_ptr<folly::IOBuf>& payload) noexcept {
  try {
    std::string methodNameIgnore;
    MessageType mtype;
    int32_t seqIdIgnore;
    ProtocolReader reader;
    reader.setInput(payload.get());
    reader.readMessageBegin(methodNameIgnore, mtype, seqIdIgnore);

    if (mtype != T_REPLY) {
      return {};
    }

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
      payloadMetadata.set_exceptionMetadata(std::move(exceptionMetadataBase));
    }
    metadata.payloadMetadata_ref() = std::move(payloadMetadata);
  } catch (...) {
    return TApplicationException(fmt::format(
        "Invalid response payload envelope: {}",
        folly::exceptionStr(std::current_exception()).toStdString()));
  }
  return {};
}

FOLLY_NODISCARD folly::exception_wrapper processFirstResponse(
    ResponseRpcMetadata& metadata,
    std::unique_ptr<folly::IOBuf>& payload,
    RocketServerFrameContext& frameContext,
    apache::thrift::protocol::PROTOCOL_TYPES protType,
    int32_t version) noexcept {
  if (!payload) {
    return {};
  }

  if (auto compression = frameContext.connection().getCompressionAlgorithm(
          payload->computeChainDataLength())) {
    metadata.compression_ref() = *compression;
  }

  DCHECK(version >= 0);
  if (version == 0) {
    return {};
  }

  switch (protType) {
    case protocol::T_BINARY_PROTOCOL:
      return processFirstResponseHelper<BinaryProtocolReader>(
          metadata, payload);
    case protocol::T_COMPACT_PROTOCOL:
      return processFirstResponseHelper<CompactProtocolReader>(
          metadata, payload);
    default:
      return TApplicationException("Invalid response payload protocol id");
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
    apache::thrift::MessageChannel::SendCallback* cb) noexcept {
  if (!data) {
    context_.sendError(
        RocketException(
            ErrorCode::INVALID, "serialization failed for response"),
        cb);
    return;
  }

  if (auto error = processFirstResponse(
          metadata, data, context_, getProtoId(), version_)) {
    sendErrorWrapped(std::move(error), kUnknownErrorCode);
    return;
  }

  context_.sendPayload(
      pack(metadata, std::move(data)),
      Flags::none().next(true).complete(true),
      cb);
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
    apache::thrift::MessageChannel::SendCallback*) noexcept {
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
  scheduleTimeouts();
}

void ThriftServerRequestStream::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::MessageChannel::SendCallback*) noexcept {
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
          metadata, data, context_, getProtoId(), version_)) {
    sendErrorWrapped(std::move(error), kUnknownErrorCode);
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
          metadata, data, context_, getProtoId(), version_)) {
    sendErrorWrapped(std::move(error), kUnknownErrorCode);
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
          metadata, exbuf, context_, getProtoId(), version_)) {
    sendErrorWrapped(std::move(error), kUnknownErrorCode);
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
  scheduleTimeouts();
}

void ThriftServerRequestSink::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::MessageChannel::SendCallback*) noexcept {
  LOG(FATAL) << "Sink requests must respond via sendSinkThriftResponse";
}

void ThriftServerRequestSink::sendSerializedError(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> exbuf) noexcept {
  if (auto error = processFirstResponse(
          metadata, exbuf, context_, getProtoId(), version_)) {
    sendErrorWrapped(std::move(error), kUnknownErrorCode);
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
          metadata, data, context_, getProtoId(), version_)) {
    sendErrorWrapped(std::move(error), kUnknownErrorCode);
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
