/*
 * Copyright 2015-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
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
#include <folly/fibers/FiberManager.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/Request.h>

#include <thrift/lib/cpp/async/TAsyncTransport.h>
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
    async::TAsyncTransport::UniquePtr socket,
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
    async::TAsyncTransport::UniquePtr socket,
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
  sendRequestStreaming(
      rpcOptions,
      std::move(buf),
      std::move(header),
      clientCallback,
      &rocket::RocketClient::sendRequestStream);
}

void RocketClientChannel::sendRequestSink(
    RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header,
    SinkClientCallback* clientCallback) {
  sendRequestStreaming(
      rpcOptions,
      std::move(buf),
      std::move(header),
      clientCallback,
      &rocket::RocketClient::sendRequestSink);
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

  if (!EnvelopeUtil::stripEnvelope(&metadata, buf)) {
    cb.release()->onResponseError(
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::CORRUPTED_DATA,
            "Unexpected problem stripping envelope"));
    return;
  }
  metadata.seqId_ref() = 0;
  DCHECK(metadata.kind_ref().has_value());

  const std::chrono::milliseconds timeout{
      metadata.clientTimeoutMs_ref().value_or(0)};
  if (rpcOptions.getClientOnlyTimeouts()) {
    metadata.clientTimeoutMs_ref().reset();
    metadata.queueTimeoutMs_ref().reset();
  }

  if (!rclient_ || !rclient_->isAlive()) {
    cb.release()->onResponseError(
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::NOT_OPEN, "Connection is not open"));
    return;
  }

  if (inflightRequestsAndStreams() >= maxInflightRequestsAndStreams_) {
    TTransportException ex(
        TTransportException::NETWORK_ERROR,
        "Too many active requests on connection");
    // Might be able to create another transaction soon
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);
    cb.release()->onResponseError(std::move(ex));
    return;
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
    tHeader->setClientType(THRIFT_HTTP_CLIENT_TYPE);

    if (response.value().hasNonemptyMetadata()) {
      ResponseRpcMetadata responseMetadata;
      try {
        deserializeMetadata(responseMetadata, response.value());
        detail::fillTHeaderFromResponseRpcMetadata(responseMetadata, *tHeader);
      } catch (const std::exception& e) {
        FB_LOG_EVERY_MS(ERROR, 10000) << "Exception on deserializing metadata: "
                                      << folly::exceptionStr(e);
        cb.release()->onResponseError(
            folly::exception_wrapper(std::current_exception(), e));
        return;
      }
    }

    cb.release()->onResponse(ClientReceiveState(
        -1, std::move(response.value()).data(), std::move(tHeader), nullptr));
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

template <typename ClientCallback, typename F>
void RocketClientChannel::sendRequestStreaming(
    RpcOptions& rpcOptions,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<THeader> header,
    ClientCallback* clientCallback,
    F method) {
  DestructorGuard dg(this);

  auto metadata = detail::makeRequestRpcMetadata(
      rpcOptions,
      std::is_same<SinkClientCallback, ClientCallback>::value
          ? RpcKind::SINK
          : RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE,
      static_cast<ProtocolId>(protocolId_),
      timeout_,
      *header,
      getPersistentWriteHeaders());

  if (!EnvelopeUtil::stripEnvelope(&metadata, buf)) {
    return clientCallback->onFirstResponseError(
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::CORRUPTED_DATA,
            "Unexpected problem stripping envelope"));
  }

  // Safe to remove after all servers update.
  metadata.seqId_ref() = 0;

  if (!rclient_ || !rclient_->isAlive()) {
    return clientCallback->onFirstResponseError(
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::NOT_OPEN, "Connection is not open"));
  }

  if (inflightRequestsAndStreams() >= maxInflightRequestsAndStreams_) {
    TTransportException ex(
        TTransportException::NETWORK_ERROR,
        "Too many active requests on connection");
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);
    return clientCallback->onFirstResponseError(
        folly::exception_wrapper(std::move(ex)));
  }

  const std::chrono::milliseconds firstResponseTimeout{
      metadata.clientTimeoutMs_ref().value_or(0)};
  if (rpcOptions.getClientOnlyTimeouts()) {
    metadata.clientTimeoutMs_ref().reset();
    metadata.queueTimeoutMs_ref().reset();
  }

  getFiberManager().addTask(
      [rclient = rclient_,
       firstResponseTimeout,
       clientCallback,
       method,
       payload = rocket::makePayload(metadata, std::move(buf))]() mutable {
        return (*rclient.*method)(
            std::move(payload), firstResponseTimeout, clientCallback);
      });
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
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::NOT_OPEN, "Channel closing now"));
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

async::TAsyncTransport* FOLLY_NULLABLE RocketClientChannel::getTransport() {
  if (!rclient_) {
    return nullptr;
  }

  auto* transportWrapper = rclient_->getTransportWrapper();
  return transportWrapper
      ? transportWrapper->getUnderlyingTransport<async::TAsyncTransport>()
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
