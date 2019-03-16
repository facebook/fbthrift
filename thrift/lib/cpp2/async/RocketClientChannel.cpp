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
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClientWriteCallback.h>
#include <thrift/lib/cpp2/transport/rocket/util/RpcMetadataUtil.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

using namespace apache::thrift::transport;

namespace apache {
namespace thrift {

namespace {
class OnWriteSuccess final : public rocket::RocketClientWriteCallback {
 public:
  explicit OnWriteSuccess(RequestCallback& requestCallback)
      : requestCallback_(requestCallback) {}

  void onWriteSuccess() noexcept override {
    folly::RequestContextScopeGuard rctx(requestCallback_.context_);
    requestCallback_.requestSent();
  }

 private:
  RequestCallback& requestCallback_;
};

std::unique_ptr<folly::IOBuf> serializeMetadata(
    const RequestRpcMetadata& requestMetadata) {
  CompactProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  requestMetadata.write(&writer);
  return queue.move();
}

void deserializeMetadata(
    ResponseRpcMetadata& dest,
    const folly::IOBuf& buffer) {
  CompactProtocolReader reader;
  reader.setInput(&buffer);
  dest.read(&reader);
}

std::unique_ptr<ResponseRpcMetadata> deserializeMetadata(
    const folly::IOBuf& buffer) {
  auto responseMetadata = std::make_unique<ResponseRpcMetadata>();
  deserializeMetadata(*responseMetadata, buffer);
  return responseMetadata;
}
} // namespace

RocketClientChannel::RocketClientChannel(
    async::TAsyncTransport::UniquePtr socket)
    : evb_(socket->getEventBase()),
      rclient_(rocket::RocketClient::create(*evb_, std::move(socket))) {}

RocketClientChannel::~RocketClientChannel() {
  unsetOnDetachable();
  inflightState_->unsetOnDetachable();
  closeNow();
}

RocketClientChannel::Ptr RocketClientChannel::newChannel(
    async::TAsyncTransport::UniquePtr socket) {
  return RocketClientChannel::Ptr(new RocketClientChannel(std::move(socket)));
}

uint32_t RocketClientChannel::sendRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return 0;
}

uint32_t RocketClientChannel::sendOnewayRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_NO_RESPONSE,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return ResponseChannel::ONEWAY_REQUEST_ID;
}

uint32_t RocketClientChannel::sendStreamRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return 0;
}

void RocketClientChannel::sendThriftRequest(
    RpcOptions& rpcOptions,
    RpcKind kind,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  DestructorGuard dg(this);

  cb->context_ = folly::RequestContext::saveContext();
  auto metadata = detail::makeRequestRpcMetadata(
      rpcOptions,
      kind,
      static_cast<ProtocolId>(protocolId_),
      timeout_,
      *header,
      getPersistentWriteHeaders());

  if (!EnvelopeUtil::stripEnvelope(&metadata, buf)) {
    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestError(ClientReceiveState(
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::CORRUPTED_DATA,
            "Unexpected problem stripping envelope"),
        std::move(ctx)));
    return;
  }
  metadata.seqId_ref() = 0;
  DCHECK(metadata.kind_ref().has_value());

  if (!rclient_ || !rclient_->isAlive()) {
    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestError(ClientReceiveState(
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::NOT_OPEN, "Connection is not open"),
        std::move(ctx)));
    return;
  }

  if (!inflightState_->incPendingRequests()) {
    TTransportException ex(
        TTransportException::NETWORK_ERROR,
        "Too many active requests on connection");

    // Might be able to create another transaction soon
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);

    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestError(ClientReceiveState(std::move(ex), std::move(ctx)));
    return;
  }

  switch (kind) {
    case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
      sendSingleRequestNoResponse(
          metadata, std::move(ctx), std::move(buf), std::move(cb));
      break;

    case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
      sendSingleRequestSingleResponse(
          metadata, std::move(ctx), std::move(buf), std::move(cb));
      break;

    case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
      sendSingleRequestStreamResponse(
          metadata,
          std::move(ctx),
          std::move(buf),
          std::move(cb),
          rpcOptions.getChunkTimeout());
      break;

    default:
      folly::assume_unreachable();
  }
}

void RocketClientChannel::sendSingleRequestNoResponse(
    const RequestRpcMetadata& metadata,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<RequestCallback> cb) {
  auto& cbRef = *cb;
  auto& fm = getFiberManager();

  fm.addTaskFinally(
      [&cbRef,
       rclient = rclient_,
       requestPayload = rocket::Payload::makeFromMetadataAndData(
           serializeMetadata(metadata), std::move(buf))]() mutable {
        OnWriteSuccess writeCallback(cbRef);
        return rclient->sendRequestFnfSync(
            std::move(requestPayload), &writeCallback);
      },
      [cb = std::move(cb),
       ctx = std::move(ctx),
       inflightWeak = folly::to_weak_ptr(inflightState_)](
          folly::Try<void>&& result) mutable {
        if (auto inflightState = inflightWeak.lock()) {
          inflightState->decPendingRequests();
        }
        if (result.hasException()) {
          folly::RequestContextScopeGuard rctx(cb->context_);
          cb->requestError(ClientReceiveState(
              std::move(result.exception()),
              folly::to_shared_ptr(std::move(ctx))));
        }
      });
}

