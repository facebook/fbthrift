/*
 * Copyright 2017-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rsocket/server/RSThriftRequests.h>

#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <yarpl/flowable/Flowable.h>
#include <yarpl/single/SingleSubscriptions.h>

namespace apache {
namespace thrift {

namespace detail {
std::unique_ptr<folly::IOBuf> serializeMetadata(
    const ResponseRpcMetadata& responseMetadata) {
  CompactProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  responseMetadata.write(&writer);
  return queue.move();
}

std::unique_ptr<RequestRpcMetadata> deserializeMetadata(
    const folly::IOBuf& buffer) {
  CompactProtocolReader reader;
  auto metadata = std::make_unique<RequestRpcMetadata>();
  reader.setInput(&buffer);
  metadata->read(&reader);
  return metadata;
}
} // namespace detail

namespace {
class SubscriberAdaptor
    : public SubscriberIf<std::unique_ptr<folly::IOBuf>>,
      public yarpl::flowable::Subscription,
      public folly::HHWheelTimer::Callback,
      public std::enable_shared_from_this<SubscriberAdaptor> {
 public:
  explicit SubscriberAdaptor(
      folly::EventBase* evb,
      std::shared_ptr<yarpl::flowable::Subscriber<rsocket::Payload>> impl,
      rsocket::Payload response,
      std::chrono::milliseconds starvation)
      : evb_(evb),
        impl_(std::move(impl)),
        response_(std::move(response)),
        starvation_(std::move(starvation)) {}

  void onSubscribe(std::unique_ptr<SubscriptionIf> subscription) override {
    subscription_ = std::move(subscription);

    impl_->onSubscribe(shared_from_this());

    if (requestCount_ == 0 && starvation_.count() > 0) {
      evb_->timer().scheduleTimeout(this, starvation_);
    }
  }

  void request(int64_t n) override {
    if (n <= 0) {
      return;
    }
    if (auto firstResponse = std::move(response_)) {
      impl_->onNext(std::move(firstResponse));
      --n;
    }
    requestCount_ = yarpl::credits::add(requestCount_, n);

    if (requestCount_ > 0) {
      cancelTimeout();
    }
    subscription_->request(n);
  }

  void cancel() override {
    cancelTimeout();
    if (auto subscription = std::move(subscription_)) {
      subscription->cancel();
    }
  }

  void onNext(std::unique_ptr<folly::IOBuf>&& value) override {
    if (!impl_) {
      return;
    }
    impl_->onNext(rsocket::Payload(std::move(value)));

    yarpl::credits::consume(requestCount_, 1);
    if (requestCount_ == 0 && starvation_.count() > 0) {
      evb_->timer().scheduleTimeout(this, starvation_);
    }
  }

  void onComplete() override {
    cancelTimeout();
    if (auto impl = std::exchange(impl_, nullptr)) {
      impl->onComplete();
    }
  }

  void onError(folly::exception_wrapper e) override {
    cancelTimeout();
    if (auto impl = std::exchange(impl_, nullptr)) {
      impl->onError(std::move(e));
    }
  }

  void timeoutExpired() noexcept override {
    if (requestCount_ == 0) {
      if (auto subscription = std::exchange(subscription_, nullptr)) {
        subscription->cancel();
      }
      onError(TApplicationException(
          TApplicationException::TApplicationExceptionType::TIMEOUT));
    }
  }

 private:
  folly::EventBase* evb_;
  std::shared_ptr<yarpl::flowable::Subscriber<rsocket::Payload>> impl_;
  rsocket::Payload response_;
  std::chrono::milliseconds starvation_;

  int64_t requestCount_{0};
  std::unique_ptr<SubscriptionIf> subscription_;
};

// Adaptor for converting shared_ptr<SubscriberAdaptor> to unique_ptr.
class UniqueSubscriberAdaptor
    : public SubscriberIf<std::unique_ptr<folly::IOBuf>> {
 public:
  explicit UniqueSubscriberAdaptor(std::shared_ptr<SubscriberAdaptor> inner)
      : inner_(std::move(inner)) {}

  void onSubscribe(std::unique_ptr<SubscriptionIf> subscription) override {
    inner_->onSubscribe(std::move(subscription));
  }
  void onNext(std::unique_ptr<folly::IOBuf>&& value) override {
    inner_->onNext(std::move(value));
  }
  void onComplete() override {
    inner_->onComplete();
  }
  void onError(folly::exception_wrapper ex) override {
    inner_->onError(std::move(ex));
  }

 private:
  std::shared_ptr<SubscriberAdaptor> inner_;
};

std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>> toFlowableInternal(
    SemiStream<std::unique_ptr<folly::IOBuf>> stream,
    folly::EventBase* eventbase,
    rsocket::Payload response,
    std::chrono::milliseconds starvation) {
  return yarpl::flowable::internal::flowableFromSubscriber<rsocket::Payload>(
      [stream = std::move(stream),
       evb = eventbase,
       initResponse = std::move(response),
       starvationMs = std::move(starvation)](auto subscriber) mutable {
        std::move(stream).via(evb).subscribe(
            std::make_unique<UniqueSubscriberAdaptor>(
                std::make_shared<SubscriberAdaptor>(
                    evb,
                    std::move(subscriber),
                    std::move(initResponse),
                    std::move(starvationMs))));
      });
}
} // namespace

RSOneWayRequest::RSOneWayRequest(
    const apache::thrift::server::ServerConfigs& serverConfigs,
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<Cpp2ConnContext> connContext,
    folly::EventBase* evb,
    folly::Function<void(RSOneWayRequest*)> onDestroy)
    : ThriftRequestCore(
          serverConfigs,
          std::move(metadata),
          std::move(connContext)),
      evb_(evb),
      onDestroy_(std::move(onDestroy)) {
  scheduleTimeouts();
}

RSOneWayRequest::~RSOneWayRequest() {
  if (auto onDestroy = std::move(onDestroy_)) {
    onDestroy(this);
  }
}

void RSOneWayRequest::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata>,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "No response is allowed";
}

void RSOneWayRequest::sendStreamThriftResponse(
    std::unique_ptr<ResponseRpcMetadata>,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept {
  LOG(FATAL) << "Server should not call this function.";
}

folly::EventBase* RSOneWayRequest::getEventBase() noexcept {
  return evb_;
}

void RSOneWayRequest::cancel() {
  ThriftRequestCore::cancel();
  if (auto onDestroy = std::move(onDestroy_)) {
    onDestroy(this);
  }
}

RSSingleRequest::RSSingleRequest(
    const apache::thrift::server::ServerConfigs& serverConfigs,
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<Cpp2ConnContext> connContext,
    folly::EventBase* evb,
    std::shared_ptr<yarpl::single::SingleObserver<rsocket::Payload>>
        singleObserver)
    : ThriftRequestCore(
          serverConfigs,
          std::move(metadata),
          std::move(connContext)),
      evb_(evb),
      singleObserver_(singleObserver) {
  scheduleTimeouts();
}

void RSSingleRequest::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf) noexcept {
  DCHECK(evb_->isInEventBaseThread()) << "Should be called in IO thread";
  DCHECK(metadata);

  singleObserver_->onSubscribe(yarpl::single::SingleSubscriptions::empty());
  singleObserver_->onSuccess(
      rsocket::Payload(std::move(buf), detail::serializeMetadata(*metadata)));
  singleObserver_ = nullptr;
}

void RSSingleRequest::sendStreamThriftResponse(
    std::unique_ptr<ResponseRpcMetadata>,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept {
  LOG(FATAL) << "Server should not call this function.";
}

folly::EventBase* RSSingleRequest::getEventBase() noexcept {
  return evb_;
}

RSStreamRequest::RSStreamRequest(
    const apache::thrift::server::ServerConfigs& serverConfigs,
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<Cpp2ConnContext> connContext,
    folly::EventBase* evb,
    std::shared_ptr<yarpl::flowable::Subscriber<rsocket::Payload>> subscriber)
    : ThriftRequestCore(
          serverConfigs,
          std::move(metadata),
          std::move(connContext)),
      evb_(evb),
      subscriber_(std::move(subscriber)) {
  scheduleTimeouts();
}

void RSStreamRequest::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata>,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "Server should not call this function.";
}

void RSStreamRequest::sendStreamThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf,
    apache::thrift::SemiStream<std::unique_ptr<folly::IOBuf>> stream) noexcept {
  auto response =
      rsocket::Payload(std::move(buf), detail::serializeMetadata(*metadata));
  if (stream) {
    auto timeout = serverConfigs_.getStreamExpireTime();
    toFlowableInternal(std::move(stream), evb_, std::move(response), timeout)
        ->subscribe(std::move(subscriber_));
  } else {
    yarpl::flowable::Flowable<rsocket::Payload>::justOnce(std::move(response))
        ->subscribe(std::move(subscriber_));
  }
}

folly::EventBase* RSStreamRequest::getEventBase() noexcept {
  return evb_;
}

} // namespace thrift
} // namespace apache
