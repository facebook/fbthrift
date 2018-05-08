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

/// This operator is optimized for a special case in Thrift.
/// Neither the operator flowable nor the result flowable will support
/// multi-subscribe
class TakeFirst
    : public yarpl::flowable::
          FlowableOperator<rsocket::Payload, std::unique_ptr<folly::IOBuf>> {
  using T = rsocket::Payload;
  using U = std::unique_ptr<folly::IOBuf>;
  using Super = yarpl::flowable::FlowableOperator<T, U>;

 public:
  TakeFirst(
      std::shared_ptr<yarpl::flowable::Flowable<T>> upstream,
      folly::Function<void()> onTerminal = nullptr)
      : upstream_(std::move(upstream)), onTerminal_(std::move(onTerminal)) {}

  virtual ~TakeFirst() {
    if (auto subscription = std::exchange(subscription_, nullptr)) {
      subscription->cancel();
    }
  }

  folly::SemiFuture<std::pair<T, std::shared_ptr<yarpl::flowable::Flowable<U>>>>
  get() {
    if (auto upstream = std::exchange(upstream_, nullptr)) {
      subscription_ = std::make_shared<Subscription>(
          this->ref_from_this(this), std::move(onTerminal_));
      upstream->subscribe(subscription_);
      subscription_->request(1);
      return std::move(future_);
    }
    throw std::logic_error("Future already retrieved");
  }

  // For 'timeout' related cases
  void cancel() {
    if (auto subscription = std::exchange(subscription_, nullptr)) {
      subscription->cancel();
      setError(std::runtime_error("cancelled"));
    }
  }

 protected:
  void subscribe(
      std::shared_ptr<yarpl::flowable::Subscriber<U>> subscriber) override {
    if (auto subscription = std::exchange(subscription_, nullptr)) {
      subscription->init(std::move(subscriber));
      return;
    }
    throw std::logic_error("already subscribed");
  }

  void setError(folly::exception_wrapper&& ew) {
    upstream_.reset();
    auto promise = std::move(promise_);
    promise.setException(std::move(ew));
  }

  void setValue(T&& value) {
    upstream_.reset();
    auto promise = std::move(promise_);
    promise.setValue(
        std::make_pair(std::move(value), this->ref_from_this(this)));
  }

 private:
  using SuperSubscription = typename Super::Subscription;
  class Subscription : public SuperSubscription {
   public:
    Subscription(
        std::shared_ptr<TakeFirst> flowable,
        folly::Function<void()> onTerminal)
        : flowable_(std::move(flowable)), onTerminal_(std::move(onTerminal)) {}

    virtual ~Subscription() {
      onTerminal();
    }

    void init(
        std::shared_ptr<yarpl::flowable::Subscriber<U>> subscriber) override {
      SuperSubscription::init(std::move(subscriber));
      hasSubscriber_ = true;
      if (onSubscribe_) {
        SuperSubscription::onSubscribeImpl();
      }
      if (onComplete_) {
        SuperSubscription::onCompleteImpl();
      }
      if (error_) {
        SuperSubscription::onErrorImpl(std::move(error_));
      }
    }

    void onSubscribeImpl() override {
      if (hasSubscriber_) {
        SuperSubscription::onSubscribeImpl();
      } else {
        onSubscribe_ = true;
      }
    }

    void onCompleteImpl() {
      if (isFirstResponse_) {
        if (auto flowable = std::exchange(flowable_, nullptr)) {
          flowable->setError(std::runtime_error("no initial response"));
        }
      } else {
        onTerminal();
        if (hasSubscriber_) {
          SuperSubscription::onCompleteImpl();
        } else {
          onComplete_ = true;
        }
      }
    }

    void onErrorImpl(folly::exception_wrapper ew) {
      if (isFirstResponse_) {
        if (auto flowable = std::exchange(flowable_, nullptr)) {
          flowable->setError(std::move(ew));
        }
      } else {
        onTerminal();
        if (hasSubscriber_) {
          SuperSubscription::onErrorImpl(std::move(ew));
        } else {
          error_ = std::move(ew);
        }
      }
    }

    void onNextImpl(T value) override {
      if (std::exchange(isFirstResponse_, false)) {
        if (auto flowable = std::exchange(flowable_, nullptr)) {
          flowable->setValue(std::move(value));
        }
      } else {
        SuperSubscription::subscriberOnNext(std::move(value.data));
      }
    }

   private:
    void onTerminal() {
      if (auto onTerminal = std::move(onTerminal_)) {
        onTerminal();
      }
    }

   private:
    std::shared_ptr<TakeFirst> flowable_;
    folly::Function<void()> onTerminal_;

    bool isFirstResponse_{true};
    bool hasSubscriber_{false};
    bool onComplete_{false};
    folly::exception_wrapper error_;
    bool onSubscribe_{false};

    friend class TakeFirst;
  };

  std::shared_ptr<yarpl::flowable::Flowable<T>> upstream_;
  folly::Function<void()> onTerminal_;

  std::shared_ptr<Subscription> subscription_;
  folly::Promise<std::pair<T, std::shared_ptr<yarpl::flowable::Flowable<U>>>>
      promise_;
  folly::SemiFuture<std::pair<T, std::shared_ptr<yarpl::flowable::Flowable<U>>>>
      future_{promise_.getSemiFuture()};
};

} // namespace thrift
} // namespace apache