void RocketClientChannel::sendSingleRequestSingleResponse(
    const RequestRpcMetadata& metadata,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<RequestCallback> cb) {
  auto& cbRef = *cb;
  auto& fm = getFiberManager();

  fm.addTaskFinally(
      [&cbRef,
       timeout =
           std::chrono::milliseconds(metadata.clientTimeoutMs_ref().value()),
       rclient = rclient_,
       requestPayload = rocket::Payload::makeFromMetadataAndData(
           serializeMetadata(metadata), std::move(buf))]() mutable {
        OnWriteSuccess writeCallback(cbRef);
        return rclient->sendRequestResponseSync(
            std::move(requestPayload), timeout, &writeCallback);
      },
      [ctx = std::move(ctx),
       cb = std::move(cb),
       protocolId = protocolId_,
       inflightWeak = folly::to_weak_ptr(inflightState_)](
          folly::Try<rocket::Payload>&& response) mutable {
        folly::RequestContextScopeGuard rctx(cb->context_);

        if (auto inflightState = inflightWeak.lock()) {
          inflightState->decPendingRequests();
        }
        if (UNLIKELY(response.hasException())) {
          cb->requestError(ClientReceiveState(
              std::move(response.exception()), std::move(ctx)));
          return;
        }

        auto tHeader = std::make_unique<transport::THeader>();
        tHeader->setClientType(THRIFT_HTTP_CLIENT_TYPE);

        if (response.value().hasNonemptyMetadata()) {
          ResponseRpcMetadata responseMetadata;
          try {
            deserializeMetadata(responseMetadata, *response.value().metadata());
            if (responseMetadata.otherMetadata_ref().has_value()) {
              tHeader->setReadHeaders(
                  std::move(responseMetadata.otherMetadata_ref().value()));
            }
          } catch (const std::exception& e) {
            FB_LOG_EVERY_MS(ERROR, 10000)
                << "Exception on deserializing metadata: "
                << folly::exceptionStr(e);
            cb->requestError(ClientReceiveState(
                folly::exception_wrapper(std::current_exception(), e),
                std::move(ctx)));
            return;
          }
        }

        cb->replyReceived(ClientReceiveState(
            protocolId,
            std::move(response.value()).data(),
            std::move(tHeader),
            std::move(ctx)));
      });
}

void RocketClientChannel::sendSingleRequestStreamResponse(
    const RequestRpcMetadata& metadata,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<RequestCallback> cb,
    std::chrono::milliseconds chunkTimeout) {
  std::shared_ptr<rocket::RocketClientFlowable> flowable;
  try {
    flowable = rclient_->createStream(rocket::Payload::makeFromMetadataAndData(
        serializeMetadata(metadata), std::move(buf)));
  } catch (const std::exception& e) {
    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestError(ClientReceiveState(
        folly::exception_wrapper(std::current_exception(), e), std::move(ctx)));
    return;
  }

  // Note that at this point, no RPC has been sent or has even been
  // scheduled to be sent. This is similar to how requestSent() behaves in
  // RSocketClientChannel.
  {
    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestSent();
  }

  auto callback = std::make_unique<ThriftClientCallback>(
      evb_, std::move(cb), std::move(ctx), protocolId_, chunkTimeout);

  auto takeFirst = std::make_shared<TakeFirst>(
      *evb_,
      std::move(callback),
      chunkTimeout,
      folly::to_weak_ptr(inflightState_));

  flowable->subscribe(std::move(takeFirst));
}

ClientChannel::SaturationStatus RocketClientChannel::getSaturationStatus() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  return ClientChannel::SaturationStatus(
      inflightState_->inflightRequests(),
      inflightState_->maxInflightRequests());
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
  return !evb_ || !transport || !rclient_ ||
      (inflightState_->inflightRequests() == 0 && rclient_->isDetachable());
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

RocketClientChannel::TakeFirst::TakeFirst(
    folly::EventBase& evb,
    std::unique_ptr<ThriftClientCallback> clientCallback,
    std::chrono::milliseconds chunkTimeout,
    std::weak_ptr<InflightState> inflightState)
    : evb_(evb),
      clientCallback_(std::move(clientCallback)),
      chunkTimeout_(chunkTimeout),
      inflightWeak_(std::move(inflightState)) {}

RocketClientChannel::TakeFirst::~TakeFirst() {
  if (auto subscription = std::move(subscription_)) {
    subscription->cancel();
  }
}

void RocketClientChannel::TakeFirst::cancel() {
  if (auto subscription = std::move(subscription_)) {
    subscription->cancel();
  }
  onError(
      folly::make_exception_wrapper<std::runtime_error>("TakeFirst canceled"));
}

