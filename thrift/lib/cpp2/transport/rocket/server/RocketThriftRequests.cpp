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

#include <thrift/lib/cpp2/async/SemiStream.h>
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
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    ActiveRequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    intptr_t rootRequestContextId,
    RocketServerFrameContext&& context)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      debugStub_(
          reqRegistry,
          *this,
          *getRequestContext(),
          std::move(debugPayload),
          rootRequestContextId) {
  scheduleTimeouts();
}

void ThriftServerRequestResponse::sendThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data) noexcept {
  // transform (e.g. compress) the response if needed
  RocketServerConnection& connection = context_.connection();
  folly::Optional<CompressionAlgorithm> compressionAlgo =
      connection.getNegotiatedCompressionAlgorithm();

  std::unique_ptr<folly::IOBuf> compressed;
  // only compress response if compressionAlgo is negotiated during TLS
  // handshake and the response size is greater than minCompressTypes
  if (compressionAlgo.hasValue() &&
      data->computeChainDataLength() >= connection.getMinCompressBytes()) {
    folly::io::CodecType compressCodec;
    switch (*compressionAlgo) {
      case CompressionAlgorithm::ZSTD:
        compressCodec = folly::io::CodecType::ZSTD;
        metadata.compression_ref() = *compressionAlgo;
        break;
      case CompressionAlgorithm::ZLIB:
        compressCodec = folly::io::CodecType::ZLIB;
        metadata.compression_ref() = *compressionAlgo;
        break;
      case CompressionAlgorithm::NONE:
        compressCodec = folly::io::CodecType::NO_COMPRESSION;
    }
    compressed = folly::io::getCodec(compressCodec)->compress(data.get());
  } else {
    compressed = std::move(data);
  }
  std::move(context_).sendPayload(
      makePayload(metadata, std::move(compressed)),
      Flags::none().next(true).complete(true));
}

void ThriftServerRequestResponse::sendStreamThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept {
  LOG(FATAL) << "Single-response requests cannot send stream responses";
}

ThriftServerRequestFnf::ThriftServerRequestFnf(
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    ActiveRequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    intptr_t rootRequestContextId,
    RocketServerFrameContext&& context,
    folly::Function<void()> onComplete)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      onComplete_(std::move(onComplete)),
      debugStub_(
          reqRegistry,
          *this,
          *getRequestContext(),
          std::move(debugPayload),
          rootRequestContextId) {
  scheduleTimeouts();
}

ThriftServerRequestFnf::~ThriftServerRequestFnf() {
  if (auto f = std::move(onComplete_)) {
    f();
  }
}

void ThriftServerRequestFnf::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "One-way requests cannot send responses";
}

void ThriftServerRequestFnf::sendStreamThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept {
  LOG(FATAL) << "One-way requests cannot send stream responses";
}

ThriftServerRequestStream::ThriftServerRequestStream(
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    std::shared_ptr<Cpp2ConnContext> connContext,
    ActiveRequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    intptr_t rootRequestContextId,
    RocketStreamClientCallback* clientCallback,
    std::shared_ptr<AsyncProcessor> cpp2Processor)
    : ThriftRequestCore(serverConfigs, std::move(metadata), *connContext),
      evb_(evb),
      clientCallback_(clientCallback),
      connContext_(std::move(connContext)),
      cpp2Processor_(std::move(cpp2Processor)),
      debugStub_(
          reqRegistry,
          *this,
          *getRequestContext(),
          std::move(debugPayload),
          rootRequestContextId) {
  scheduleTimeouts();
}

void ThriftServerRequestStream::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "Stream requests must respond via sendStreamThriftResponse";
}

void ThriftServerRequestStream::sendStreamReply(
    ResponseAndSemiStream<
        std::unique_ptr<folly::IOBuf>,
        std::unique_ptr<folly::IOBuf>>&& result,
    MessageChannel::SendCallback*,
    folly::Optional<uint32_t> crc32c) noexcept {
  if (result.stream) {
    auto* serverCallback =
        std::move(result.stream).toStreamServerCallbackPtr(*getEventBase());
    // Note that onSubscribe will run after onFirstResponse
    sendStreamReply(std::move(result.response), serverCallback, crc32c);
  } else {
    sendStreamReply(std::move(result.response), nullptr, crc32c);
  }
}

void ThriftServerRequestStream::sendStreamThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    StreamServerCallback* stream) noexcept {
  if (!stream) {
    sendStreamThriftError(std::move(metadata), std::move(data));
    return;
  }
  stream->resetClientCallback(*clientCallback_);
  clientCallback_->setProtoId(getProtoId());
  clientCallback_->onFirstResponse(
      FirstResponsePayload{std::move(data), std::move(metadata)},
      nullptr /* evb */,
      stream);
}

void ThriftServerRequestStream::sendStreamThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::detail::ServerStreamFactory&& stream) noexcept {
  clientCallback_->setProtoId(getProtoId());
  stream(
      apache::thrift::FirstResponsePayload{std::move(data),
                                           std::move(metadata)},
      clientCallback_,
      &evb_);
}

void ThriftServerRequestStream::sendStreamThriftError(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf> data) noexcept {
  std::exchange(clientCallback_, nullptr)
      ->onFirstResponseError(
          folly::make_exception_wrapper<thrift::detail::EncodedError>(
              std::move(data)));
}

ThriftServerRequestSink::ThriftServerRequestSink(
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    std::shared_ptr<Cpp2ConnContext> connContext,
    ActiveRequestsRegistry& reqRegistry,
    std::unique_ptr<folly::IOBuf> debugPayload,
    intptr_t rootRequestContextId,
    RocketSinkClientCallback* clientCallback,
    std::shared_ptr<AsyncProcessor> cpp2Processor)
    : ThriftRequestCore(serverConfigs, std::move(metadata), *connContext),
      evb_(evb),
      clientCallback_(clientCallback),
      connContext_(std::move(connContext)),
      cpp2Processor_(std::move(cpp2Processor)),
      debugStub_(
          reqRegistry,
          *this,
          *getRequestContext(),
          std::move(debugPayload),
          rootRequestContextId) {
  scheduleTimeouts();
}

void ThriftServerRequestSink::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "Sink requests must respond via sendSinkThriftResponse";
}

void ThriftServerRequestSink::sendStreamThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept {
  LOG(FATAL) << "Sink requests cannot send stream responses";
}

#if FOLLY_HAS_COROUTINES
void ThriftServerRequestSink::sendSinkThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::detail::SinkConsumerImpl&& sinkConsumer) noexcept {
  if (sinkConsumer) {
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
