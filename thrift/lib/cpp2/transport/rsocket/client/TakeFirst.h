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
#include "yarpl/flowable/Flowable.h"
#include "yarpl/flowable/Subscriber.h"

namespace apache {
namespace thrift {

/// This operator is optimized for a special case in Thrift.
/// Neither the operator flowable nor the result flowable will support
/// multi-subscribe
template <typename T>
class TakeFirst : public yarpl::flowable::FlowableOperator<T, T> {
  using Super = yarpl::flowable::FlowableOperator<T, T>;

 public:
  TakeFirst(std::shared_ptr<yarpl::flowable::Flowable<T>> upstream)
      : upstream_(std::move(upstream)) {}

  virtual ~TakeFirst() {
    if (auto subscription = std::exchange(subscription_, nullptr)) {
      subscription->cancel();
    }
  }

  folly::SemiFuture<std::pair<T, std::shared_ptr<yarpl::flowable::Flowable<T>>>>
  get() {
    if (auto upstream = std::exchange(upstream_, nullptr)) {
      subscription_ = std::make_shared<Subscription>(this->ref_from_this(this));
      upstream->subscribe(subscription_);
      subscription_->request(1);
    }
    return promise_.getSemiFuture();
  }

 protected:
  void subscribe(
      std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber) override {
    subscription_->init(std::move(subscriber));
  }

  void setError(folly::exception_wrapper&& ew) {
    promise_.setException(std::move(ew));
  }

  void setValue(T&& value) {
    promise_.setValue(
        std::make_pair(std::move(value), this->ref_from_this(this)));
  }

 private:
  using SuperSubscription = typename Super::Subscription;
  class Subscription : public SuperSubscription {
   public:
    Subscription(std::shared_ptr<TakeFirst<T>> flowable)
        : flowable_(std::move(flowable)) {}

    void init(
        std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber) override {
      if (initialized_) {
        subscriber->onSubscribe(yarpl::flowable::Subscription::create());
        subscriber->onError(std::runtime_error("already subscribed"));
        return;
      }
      initialized_ = true;
      SuperSubscription::init(std::move(subscriber));
      if (onSubscribe_) {
        SuperSubscription::onSubscribeImpl();
      }
    }

    void onSubscribeImpl() override {
      onSubscribe_ = true;
    }

    void onCompleteImpl() {
      if (isFirstResponse_) {
        if (auto flowable = std::exchange(flowable_, nullptr)) {
          flowable->setError(std::runtime_error("no initial response"));
        }
      } else {
        SuperSubscription::onCompleteImpl();
      }
    }

    void onErrorImpl(folly::exception_wrapper ew) {
      if (isFirstResponse_) {
        if (auto flowable = std::exchange(flowable_, nullptr)) {
          flowable->setError(std::move(ew));
        }
      } else {
        SuperSubscription::onErrorImpl(std::move(ew));
      }
    }

    void onNextImpl(T value) override {
      if (std::exchange(isFirstResponse_, false)) {
        if (auto flowable = std::exchange(flowable_, nullptr)) {
          flowable->setValue(std::move(value));
        }
      } else {
        SuperSubscription::subscriberOnNext(std::move(value));
      }
    }

   private:
    std::shared_ptr<TakeFirst<T>> flowable_;
    bool isFirstResponse_{true};
    bool onSubscribe_{false};
    bool initialized_{false};

    friend class TakeFirst<T>;
  };

  std::shared_ptr<yarpl::flowable::Flowable<T>> upstream_;
  std::shared_ptr<Subscription> subscription_;
  folly::Promise<std::pair<T, std::shared_ptr<yarpl::flowable::Flowable<T>>>>
      promise_;
};

} // namespace thrift
} // namespace apache
