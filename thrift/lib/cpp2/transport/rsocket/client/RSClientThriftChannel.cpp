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

using namespace apache::thrift::transport;
using namespace apache::thrift::detail;
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
    std::chrono::milliseconds(5000);

// Adds a timer that timesout if the observer could not get its onSuccess or
// onError methods being called in a specified time range, which causes onError
// method to be called.
class TimedSingleObserver : public SingleObserverBase<Payload>,
                            public folly::HHWheelTimer::Callback {
 public:
  TimedSingleObserver(
      std::unique_ptr<ThriftClientCallback> callback,
      std::chrono::milliseconds timeout,
      ChannelCounters& counters)
      : timeout_(timeout),
        timer_(timeout, callback->getEventBase()),
        callback_(std::move(callback)),
        counters_(counters) {}

  virtual ~TimedSingleObserver() {
    complete();
  }

  void setDecrementPendingRequestCounter() {
    decrementPendingRequestCounter_ = true;
  }

 protected:
  void onSubscribe(Reference<SingleSubscription> subscription) override {
    auto ref = this->ref_from_this(this);
    SingleObserverBase<Payload>::onSubscribe(std::move(subscription));

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
      if (ref->complete()) {
        ref->callback_->onThriftResponse(
            payload.metadata
                ? RSClientThriftChannel::deserializeMetadata(*payload.metadata)
                : std::make_unique<ResponseRpcMetadata>(),
            std::move(payload.data));
      }
    });
    if (SingleObserverBase<Payload>::subscription()) {
      // TODO: can we get rid of calling the parent functions
      SingleObserverBase<Payload>::onSuccess({});
    }
  }

  void onError(folly::exception_wrapper ew) override {
    auto ref = this->ref_from_this(this);
    auto evb = callback_->getEventBase();
    evb->runInEventBaseThread([ref, ew = std::move(ew)]() mutable {
      if (ref->complete()) {
        ref->callback_->onError(std::move(ew));
        // TODO: Inspect the cases where might we end up in this function.
        // 1- server closes the stream before all the messages are
        // delievered.
        // 2- time outs
      }
    });
    if (SingleObserverBase<Payload>::subscription()) {
      // TODO: can we get rid of calling the parent functions
      SingleObserverBase<Payload>::onError({});
    }
  }

  void timeoutExpired() noexcept override {
    onError(folly::make_exception_wrapper<TTransportException>(
        apache::thrift::transport::TTransportException::TIMED_OUT));
  }

  void callbackCanceled() noexcept override {
    // nothing!
  }

  bool complete() {
    if (alreadySignalled_) {
      return false;
    }
    alreadySignalled_ = true;

    cancelTimeout();
    if (decrementPendingRequestCounter_) {
      counters_.decPendingRequests();
      decrementPendingRequestCounter_ = false;
    }
    return true;
  }

 private:
  std::chrono::milliseconds timeout_;
  // TODO: WheelTimerInstance is part of Proxygen, we need to use another timer.
  WheelTimerInstance timer_;
  std::unique_ptr<ThriftClientCallback> callback_;
  apache::thrift::detail::ChannelCounters& counters_;

  bool alreadySignalled_{false};
  bool decrementPendingRequestCounter_{false};
};
} // namespace

RSClientThriftChannel::RSClientThriftChannel(
    std::shared_ptr<RSocketRequester> rsRequester,
    ChannelCounters& channelCounters)
    : rsRequester_(std::move(rsRequester)), channelCounters_(channelCounters) {}

void RSClientThriftChannel::sendThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(metadata->__isset.kind);

  if (!rsRequester_) {
    if (callback) {
      auto evb = callback->getEventBase();
      evb->runInEventBaseThread([evbCallback = std::move(callback)]() mutable {
        evbCallback->onError(folly::make_exception_wrapper<TTransportException>(
            TTransportException::NOT_OPEN));
      });
    }
    return;
  }

  metadata->seqId = 0;
  metadata->__isset.seqId = true;
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

  if (channelCounters_.incPendingRequests()) {
    auto guard =
        folly::makeGuard([&] { channelCounters_.decPendingRequests(); });
    rsRequester_
        ->fireAndForget(rsocket::Payload(
            std::move(buf), serializeMetadata(*requestMetadata)))
        ->subscribe([] {});
  } else {
    LOG(ERROR) << "max number of pending requests is exceeded";
  }
}

void RSClientThriftChannel::sendSingleRequestResponse(
    std::unique_ptr<RequestRpcMetadata> requestMetadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(requestMetadata);
  auto timeout = kDefaultRequestTimeout;
  if (requestMetadata->__isset.clientTimeoutMs) {
    timeout = std::chrono::milliseconds(requestMetadata->clientTimeoutMs);
  }

  auto singleObserver = yarpl::make_ref<TimedSingleObserver>(
      std::forward<std::unique_ptr<ThriftClientCallback>>(callback),
      timeout,
      channelCounters_);

  if (channelCounters_.incPendingRequests()) {
    singleObserver->setDecrementPendingRequestCounter();

    // As we send clientTimeoutMs, queueTimeoutMs and priority values using
    // RequestRpcMetadata object, there is no need for RSocket to put them to
    // metadata->otherMetadata map.

    rsRequester_
        ->requestResponse(rsocket::Payload(
            std::move(buf), serializeMetadata(*requestMetadata)))
        ->subscribe(std::move(singleObserver));
  } else {
    TTransportException ex(
        TTransportException::NETWORK_ERROR,
        "Too many active requests on connection");
    // Might be able to create another transaction soon
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);
    yarpl::single::Singles::error<Payload>(
        folly::make_exception_wrapper<TTransportException>(std::move(ex)))
        ->subscribe(std::move(singleObserver));
  }
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

// ChannelCounters' functions
static constexpr uint32_t kMaxPendingRequests =
    std::numeric_limits<uint32_t>::max();

ChannelCounters::ChannelCounters()
    : maxPendingRequests_(kMaxPendingRequests),
      requestTimeout_(kDefaultRequestTimeout) {}

void ChannelCounters::setMaxPendingRequests(uint32_t count) {
  maxPendingRequests_ = count;
}

uint32_t ChannelCounters::getMaxPendingRequests() {
  return maxPendingRequests_;
}

uint32_t ChannelCounters::getPendingRequests() {
  return pendingRequests_;
}

bool ChannelCounters::incPendingRequests() {
  if (pendingRequests_ >= maxPendingRequests_) {
    return false;
  }
  ++pendingRequests_;
  return true;
}

void ChannelCounters::decPendingRequests() {
  --pendingRequests_;
}

void ChannelCounters::setRequestTimeout(std::chrono::milliseconds timeout) {
  requestTimeout_ = timeout;
}

std::chrono::milliseconds ChannelCounters::getRequestTimeout() {
  return requestTimeout_;
}

} // namespace thrift
} // namespace apache
