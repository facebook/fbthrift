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
#include "thrift/lib/cpp2/transport/rsocket/client/RSClientThriftChannel.h"

#include <proxygen/lib/utils/WheelTimerInstance.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RPCSubscriber.h>

namespace apache {
namespace thrift {

using namespace rsocket;
using namespace yarpl::single;
using proxygen::WheelTimerInstance;

std::unique_ptr<folly::IOBuf> RSClientThriftChannel::serializeMetadata(
    const RequestRpcMetadata& requestMetadata) {
  CompactProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  requestMetadata.write(&writer);
  return queue.move();
}

std::unique_ptr<ResponseRpcMetadata> RSClientThriftChannel::deserializeMetadata(
    const folly::IOBuf& buffer) {
  CompactProtocolReader reader;
  auto responseMetadata = std::make_unique<ResponseRpcMetadata>();
  reader.setInput(&buffer);
  responseMetadata->read(&reader);
  return responseMetadata;
}

namespace {
// Never timeout in the default
static constexpr std::chrono::milliseconds kDefaultRequestTimeout =
    std::chrono::milliseconds(0);

// Adds a timer that timesout if the observer could not get its onSuccess or
// onError methods being called in a specified time range, which causes onError
// method to be called.
class TimedSingleObserver : public SingleObserver<Payload>,
                            public folly::HHWheelTimer::Callback {
 public:
  TimedSingleObserver(
      std::unique_ptr<ThriftClientCallback> callback,
      std::chrono::milliseconds timeout)
      : timeout_(timeout),
        timer_(timeout, callback->getEventBase()),
        callback_(std::move(callback)) {}

 protected:
  void onSubscribe(Reference<SingleSubscription> subscription) override {
    auto ref = this->ref_from_this(this);
    SingleObserver<Payload>::onSubscribe(std::move(subscription));

    if (timeout_.count() > 0) {
      auto evb = callback_->getEventBase();
      evb->runInEventBaseThread(
          [this]() { timer_.scheduleTimeout(this, timeout_); });
    }
  }

  void onSuccess(Payload payload) override {
    auto ref = this->ref_from_this(this);
    auto evb = callback_->getEventBase();
    evb->runInEventBaseThread([ref, payload = std::move(payload)]() mutable {
      if (ref->alreadySignalled_) {
        return;
      }
      ref->alreadySignalled_ = true;

      ref->cancelTimeout();
      ref->callback_->onThriftResponse(
          payload.metadata
              ? RSClientThriftChannel::deserializeMetadata(*payload.metadata)
              : std::make_unique<ResponseRpcMetadata>(),
          std::move(payload.data));
    });
    if (SingleObserver<Payload>::subscription()) {
      // TODO: can we get rid of calling the parent functions
      SingleObserver<Payload>::onSuccess({});
    }
  }

  void onError(folly::exception_wrapper ew) override {
    auto ref = this->ref_from_this(this);
    auto evb = callback_->getEventBase();
    evb->runInEventBaseThread([ref, ew = std::move(ew)]() mutable {
      if (ref->alreadySignalled_) {
        return;
      }
      ref->alreadySignalled_ = true;

      ref->cancelTimeout();
      ref->callback_->onError(std::move(ew));
      // TODO: Inspect the cases where might we end up in this function.
      // 1- server closes the stream before all the messages are
      // delievered.
      // 2- time outs
    });
    if (SingleObserver<Payload>::subscription()) {
      // TODO: can we get rid of calling the parent functions
      SingleObserver<Payload>::onError({});
    }
  }

  void timeoutExpired() noexcept override {
    onError(std::runtime_error("Request has timed out"));
  }

  void callbackCanceled() noexcept override {
    // nothing!
  }

 private:
  std::chrono::milliseconds timeout_;
  // TODO: WheelTimerInstance is part of Proxygen, we need to use another timer.
  WheelTimerInstance timer_;
  std::unique_ptr<ThriftClientCallback> callback_;
  bool alreadySignalled_{false};
};
} // namespace

RSClientThriftChannel::RSClientThriftChannel(
    std::shared_ptr<RSocketRequester> rsRequester)
    : rsRequester_(std::move(rsRequester)) {}

void RSClientThriftChannel::sendThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(metadata->__isset.kind);
  switch (metadata->kind) {
    case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
      sendSingleRequestResponse(
          std::move(metadata), std::move(payload), std::move(callback));
      break;
    case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
      sendSingleRequestNoResponse(
          std::move(metadata), std::move(payload), std::move(callback));
      break;
    case RpcKind::STREAMING_REQUEST_STREAMING_RESPONSE:
      channelRequest(std::move(metadata), std::move(payload));
      break;
    default:
      LOG(FATAL) << "not implemented";
  }
}

void RSClientThriftChannel::sendSingleRequestNoResponse(
    std::unique_ptr<RequestRpcMetadata> requestMetadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ThriftClientCallback>) noexcept {
  DCHECK(requestMetadata);

  rsRequester_
      ->fireAndForget(
          rsocket::Payload(std::move(buf), serializeMetadata(*requestMetadata)))
      ->subscribe([] {});
}

void RSClientThriftChannel::sendSingleRequestResponse(
    std::unique_ptr<RequestRpcMetadata> requestMetadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(requestMetadata);
  // As we send clientTimeoutMs, queueTimeoutMs and priority values using
  // RequestRpcMetadata object, there is no need for RSocket to put them to
  // metadata->otherMetadata map.

  auto timeout = kDefaultRequestTimeout;
  if (requestMetadata->__isset.clientTimeoutMs) {
    timeout = std::chrono::milliseconds(requestMetadata->clientTimeoutMs);
  }

  auto singleObserver = yarpl::make_ref<TimedSingleObserver>(
      std::forward<std::unique_ptr<ThriftClientCallback>>(callback), timeout);

  rsRequester_
      ->requestResponse(
          rsocket::Payload(std::move(buf), serializeMetadata(*requestMetadata)))
      ->subscribe(singleObserver);
}

void RSClientThriftChannel::channelRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> payload) noexcept {
  auto input = yarpl::flowable::Flowables::fromPublisher<rsocket::Payload>(
      [this, initialBuf = std::move(payload), metadata = std::move(metadata)](
          yarpl::Reference<yarpl::flowable::Subscriber<rsocket::Payload>>
              subscriber) mutable {
        VLOG(3) << "Input is started to be consumed: "
                << initialBuf->cloneAsValue().moveToFbString().toStdString();
        outputPromise_.setValue(yarpl::make_ref<RPCSubscriber>(
            serializeMetadata(*metadata),
            std::move(initialBuf),
            std::move(subscriber)));
      });

  // Perform the rpc call
  auto result = rsRequester_->requestChannel(input);
  result
      ->map([](auto payload) -> std::unique_ptr<folly::IOBuf> {
        VLOG(3) << "Request channel: "
                << payload.data->cloneAsValue().moveToFbString().toStdString();

        // TODO - don't drop the headers
        return std::move(payload.data);
      })
      ->subscribe(input_);
}
} // namespace thrift
} // namespace apache
