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
#include <thrift/lib/cpp2/transport/rsocket/client/RPCSubscriber.h>

namespace apache {
namespace thrift {

using namespace apache::thrift::transport;
using namespace apache::thrift::detail;
using namespace rsocket;
using namespace yarpl::single;

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

void RSClientThriftChannel::sendStreamThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> payload,
    std::unique_ptr<StreamRequestCallback> callback) noexcept {
  evb_->dcheckIsInEventBaseThread();

  if (!EnvelopeUtil::stripEnvelope(metadata.get(), payload)) {
    LOG(ERROR) << "Unexpected problem stripping envelope";
    callback->getOutput()->onSubscribe(yarpl::flowable::Subscription::empty());
    callback->getOutput()->onError(folly::exception_wrapper(
        TTransportException("Unexpected problem stripping envelope")));
    return;
  }
  metadata->seqId = 0;
  metadata->__isset.seqId = true;
  DCHECK(metadata->__isset.kind);
  switch (metadata->kind) {
    case RpcKind::STREAMING_REQUEST_STREAMING_RESPONSE:
      sendStreamRequestStreamResponse(
          std::move(metadata), std::move(payload), std::move(callback));
      break;
    case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
      sendSingleRequestStreamResponse(
          std::move(metadata), std::move(payload), std::move(callback));
      break;
    default: {
      LOG(ERROR) << "Unknown RpcKind value in the Metadata";
      callback->getOutput()->onSubscribe(
          yarpl::flowable::Subscription::empty());
      callback->getOutput()->onError(folly::exception_wrapper(
          TTransportException("Unknown RpcKind value in the Metadata")));
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
        ->fireAndForget(
            rsocket::Payload(std::move(buf), serializeMetadata(*metadata)))
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

  auto singleObserver = yarpl::make_ref<CountedSingleObserver>(
      std::move(callback), canExecute ? &channelCounters_ : nullptr);

  if (canExecute) {
    // As we send clientTimeoutMs, queueTimeoutMs and priority values using
    // RequestRpcMetadata object, there is no need for RSocket to put them to
    // metadata->otherMetadata map.

    rsRequester_
        ->requestResponse(
            rsocket::Payload(std::move(buf), serializeMetadata(*metadata)))
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

void RSClientThriftChannel::sendStreamRequestStreamResponse(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<StreamRequestCallback> callback) noexcept {
  auto output = callback->getOutput();

  auto input =
      yarpl::flowable::internal::flowableFromSubscriber<rsocket::Payload>(
          [initialBuf = std::move(buf),
           metadata = std::move(metadata),
           callback = std::move(callback)](
              std::shared_ptr<yarpl::flowable::Subscriber<rsocket::Payload>>
                  subscriber) mutable {
            VLOG(3)
                << "Input is started to be consumed: "
                << initialBuf->cloneAsValue().moveToFbString().toStdString();
            StreamRequestCallback* scb =
                static_cast<StreamRequestCallback*>(callback.get());
            auto rpc_subscriber = yarpl::make_ref<RPCSubscriber>(
                serializeMetadata(*metadata),
                std::move(initialBuf),
                std::move(subscriber));
            rpc_subscriber->init();
            scb->subscribeToInput(std::move(rpc_subscriber));
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
      ->subscribe(output);
}

void RSClientThriftChannel::sendSingleRequestStreamResponse(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<StreamRequestCallback> callback) noexcept {
  auto result = rsRequester_->requestStream(
      rsocket::Payload(std::move(buf), serializeMetadata(*metadata)));
  result
      ->map([](auto payload) -> std::unique_ptr<folly::IOBuf> {
        // TODO - don't drop the headers
        return std::move(payload.data);
      })
      ->subscribe(callback->getOutput());
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
