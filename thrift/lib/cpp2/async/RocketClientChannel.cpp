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
#include <thrift/lib/cpp2/transport/rocket/client/RocketClientWriteCallback.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

using namespace apache::thrift::transport;

namespace apache {
namespace thrift {

namespace {
class OnWriteSuccess final : public rocket::RocketClientWriteCallback {
 public:
  explicit OnWriteSuccess(RequestClientCallback& requestCallback)
      : requestCallback_(requestCallback) {}

  void onWriteSuccess() noexcept override {
    requestCallback_.onRequestSent();
  }

 private:
  RequestClientCallback& requestCallback_;
};

void deserializeMetadata(ResponseRpcMetadata& dest, const rocket::Payload& p) {
  CompactProtocolReader reader;
  reader.setInput(p.buffer());
  dest.read(&reader);
  if (reader.getCursorPosition() > p.metadataSize()) {
    folly::throw_exception<std::out_of_range>("underflow");
  }
}
} // namespace

rocket::SetupFrame RocketClientChannel::makeSetupFrame(
    RequestSetupMetadata meta) {
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
    folly::AsyncTransportWrapper::UniquePtr socket,
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

RocketClientChannel::Ptr RocketClientChannel::newChannel(
    folly::AsyncTransportWrapper::UniquePtr socket,
    RequestSetupMetadata meta) {
  return RocketClientChannel::Ptr(
      new RocketClientChannel(std::move(socket), std::move(meta)));
}

void RocketClientChannel::sendRequestResponse(
    RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cb) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE,
      std::move(buf),
      std::move(header),
      std::move(cb));
}

void RocketClientChannel::sendRequestNoResponse(
    RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cb) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_NO_RESPONSE,
      std::move(buf),
      std::move(header),
      std::move(cb));
}

void RocketClientChannel::sendRequestStream(
    RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<THeader> header,
    StreamClientCallback* clientCallback) {
  DestructorGuard dg(this);

  auto metadata = detail::makeRequestRpcMetadata(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE,
      static_cast<ProtocolId>(protocolId_),
      timeout_,
      *header,
      getPersistentWriteHeaders());

  std::chrono::milliseconds firstResponseTimeout;
  if (!preSendValidation(
          metadata, rpcOptions, buf, clientCallback, firstResponseTimeout)) {
    return;
  }

  return rclient_->sendRequestStream(
      rocket::makePayload(metadata, std::move(buf)),
      firstResponseTimeout,
      rpcOptions.getChunkTimeout(),
      rpcOptions.getChunkBufferSize(),
      clientCallback);
}

void RocketClientChannel::sendRequestSink(
    RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header,
    SinkClientCallback* clientCallback) {
  DestructorGuard dg(this);

  auto metadata = detail::makeRequestRpcMetadata(
      rpcOptions,
      RpcKind::SINK,
      static_cast<ProtocolId>(protocolId_),
      timeout_,
      *header,
      getPersistentWriteHeaders());

  std::chrono::milliseconds firstResponseTimeout;
  if (!preSendValidation(
          metadata, rpcOptions, buf, clientCallback, firstResponseTimeout)) {
    return;
  }

  return rclient_->sendRequestSink(
      rocket::makePayload(metadata, std::move(buf)),
      firstResponseTimeout,
      clientCallback);
}

void RocketClientChannel::sendThriftRequest(
    RpcOptions& rpcOptions,
    RpcKind kind,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cb) {
  DestructorGuard dg(this);

  auto metadata = detail::makeRequestRpcMetadata(
      rpcOptions,
      kind,
      static_cast<ProtocolId>(protocolId_),
      timeout_,
      *header,
      getPersistentWriteHeaders());

  std::chrono::milliseconds timeout;
  if (!preSendValidation(metadata, rpcOptions, buf, cb, timeout)) {
    return;
  }

  // compress the request if needed
  if (autoCompressSizeLimit_.hasValue() &&
      *autoCompressSizeLimit_ < int(buf->computeChainDataLength())) {
    if (negotiatedCompressionAlgo_.hasValue()) {
      rocket::compressRequest(metadata, buf, *negotiatedCompressionAlgo_);
    }
  }

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
  auto& cbRef = *cb;

  auto sendRequestFunc = [&cbRef,
                          rclient = rclient_,
                          requestPayload = rocket::makePayload(
                              metadata, std::move(buf))]() mutable {
    OnWriteSuccess writeCallback(cbRef);
    return rclient->sendRequestFnfSync(
        std::move(requestPayload), &writeCallback);
  };

  auto finallyFunc = [cb = std::move(cb),
                      g = inflightGuard()](folly::Try<void>&& result) mutable {
    if (result.hasException()) {
      cb.release()->onResponseError(std::move(result.exception()));
    } else {
      // onRequestSent is already called by the writeCallback.
      cb.release();
    }
  };

  if (cbRef.isSync() && folly::fibers::onFiber()) {
    finallyFunc(folly::makeTryWith(std::move(sendRequestFunc)));
  } else {
    auto& fm = getFiberManager();
    fm.addTaskFinally(
        std::move(sendRequestFunc),
        [finallyFunc = std::move(finallyFunc)](
            folly::Try<folly::Try<void>>&& arg) mutable {
          finallyFunc(collapseTry(std::move(arg)));
        });
  }
}

