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

#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/core/EnvelopeUtil.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/cpp2/transport/rsocket/client/TakeFirst.h>

namespace apache {
namespace thrift {

using namespace apache::thrift::transport;
using namespace apache::thrift::detail;
using namespace rsocket;
using namespace yarpl::single;
using namespace yarpl::flowable;

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

class CountedSingleObserver : public SingleObserver<Payload> {
 public:
  CountedSingleObserver(
      std::unique_ptr<ThriftClientCallback> callback,
      ChannelCounters* counters)
      : callback_(std::move(callback)), counters_(counters) {}

  virtual ~CountedSingleObserver() {
    if (counters_) {
      counters_->decPendingRequests();
    }
  }

 protected:
  void onSubscribe(std::shared_ptr<SingleSubscription>) override {
    // Note that we don't need the Subscription object.
    auto ref = this->ref_from_this(this);
    auto evb = callback_->getEventBase();
    evb->runInEventBaseThread([ref]() {
      if (ref->callback_) {
        ref->callback_->onThriftRequestSent();
      }
    });
  }

  void onSuccess(Payload payload) override {
    if (auto callback = std::move(callback_)) {
      auto evb = callback->getEventBase();
      evb->runInEventBaseThread([callback = std::move(callback),
                                 payload = std::move(payload)]() mutable {
        callback->onThriftResponse(
            payload.metadata
                ? RSClientThriftChannel::deserializeMetadata(*payload.metadata)
                : std::make_unique<ResponseRpcMetadata>(),
            std::move(payload.data));
      });
    }
  }

  void onError(folly::exception_wrapper ew) override {
    if (auto callback = std::move(callback_)) {
      auto evb = callback->getEventBase();
      evb->runInEventBaseThread(
          [callback = std::move(callback), ew = std::move(ew)]() mutable {
            callback->onError(std::move(ew));
            // TODO: Inspect the cases where might we end up in this function.
            // 1- server closes the stream before all the messages are
            // delievered.
            // 2- time outs
          });
    }
  }

