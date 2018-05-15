/*
 * Copyright 2004-present Facebook, Inc.
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

#include <folly/futures/Future.h>
#include <rsocket/Payload.h>
#include <yarpl/flowable/Flowable.h>
#include <yarpl/flowable/Subscriber.h>

namespace apache {
namespace thrift {

class TakeFirst
    : public yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>,
      public yarpl::flowable::Subscriber<rsocket::Payload> {
  using T = rsocket::Payload;
  using U = std::unique_ptr<folly::IOBuf>;

 public:
  TakeFirst(
      folly::Function<void()> onRequestSent,
      folly::Function<void(std::pair<T, std::shared_ptr<Flowable<U>>>)>
          onResponse,
      folly::Function<void(folly::exception_wrapper)> onError,
      folly::Function<void()> onTerminal)
      : onRequestSent_(std::move(onRequestSent)),
        onResponse_(std::move(onResponse)),
        onError_(std::move(onError)),
        onTerminal_(std::move(onTerminal)) {}

  virtual ~TakeFirst() {
    if (auto subscription = std::exchange(subscription_, nullptr)) {
      subscription->cancel();
    }
  }

  // For 'timeout' related cases
  void cancel() {
    if (auto subscription = std::exchange(subscription_, nullptr)) {
      subscription->cancel();
      onResponse_ = nullptr;
      onTerminal_ = nullptr;
      if (auto onErr = std::exchange(onError_, nullptr)) {
        onErr(std::runtime_error("cancelled"));
      }
    }
  }

 protected:
  void onSubscribe(
      std::shared_ptr<yarpl::flowable::Subscription> subscription) override {
    subscription_ = std::move(subscription);
    if (auto onReq = std::exchange(onRequestSent_, nullptr)) {
      onReq();
    }
    subscription_->request(1);
  }

  void subscribe(
      std::shared_ptr<yarpl::flowable::Subscriber<U>> subscriber) override {
    if (auto subscription = std::exchange(subscription_, nullptr)) {
      subscriber_ = std::move(subscriber);
      subscriber_->onSubscribe(std::move(subscription));
      if (completed_) {
        onComplete();
      }
      if (error_) {
        onError(std::move(error_));
      }
      return;
    }
    throw std::logic_error("already subscribed");
  }

  void onComplete() {
    if (isFirstResponse_) {
      onError(std::runtime_error("no initial response"));
      return;
    }
    if (auto subscriber = std::exchange(subscriber_, nullptr)) {
      subscriber->onComplete();
    } else {
      completed_ = true;
    }
    onTerminal();
  }

  void onError(folly::exception_wrapper ew) {
    if (isFirstResponse_) {
      onResponse_ = nullptr;
      onTerminal_ = nullptr;
      if (auto onErr = std::exchange(onError_, nullptr)) {
        onErr(std::move(ew));
      }
      return;
    }

    if (auto subscriber = std::exchange(subscriber_, nullptr)) {
      subscriber->onError(std::move(ew));
    } else {
      error_ = std::move(ew);
    }
    onTerminal();
  }

  // Used for breaking the cycle between Subscription and Subscriber
  // when the response Flowable is not subscribed at all.
  class SafeFlowable : public Flowable<U> {
   public:
    SafeFlowable(std::shared_ptr<TakeFirst> inner) : inner_(std::move(inner)) {}

    ~SafeFlowable() {
      if (auto inner = std::exchange(inner_, nullptr)) {
        inner->cancel();
      }
    }

    void subscribe(std::shared_ptr<yarpl::flowable::Subscriber<U>> subscriber) {
      if (auto inner = std::exchange(inner_, nullptr)) {
        inner->subscribe(std::move(subscriber));
        return;
      }
      throw std::logic_error("already subscribed");
    }

    std::shared_ptr<TakeFirst> inner_;
  };

  void onNext(T value) override {
    if (std::exchange(isFirstResponse_, false)) {
      onError_ = nullptr;
      if (auto onResponse = std::exchange(onResponse_, nullptr)) {
        onResponse(std::make_pair(
            std::move(value),
            std::make_shared<SafeFlowable>(this->ref_from_this(this))));
      }
    } else {
      DCHECK(subscriber_);
      subscriber_->onNext(std::move(value.data));
    }
  }

  void onTerminal() {
    if (auto onTerminal = std::move(onTerminal_)) {
      onTerminal();
    }
  }

  folly::Function<void()> onRequestSent_;
  folly::Function<void(std::pair<T, std::shared_ptr<Flowable<U>>>)> onResponse_;
  folly::Function<void(folly::exception_wrapper)> onError_;
  folly::Function<void()> onTerminal_;

  bool isFirstResponse_{true};
  bool completed_{false};
  folly::exception_wrapper error_;

  std::shared_ptr<yarpl::flowable::Subscriber<U>> subscriber_;
  std::shared_ptr<yarpl::flowable::Subscription> subscription_;
};

} // namespace thrift
} // namespace apache