void RocketClientChannel::sendSingleRequestSingleResponse(
    const RequestRpcMetadata& metadata,
    std::chrono::milliseconds timeout,
    std::unique_ptr<folly::IOBuf> buf,
    RequestClientCallback::Ptr cb) {
  auto& cbRef = *cb;

  auto sendRequestFunc = [&cbRef,
                          timeout,
                          rclient = rclient_,
                          requestPayload = rocket::makePayload(
                              metadata, std::move(buf))]() mutable {
    OnWriteSuccess writeCallback(cbRef);
    return rclient->sendRequestResponseSync(
        std::move(requestPayload), timeout, &writeCallback);
  };

  auto finallyFunc = [cb = std::move(cb), g = inflightGuard()](
                         folly::Try<rocket::Payload>&& response) mutable {
    if (UNLIKELY(response.hasException())) {
      cb.release()->onResponseError(std::move(response.exception()));
      return;
    }

    auto tHeader = std::make_unique<transport::THeader>();
    tHeader->setClientType(THRIFT_ROCKET_CLIENT_TYPE);

    std::unique_ptr<folly::IOBuf> uncompressedResponse;
    if (response.value().hasNonemptyMetadata()) {
      ResponseRpcMetadata responseMetadata;
      try {
        deserializeMetadata(responseMetadata, response.value());
        detail::fillTHeaderFromResponseRpcMetadata(responseMetadata, *tHeader);
        if (auto compress = responseMetadata.compression_ref()) {
          folly::io::CodecType codec;
          switch (*compress) {
            case CompressionAlgorithm::ZSTD:
              codec = folly::io::CodecType::ZSTD;
              break;
            case CompressionAlgorithm::ZLIB:
              codec = folly::io::CodecType::ZLIB;
              break;
            case CompressionAlgorithm::NONE:
              codec = folly::io::CodecType::NO_COMPRESSION;
              break;
          }
          uncompressedResponse = folly::io::getCodec(codec)->uncompress(
              std::move(response.value()).data().get());
        } else {
          uncompressedResponse = std::move(response.value()).data();
        }
      } catch (const std::exception& e) {
        FB_LOG_EVERY_MS(ERROR, 10000) << "Exception on deserializing metadata: "
                                      << folly::exceptionStr(e);
        cb.release()->onResponseError(
            folly::exception_wrapper(std::current_exception(), e));
        return;
      }
    } else {
      uncompressedResponse = std::move(response.value()).data();
    }
    cb.release()->onResponse(ClientReceiveState(
        static_cast<uint16_t>(-1),
        std::move(uncompressedResponse),
        std::move(tHeader),
        nullptr));
  };

  if (cbRef.isSync() && folly::fibers::onFiber()) {
    finallyFunc(folly::makeTryWith(std::move(sendRequestFunc)));
  } else {
    auto& fm = getFiberManager();
    fm.addTaskFinally(
        std::move(sendRequestFunc),
        [finallyFunc = std::move(finallyFunc)](
            folly::Try<folly::Try<rocket::Payload>>&& arg) mutable {
          finallyFunc(collapseTry(std::move(arg)));
        });
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
    RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf>& buf,
    CallbackPtr& cb,
    std::chrono::milliseconds& firstResponseTimeout) {
  if (!EnvelopeUtil::stripEnvelope(&metadata, buf)) {
    onResponseError(
        cb,
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::CORRUPTED_DATA,
            "Unexpected problem stripping envelope"));
    return false;
  }
  metadata.seqId_ref().reset();
  DCHECK(metadata.kind_ref().has_value());

  if (!rclient_ || !rclient_->isAlive()) {
    onResponseError(
        cb,
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::NOT_OPEN, "Connection is not open"));
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
  return true;
}

ClientChannel::SaturationStatus RocketClientChannel::getSaturationStatus() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  return ClientChannel::SaturationStatus(
      inflightRequestsAndStreams(), maxInflightRequestsAndStreams_);
}

void RocketClientChannel::closeNow() {
  DCHECK(!evb_ || evb_->isInEventBaseThread());
  if (rclient_) {
    rclient_->closeNow(
        transport::TTransportException("RocketClientChannel::closeNow"));
    rclient_.reset();
  }
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

folly::AsyncTransportWrapper* FOLLY_NULLABLE
RocketClientChannel::getTransport() {
  if (!rclient_) {
    return nullptr;
  }

  auto* transportWrapper = rclient_->getTransportWrapper();
  return transportWrapper
      ? transportWrapper->getUnderlyingTransport<folly::AsyncTransportWrapper>()
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

constexpr std::chrono::milliseconds RocketClientChannel::kDefaultRpcTimeout;

} // namespace thrift
} // namespace apache