 private:
  std::unique_ptr<ThriftClientCallback> callback_;
  apache::thrift::detail::ChannelCounters* counters_;
};

/// Take the first item from the stream as the response of the RPC call
class CountedStream
    : public FlowableOperator<Payload, std::unique_ptr<folly::IOBuf>> {
 public:
  CountedStream(
      std::unique_ptr<ThriftClientCallback> callback,
      ChannelCounters* counters,
      folly::EventBase* ioEvb)
      : callback_(std::move(callback)), counters_(counters), ioEvb_(ioEvb) {}

  virtual ~CountedStream() {
    releaseCounter();
  }

  ThriftClientCallback& callback() {
    return *callback_;
  }

  // Returns the initial result payload
  void setResult(std::shared_ptr<Flowable<Payload>> upstream) {
    auto takeFirst = std::make_shared<TakeFirst<Payload>>(upstream);
    takeFirst->get()
        .via(ioEvb_)
        .then(
            [ref = this->ref_from_this(this)](
                std::pair<Payload, std::shared_ptr<Flowable<Payload>>> result) {
              auto evb = ref->callback_->getEventBase();
              auto lambda = [ref, result = std::move(result)]() mutable {
                ref->setUpstream(std::move(result.second));
                auto callback = std::move(ref->callback_);
                callback->onThriftResponse(
                    result.first.metadata
                        ? RSClientThriftChannel::deserializeMetadata(
                              *result.first.metadata)
                        : std::make_unique<ResponseRpcMetadata>(),
                    std::move(result.first.data),
                    toStream(
                        std::static_pointer_cast<
                            Flowable<std::unique_ptr<folly::IOBuf>>>(ref),
                        ref->ioEvb_));
              };
              evb->runInEventBaseThread(std::move(lambda));
            })
        .onError(
            [ref = this->ref_from_this(this)](folly::exception_wrapper ew) {
              auto evb = ref->callback().getEventBase();
              evb->runInEventBaseThread([callback = std::move(ref->callback_),
                                         ew = std::move(ew)]() mutable {
                callback->onError(std::move(ew));
              });
            });
  }

  void subscribe(std::shared_ptr<Subscriber<std::unique_ptr<folly::IOBuf>>>
                     subscriber) override {
    CHECK(upstream_) << "Verify that setUpstream method is called.";
    upstream_->subscribe(std::make_shared<Subscription>(
        this->ref_from_this(this), std::move(subscriber)));
  }

 protected:
  void setUpstream(std::shared_ptr<Flowable<Payload>> stream) {
    upstream_ = std::move(stream);
  }

  void releaseCounter() {
    if (auto counters = std::exchange(counters_, nullptr)) {
      counters->decPendingRequests();
    }
  }

 private:
  using SuperSubscription =
      FlowableOperator<Payload, std::unique_ptr<folly::IOBuf>>::Subscription;
  class Subscription : public SuperSubscription {
   public:
    Subscription(
        std::shared_ptr<CountedStream> flowable,
        std::shared_ptr<Subscriber<std::unique_ptr<folly::IOBuf>>> subscriber)
        : SuperSubscription(std::move(subscriber)),
          flowable_(std::move(flowable)) {}

    void onNextImpl(Payload value) override {
      try {
        // TODO - don't drop the headers
        this->subscriberOnNext(std::move(value.data));
      } catch (const std::exception& exn) {
        folly::exception_wrapper ew{std::current_exception(), exn};
        this->terminateErr(std::move(ew));
      }
    }

    void onTerminateImpl() override {
      if (auto flowable = std::exchange(flowable_, nullptr)) {
        flowable->releaseCounter();
      }
    }

   private:
    std::shared_ptr<CountedStream> flowable_;
  };

  std::unique_ptr<ThriftClientCallback> callback_;
  std::shared_ptr<Flowable<Payload>> upstream_;
  std::shared_ptr<Subscription> subscription_;

  apache::thrift::detail::ChannelCounters* counters_;
  folly::EventBase* ioEvb_;
};

} // namespace

RSClientThriftChannel::RSClientThriftChannel(
    std::shared_ptr<RSRequester> rsRequester,
    ChannelCounters& channelCounters,
    folly::EventBase* evb)
    : rsRequester_(std::move(rsRequester)),
      channelCounters_(channelCounters),
      evb_(evb) {}

void RSClientThriftChannel::sendThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  evb_->dcheckIsInEventBaseThread();

  if (!EnvelopeUtil::stripEnvelope(metadata.get(), payload)) {
    LOG(ERROR) << "Unexpected problem stripping envelope";
    auto evb = callback->getEventBase();
    evb->runInEventBaseThread([cb = std::move(callback)]() mutable {
      cb->onError(folly::exception_wrapper(
          TTransportException("Unexpected problem stripping envelope")));
    });
    return;
  }
  metadata->seqId = 0;
  metadata->__isset.seqId = true;
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
    case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
      sendSingleRequestStreamResponse(
          std::move(metadata), std::move(payload), std::move(callback));
      break;
    case RpcKind::STREAMING_REQUEST_STREAMING_RESPONSE:
      sendStreamRequestStreamResponse(
          std::move(metadata), std::move(payload), std::move(callback));
      break;
    default: {
      LOG(ERROR) << "Unknown RpcKind value in the Metadata";
      auto evb = callback->getEventBase();
      evb->runInEventBaseThread([cb = std::move(callback)]() mutable {
        cb->onError(folly::exception_wrapper(
            TTransportException("Unknown RpcKind value in the Metadata")));
      });
    }
  }
}

void RSClientThriftChannel::sendSingleRequestNoResponse(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(metadata);

  if (channelCounters_.incPendingRequests()) {
    auto guard =
        folly::makeGuard([&] { channelCounters_.decPendingRequests(); });
    rsRequester_
        ->fireAndForget(Payload(std::move(buf), serializeMetadata(*metadata)))
        ->subscribe([] {});
  } else {
    LOG_EVERY_N(ERROR, 100)
        << "max number of pending requests is exceeded x100";
  }

  auto cbEvb = callback->getEventBase();
  cbEvb->runInEventBaseThread(
      [cb = std::move(callback)]() mutable { cb->onThriftRequestSent(); });
}