void RocketClientChannel::TakeFirst::onSubscribe(
    std::shared_ptr<yarpl::flowable::Subscription> subscription) {
  subscription_ = std::move(subscription);
  subscription_->request(1);
}

void RocketClientChannel::TakeFirst::subscribe(
    std::shared_ptr<yarpl::flowable::Subscriber<U>> subscriber) {
  if (auto subscription = std::move(subscription_)) {
    subscriber_ = std::move(subscriber);
    subscriber_->onSubscribe(std::move(subscription));
    if (completeBeforeSubscribed_) {
      onComplete();
    }
    if (errorBeforeSubscribed_) {
      onError(std::move(errorBeforeSubscribed_));
    }
  } else {
    throw std::logic_error(
        "TakeFirst subscribed to without active subscription");
  }
}

void RocketClientChannel::TakeFirst::onComplete() {
  if (awaitingFirstResponse_) {
    return onError(folly::make_exception_wrapper<std::runtime_error>(
        "TakeFirst received no initial response"));
  }

  if (auto subscriber = std::move(subscriber_)) {
    subscriber->onComplete();
  } else {
    completeBeforeSubscribed_ = true;
  }

  onStreamTerminated();
}

void RocketClientChannel::TakeFirst::onNormalFirstResponse(
    rocket::Payload&& firstPayload,
    std::shared_ptr<yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>>
        tail) {
  if (chunkTimeout_ != std::chrono::milliseconds::zero()) {
    tail = tail->timeout(evb_, chunkTimeout_, chunkTimeout_, [] {
      return transport::TTransportException(
          transport::TTransportException::TTransportExceptionType::TIMED_OUT);
    });
  }

  std::unique_ptr<ResponseRpcMetadata> metadata;
  try {
    metadata = firstPayload.hasNonemptyMetadata()
        ? deserializeMetadata(*firstPayload.metadata())
        : std::make_unique<ResponseRpcMetadata>();
  } catch (const std::exception& ex) {
    FB_LOG_EVERY_MS(ERROR, 10000)
        << "Exception on deserializing metadata: " << folly::exceptionStr(ex);
    onErrorFirstResponse(
        folly::exception_wrapper(std::current_exception(), ex));
    return;
  }

  auto cb = std::move(clientCallback_);
  cb->onThriftResponse(
      std::move(metadata),
      std::move(firstPayload).data(),
      toStream(std::move(tail), &evb_));
}

void RocketClientChannel::TakeFirst::onErrorFirstResponse(
    folly::exception_wrapper ew) {
  if (auto inflight = inflightWeak_.lock()) {
    inflight->decPendingRequests();
  }
  inflightWeak_.reset();
  auto cb = std::move(clientCallback_);
  cb->onError(std::move(ew));
}

void RocketClientChannel::TakeFirst::onStreamTerminated() {
  if (auto inflight = inflightWeak_.lock()) {
    inflight->decPendingRequests();
  }
  inflightWeak_.reset();
}

void RocketClientChannel::TakeFirst::onError(folly::exception_wrapper ew) {
  if (awaitingFirstResponse_) {
    return onErrorFirstResponse(std::move(ew));
  }

  SCOPE_EXIT {
    onStreamTerminated();
  };

  if (auto subscriber = std::move(subscriber_)) {
    folly::exception_wrapper hijacked;
    if (ew.with_exception([&hijacked](rocket::RocketException& rex) {
          hijacked = folly::exception_wrapper(
              apache::thrift::detail::EncodedError(rex.moveErrorData()));
        })) {
      subscriber->onError(std::move(hijacked));
    } else {
      subscriber->onError(std::move(ew));
    }
  } else {
    errorBeforeSubscribed_ = std::move(ew);
  }
}

void RocketClientChannel::TakeFirst::onNext(TakeFirst::T payload) {
  // Used for breaking the cycle between Subscription and Subscriber when
  // the response Flowable is not subscribed at all.
  class SafeFlowable : public Flowable<U> {
   public:
    explicit SafeFlowable(std::shared_ptr<TakeFirst> inner)
        : inner_(std::move(inner)) {}

    ~SafeFlowable() override {
      if (auto inner = std::move(inner_)) {
        inner->cancel();
      }
    }

    void subscribe(
        std::shared_ptr<yarpl::flowable::Subscriber<U>> subscriber) override {
      if (auto inner = std::move(inner_)) {
        inner->subscribe(std::move(subscriber));
      } else {
        throw std::logic_error(
            "Cannot subscribe to SafeFlowable more than once");
      }
    }

   private:
    std::shared_ptr<TakeFirst> inner_;
  };

  if (awaitingFirstResponse_) {
    awaitingFirstResponse_ = false;
    onNormalFirstResponse(
        std::move(payload),
        std::make_shared<SafeFlowable>(this->ref_from_this(this)));
  } else {
    DCHECK(subscriber_);
    subscriber_->onNext(std::move(payload).data());
  }
}

constexpr std::chrono::milliseconds RocketClientChannel::kDefaultRpcTimeout;

} // namespace thrift
} // namespace apache
