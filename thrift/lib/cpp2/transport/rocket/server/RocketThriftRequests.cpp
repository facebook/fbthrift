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
#include <folly/compression/Compression.h>
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

ThriftServerRequestResponse::ThriftServerRequestResponse(
    RequestsRegistry::DebugStub& debugStubToInit,
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    RequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    intptr_t rootRequestContextId,
    RocketServerFrameContext&& context)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)) {
  new (&debugStubToInit) RequestsRegistry::DebugStub(
      reqRegistry,
      *this,
      *getRequestContext(),
      getProtoId(),
      std::move(debugPayload),
      rootRequestContextId);
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

  // transform (e.g. compress) the response if needed
  RocketServerConnection& connection = context_.connection();
  folly::Optional<CompressionAlgorithm> compressionAlgo =
      connection.getNegotiatedCompressionAlgorithm();
  // only compress response if compressionAlgo is negotiated during TLS
  // handshake and the response size is greater than minCompressTypes
  if (compressionAlgo.has_value() &&
      data->computeChainDataLength() >= connection.getMinCompressBytes()) {
    compressPayload(metadata, data, *compressionAlgo);
  }
  context_.sendPayload(
      makePayload(metadata, std::move(data)),
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
    RequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    intptr_t rootRequestContextId,
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
      getProtoId(),
      std::move(debugPayload),
      rootRequestContextId);
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
    RequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    intptr_t rootRequestContextId,
    RocketServerFrameContext&& context,
    RocketStreamClientCallback* clientCallback,
    std::shared_ptr<AsyncProcessor> cpp2Processor)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      clientCallback_(clientCallback),
      cpp2Processor_(std::move(cpp2Processor)) {
  new (&debugStubToInit) RequestsRegistry::DebugStub(
      reqRegistry,
      *this,
      *getRequestContext(),
      getProtoId(),
      std::move(debugPayload),
      rootRequestContextId);
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
  context_.unsetMarkRequestComplete();
  clientCallback_->setProtoId(getProtoId());
  stream(
      apache::thrift::FirstResponsePayload{std::move(data),
                                           std::move(metadata)},
      clientCallback_,
      &evb_);
}

void ThriftServerRequestStream::sendSerializedError(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf> exbuf) noexcept {
  std::exchange(clientCallback_, nullptr)
      ->onFirstResponseError(
          folly::make_exception_wrapper<thrift::detail::EncodedError>(
              std::move(exbuf)));
}

ThriftServerRequestSink::ThriftServerRequestSink(
    RequestsRegistry::DebugStub& debugStubToInit,
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    RequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    intptr_t rootRequestContextId,
    RocketServerFrameContext&& context,
    RocketSinkClientCallback* clientCallback,
    std::shared_ptr<AsyncProcessor> cpp2Processor)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      clientCallback_(clientCallback),
      cpp2Processor_(std::move(cpp2Processor)) {
  new (&debugStubToInit) RequestsRegistry::DebugStub(
      reqRegistry,
      *this,
      *getRequestContext(),
      getProtoId(),
      std::move(debugPayload),
      rootRequestContextId);
  scheduleTimeouts();
}

void ThriftServerRequestSink::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::MessageChannel::SendCallback*) noexcept {
  LOG(FATAL) << "Sink requests must respond via sendSinkThriftResponse";
}

void ThriftServerRequestSink::sendSerializedError(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf> exbuf) noexcept {
  std::exchange(clientCallback_, nullptr)
      ->onFirstResponseError(
          folly::make_exception_wrapper<thrift::detail::EncodedError>(
              std::move(exbuf)));
}

#if FOLLY_HAS_COROUTINES
void ThriftServerRequestSink::sendSinkThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::detail::SinkConsumerImpl&& sinkConsumer) noexcept {
  if (sinkConsumer) {
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
  } else {
    std::exchange(clientCallback_, nullptr)
        ->onFirstResponseError(
            folly::make_exception_wrapper<thrift::detail::EncodedError>(
                std::move(data)));
  }
}
#endif

} // namespace rocket
} // namespace thrift
} // namespace apache
