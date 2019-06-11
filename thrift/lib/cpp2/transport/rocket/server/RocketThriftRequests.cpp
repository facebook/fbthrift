/*
 * Copyright 2018-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rocket/server/RocketThriftRequests.h>

#include <memory>

#include <folly/Function.h>
#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

#include <thrift/lib/cpp2/async/SemiStream.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerStreamSubscriber.h>

#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

#include <yarpl/flowable/Flowable.h>
#include <yarpl/flowable/Subscriber.h>

namespace apache {
namespace thrift {
namespace rocket {

namespace {
std::unique_ptr<folly::IOBuf> serializeMetadata(
    const ResponseRpcMetadata& responseMetadata) {
  CompactProtocolWriter writer;
  // Default is to leave some headroom for rsocket headers
  size_t serSize = responseMetadata.serializedSizeZC(&writer);
  constexpr size_t kHeadroomBytes = 16;
  constexpr size_t kMinAllocBytes = 1024;
  auto buf =
      folly::IOBuf::create(std::max(kHeadroomBytes + serSize, kMinAllocBytes));
  buf->advance(kHeadroomBytes);
  folly::IOBufQueue queue;
  queue.append(std::move(buf));
  writer.setOutput(&queue);
  responseMetadata.write(&writer);
  return queue.move();
}

class SubscriberAdaptor final
    : public SubscriberIf<std::unique_ptr<folly::IOBuf>>,
      public yarpl::flowable::Subscription,
      public folly::HHWheelTimer::Callback,
      public std::enable_shared_from_this<SubscriberAdaptor> {
 public:
  SubscriberAdaptor(
      folly::EventBase* evb,
      std::shared_ptr<yarpl::flowable::Subscriber<Payload>> impl,
      Payload response,
      std::chrono::milliseconds starvationTimeout)
      : evb_(evb),
        impl_(std::move(impl)),
        response_(std::move(response)),
        starvationTimeout_(starvationTimeout) {}

  void onSubscribe(std::unique_ptr<SubscriptionIf> subscription) override {
    subscription_ = std::move(subscription);

    impl_->onSubscribe(shared_from_this());

    if (requestCount_ == 0 &&
        starvationTimeout_ != std::chrono::milliseconds::zero()) {
      evb_->timer().scheduleTimeout(this, starvationTimeout_);
    }
  }

  void request(int64_t n) override {
    if (n <= 0) {
      return;
    }

    if (auto firstResponse = std::move(response_)) {
      impl_->onNext(std::move(*firstResponse));
      --n;
    }

    requestCount_ = yarpl::credits::add(requestCount_, n);
    if (requestCount_ > 0) {
      cancelTimeout();
      subscription_->request(n);
    }
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
    impl_->onNext(Payload::makeFromData(std::move(value)));

    yarpl::credits::consume(requestCount_, 1);
    if (requestCount_ == 0 &&
        starvationTimeout_ != std::chrono::milliseconds::zero()) {
      evb_->timer().scheduleTimeout(this, starvationTimeout_);
    }
  }

  void onComplete() override {
    cancelTimeout();
    if (auto impl = std::exchange(impl_, nullptr)) {
      impl->onComplete();
    }
  }

  void onError(folly::exception_wrapper ew) override {
    cancelTimeout();
    if (auto impl = std::exchange(impl_, nullptr)) {
      folly::exception_wrapper hijacked;
      if (ew.with_exception([&hijacked](thrift::detail::EncodedError& err) {
            hijacked = folly::make_exception_wrapper<RocketException>(
                ErrorCode::APPLICATION_ERROR, std::move(err.encoded));
          })) {
        impl->onError(std::move(hijacked));
      } else {
        impl->onError(std::move(ew));
      }
    }
  }

  void timeoutExpired() noexcept override {
    if (requestCount_ == 0) {
      if (auto subscription = std::exchange(subscription_, nullptr)) {
        subscription->cancel();
      }
      onError(folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::TApplicationExceptionType::TIMEOUT));
    }
  }

 private:
  folly::EventBase* evb_;
  std::shared_ptr<yarpl::flowable::Subscriber<Payload>> impl_;
  folly::Optional<Payload> response_;
  // If the subscriber receives no onNext() signal for a duration of
  // starvationTimeout_, we'll terminate the stream with onError().
  const std::chrono::milliseconds starvationTimeout_;

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

std::shared_ptr<yarpl::flowable::Flowable<Payload>> toFlowable(
    SemiStream<std::unique_ptr<folly::IOBuf>> stream,
    folly::EventBase& evb,
    Payload&& response,
    std::chrono::milliseconds starvationTimeout) {
  return yarpl::flowable::internal::flowableFromSubscriber<Payload>(
      [stream = std::move(stream),
       &evb = evb,
       response = std::move(response),
       timeout = starvationTimeout](auto subscriber) mutable {
        std::move(stream).via(&evb).subscribe(
            std::make_unique<UniqueSubscriberAdaptor>(
                std::make_shared<SubscriberAdaptor>(
                    &evb,
                    std::move(subscriber),
                    std::move(response),
                    timeout)));
      });
}
} // namespace

ThriftServerRequestResponse::ThriftServerRequestResponse(
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    RocketServerFrameContext&& context)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)) {
  scheduleTimeouts();
}

void ThriftServerRequestResponse::sendThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data) noexcept {
  auto responsePayload = Payload::makeFromMetadataAndData(
      serializeMetadata(metadata), std::move(data));
  std::move(context_).sendPayload(
      std::move(responsePayload), Flags::none().next(true).complete(true));
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
    RocketServerFrameContext&& context,
    folly::Function<void()> onComplete)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      onComplete_(std::move(onComplete)) {
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
    std::shared_ptr<RocketServerStreamSubscriber> subscriber,
    std::shared_ptr<AsyncProcessor> cpp2Processor)
    : ThriftRequestCore(serverConfigs, std::move(metadata), *connContext),
      evb_(evb),
      subscriber_(std::move(subscriber)),
      connContext_(std::move(connContext)),
      cpp2Processor_(std::move(cpp2Processor)) {
  scheduleTimeouts();
}

void ThriftServerRequestStream::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "Stream requests must respond via sendStreamThriftResponse";
}

void ThriftServerRequestStream::sendStreamThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    SemiStream<std::unique_ptr<folly::IOBuf>> stream) noexcept {
  auto response = Payload::makeFromMetadataAndData(
      serializeMetadata(metadata), std::move(data));

  if (stream) {
    const auto timeout = serverConfigs_.getStreamExpireTime();
    toFlowable(std::move(stream), *getEventBase(), std::move(response), timeout)
        ->subscribe(std::move(subscriber_));
  } else {
    yarpl::flowable::Flowable<Payload>::justOnce(std::move(response))
        ->subscribe(std::move(subscriber_));
  }
}

} // namespace rocket
} // namespace thrift
} // namespace apache
