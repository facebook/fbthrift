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

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include <folly/ExceptionWrapper.h>

#include <yarpl/flowable/Flowable.h>
#include <yarpl/flowable/Subscription.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

namespace apache {
namespace thrift {
namespace rocket {

class ErrorFrame;
class PayloadFrame;
class RocketClient;

class RocketClientFlowable final : public yarpl::flowable::Flowable<Payload> {
 private:
  class RocketClientSubscription final : public yarpl::flowable::Subscription {
   public:
    RocketClientSubscription(StreamId streamId, RocketClient& client)
        : client_(&client), streamId_(streamId) {}

    void request(int64_t n) override;
    void cancel() override;

   private:
    RocketClient* client_;
    StreamId streamId_;
  };

 public:
  RocketClientFlowable(StreamId streamId, RocketClient& client)
      : client_(client), streamId_(streamId) {}

  // Flowable implementation
  void subscribe(std::shared_ptr<yarpl::flowable::Subscriber<Payload>>
                     subscriber) override {
    if (std::exchange(subscribed_, true)) {
      throw std::logic_error("Already subscribed to RocketClientFlowable");
    }

    subscriber_ = std::move(subscriber);
    subscriber_->onSubscribe(
        std::make_shared<RocketClientSubscription>(streamId_, client_));

    if (completed_) {
      onComplete();
    }
    if (ew_) {
      onError(std::move(ew_));
    }
  }

  // RocketClient callbacks
  void onNext(Payload next) {
    if (subscriber_) {
      subscriber_->onNext(std::move(next));
    }
  }

  void onError(folly::exception_wrapper ew) {
    if (auto subscriber = std::exchange(subscriber_, nullptr)) {
      subscriber->onError(std::move(ew));
    } else {
      ew_ = std::move(ew);
    }
  }

  void onComplete() {
    if (auto subscriber = std::exchange(subscriber_, nullptr)) {
      subscriber->onComplete();
    }
    completed_ = true;
  }

  void onPayloadFrame(PayloadFrame&& frame);
  void onErrorFrame(ErrorFrame&& frame);

 private:
  folly::exception_wrapper ew_;
  std::shared_ptr<yarpl::flowable::Subscriber<Payload>> subscriber_;
  std::unique_ptr<Payload> bufferedFragments_;
  RocketClient& client_;
  StreamId streamId_;
  bool subscribed_{false};
  bool completed_{false};
};

} // namespace rocket
} // namespace thrift
} // namespace apache