void RSClientThriftChannel::sendSingleRequestResponse(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(metadata);
  bool canExecute = channelCounters_.incPendingRequests();

  auto singleObserver = std::make_shared<CountedSingleObserver>(
      std::move(callback), canExecute ? &channelCounters_ : nullptr);

  if (!canExecute) {
    TTransportException ex(
        TTransportException::NETWORK_ERROR,
        "Too many active requests on connection");
    // Might be able to create another transaction soon
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);
    yarpl::single::Singles::error<Payload>(
        folly::make_exception_wrapper<TTransportException>(std::move(ex)))
        ->subscribe(std::move(singleObserver));
    return;
  }

  // As we send clientTimeoutMs, queueTimeoutMs and priority values using
  // RequestRpcMetadata object, there is no need for RSocket to put them to
  // metadata->otherMetadata map.

  rsRequester_
      ->requestResponse(Payload(std::move(buf), serializeMetadata(*metadata)))
      ->subscribe(std::move(singleObserver));
}

void RSClientThriftChannel::sendStreamRequestStreamResponse(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  bool canExecute = channelCounters_.incPendingRequests();

  if (!canExecute) {
    auto evb = callback->getEventBase();
    evb->runInEventBaseThread([callback = std::move(callback)]() mutable {
      TTransportException ex(
          TTransportException::NETWORK_ERROR,
          "Too many active requests on connection");
      // Might be able to create another transaction soon
      ex.setOptions(TTransportException::CHANNEL_IS_VALID);

      callback->onError(folly::exception_wrapper(std::move(ex)));
    });
    return;
  }

  auto countedStream = std::make_shared<CountedStream>(
      std::move(callback), &channelCounters_, evb_);

  auto input = internal::flowableFromSubscriber<Payload>(
      [countedStream](std::shared_ptr<Subscriber<Payload>> subscriber) mutable {
        auto& scb = countedStream->callback();
        auto callback = scb.inner();
        auto inputStream = callback->extractStream();
        DCHECK(inputStream) << "callback's inputStream_ is missing";
        auto mappedStream = std::move(inputStream).map([](auto&& buf) mutable {
          return Payload(std::move(buf));
        });
        toFlowable(std::move(mappedStream))->subscribe(std::move(subscriber));

        // no need any more
        countedStream.reset();
      });

  // Perform the rpc call
  auto result = rsRequester_->requestChannel(
      Payload(std::move(buf), serializeMetadata(*metadata)), input);

  // Whenever first response arrives, provide the first response and
  // itself, as stream, to the callback
  countedStream->setResult(result);
}

void RSClientThriftChannel::sendSingleRequestStreamResponse(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  bool canExecute = channelCounters_.incPendingRequests();
  if (!canExecute) {
    auto evb = callback->getEventBase();
    evb->runInEventBaseThread([callback = std::move(callback)]() mutable {
      TTransportException ex(
          TTransportException::NETWORK_ERROR,
          "Too many active requests on connection");
      // Might be able to create another transaction soon
      ex.setOptions(TTransportException::CHANNEL_IS_VALID);

      callback->onError(folly::exception_wrapper(std::move(ex)));
    });
    return;
  }

  auto countedStream = std::make_shared<CountedStream>(
      std::move(callback), &channelCounters_, evb_);

  auto result = rsRequester_->requestStream(
      Payload(std::move(buf), serializeMetadata(*metadata)));

  // Whenever first response arrives, provide the first response and itself, as
  // stream, to the callback
  countedStream->setResult(result);
}

bool RSClientThriftChannel::isDetachable() {
  return true;
}

bool RSClientThriftChannel::attachEventBase(folly::EventBase* evb) {
  evb_ = evb;
  return true;
}

void RSClientThriftChannel::detachEventBase() {
  evb_ = nullptr;
}

// ChannelCounters' functions
static constexpr uint32_t kMaxPendingRequests =
    std::numeric_limits<uint32_t>::max();

ChannelCounters::ChannelCounters() : maxPendingRequests_(kMaxPendingRequests) {}

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

} // namespace thrift
} // namespace apache
