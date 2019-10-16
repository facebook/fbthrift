/*
 * Copyright 2019-present Facebook, Inc.
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

#pragma once

#include <algorithm>
#include <limits>

#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

#include <yarpl/flowable/Flowable.h>
#include <yarpl/flowable/Subscription.h>

namespace apache {
namespace thrift {

class ClientCallbackFlowable final
    : public yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>,
      public StreamClientCallback {
 private:
  class RocketClientSubscription final : public yarpl::flowable::Subscription {
   public:
    explicit RocketClientSubscription(
        std::shared_ptr<ClientCallbackFlowable> flowable)
        : flowable_(std::move(flowable)) {}

    void request(int64_t n) override {
      if (!flowable_) {
        return;
      }
      n = std::min<int64_t>(
          std::max<int64_t>(0, n), std::numeric_limits<int32_t>::max());
      flowable_->serverCallback_->onStreamRequestN(n);
    }

    void cancel() override {
      flowable_->serverCallback_->onStreamCancel();
      flowable_.reset();
    }

   private:
    std::shared_ptr<ClientCallbackFlowable> flowable_;
  };

 public:
  ClientCallbackFlowable(
      std::unique_ptr<ThriftClientCallback> clientCallback,
      std::chrono::milliseconds)
      : clientCallback_(std::move(clientCallback)) {}

  void init() {
    self_ = this->ref_from_this(this);
  }
  // yarpl::flowable::Flowable interface
  void subscribe(std::shared_ptr<yarpl::flowable::Subscriber<
                     std::unique_ptr<folly::IOBuf>>> subscriber) override {
    subscriber_ = std::move(subscriber);
    auto subscription =
        std::make_shared<RocketClientSubscription>(this->ref_from_this(this));
    subscriber_->onSubscribe(std::move(subscription));
    if (pendingComplete_) {
      onStreamComplete();
    }
    if (pendingError_) {
      onStreamError(std::move(pendingError_));
    }
  }

  // ClientCallback interface
  void onFirstResponse(
      FirstResponsePayload&& firstPayload,
      folly::EventBase* evb,
      StreamServerCallback* serverCallback) override {
    serverCallback_ = serverCallback;
    auto cb = std::move(clientCallback_);
    cb->onThriftResponse(
        std::move(firstPayload.metadata),
        std::move(firstPayload.payload),
        toStream(std::move(self_), evb));
  }

  void onFirstResponseError(folly::exception_wrapper ew) override {
    auto cb = std::move(clientCallback_);
    cb->onError(std::move(ew));
    self_.reset();
  }

  void onStreamNext(StreamPayload&& payload) override {
    subscriber_->onNext(std::move(payload.payload));
  }

  void onStreamError(folly::exception_wrapper ew) override {
    if (!subscriber_) {
      pendingError_ = std::move(ew);
      return;
    }

    folly::exception_wrapper hijacked;
    if (ew.with_exception([&hijacked](rocket::RocketException& rex) {
          hijacked = folly::exception_wrapper(
              apache::thrift::detail::EncodedError(rex.moveErrorData()));
        })) {
      subscriber_->onError(std::move(hijacked));
    } else {
      subscriber_->onError(std::move(ew));
    }
  }

  void onStreamComplete() override {
    if (subscriber_) {
      subscriber_->onComplete();
    } else {
      pendingComplete_ = true;
    }
  }

 private:
  std::unique_ptr<ThriftClientCallback> clientCallback_;
  std::shared_ptr<yarpl::flowable::Subscriber<std::unique_ptr<folly::IOBuf>>>
      subscriber_;
  StreamServerCallback* serverCallback_{nullptr};
  std::shared_ptr<yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>>
      self_;
  bool pendingComplete_{false};
  folly::exception_wrapper pendingError_;
};

} // namespace thrift
} // namespace apache
