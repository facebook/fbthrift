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

#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

#include <yarpl/Observable.h>

namespace apache {
namespace thrift {
namespace detail {
template <typename T>
class StreamPublisherState : public yarpl::observable::Observable<T>,
                             public yarpl::observable::Subscription {
 public:
  explicit StreamPublisherState(folly::Function<void()> onCanceled)
      : state_(folly::in_place, std::move(onCanceled)) {}

  std::shared_ptr<yarpl::observable::Subscription> subscribe(
      std::shared_ptr<yarpl::observable::Observer<T>> observer) override {
    observer_ = observer;
    state_.wlock()->observer_ = observer;

    auto self = this->ref_from_this(this);
    observer->onSubscribe(self);
    return self;
  }

  void cancel() override {
    canceled_ = true;
    completeImpl();
  }

  void next(T&& value) {
    if (auto observer = observer_.lock()) {
      observer->onNext(std::move(value));
    }
  }

  void next(const T& value) {
    if (auto observer = observer_.lock()) {
      observer->onNext(value);
    }
  }

  void complete(folly::exception_wrapper e) {
    if (auto observer = observer_.lock()) {
      observer->onError(std::move(e));
    }
    completeImpl();
  }

  void complete() {
    if (auto observer = observer_.lock()) {
      observer->onComplete();
    }
    completeImpl();
  }

  void release() {
    if (!canceled_) {
      LOG(FATAL) << "StreamPublisher has to be completed or canceled.";
    }
  }

 private:
  void completeImpl() {
    auto wState = state_.wlock();
    wState->observer_ = nullptr;
    if (auto onCompleteOrCanceled =
            std::exchange(wState->onCompleteOrCanceled_, nullptr)) {
      onCompleteOrCanceled();
    }
  }

  struct State {
    explicit State(folly::Function<void()> onCompleteOrCanceled)
        : onCompleteOrCanceled_(std::move(onCompleteOrCanceled)) {}

    folly::Function<void()> onCompleteOrCanceled_;
    std::shared_ptr<yarpl::observable::Observer<T>> observer_;
  };
  folly::Synchronized<State> state_;
  std::weak_ptr<yarpl::observable::Observer<T>> observer_;
  std::atomic<bool> canceled_{false};
};

template <typename T>
class EagerSubscribeOperator : public yarpl::flowable::FlowableOperator<T, T> {
  using Super = yarpl::flowable::FlowableOperator<T, T>;

 public:
  EagerSubscribeOperator(std::shared_ptr<yarpl::flowable::Flowable<T>> upstream)
      : subscription_(std::make_shared<Subscription>()) {
    upstream->subscribe(subscription_);
  }

  void subscribe(
      std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber) override {
    auto subscription = std::move(subscription_);
    subscription->setSubscriber(std::move(subscriber));
  }

  ~EagerSubscribeOperator() {
    if (auto subscription = std::move(subscription_)) {
      class CancellingSubscriber : public yarpl::flowable::Subscriber<T> {
       public:
        void onSubscribe(std::shared_ptr<yarpl::flowable::Subscription>
                             subscription) override {
          subscription->cancel();
        }
        void onNext(T) override {}
        void onComplete() override {}
        void onError(folly::exception_wrapper) override {}
      };
      subscription->setSubscriber(std::make_shared<CancellingSubscriber>());
    }
  }

 private:
  using SuperSubscription = typename Super::Subscription;
  class Subscription : public SuperSubscription {
   public:
    void onNextImpl(T value) override {
      DCHECK(state_ == State::HAS_SUBSCRIBER);
      SuperSubscription::subscriberOnNext(std::move(value));
    }

    void onCompleteImpl() override {
      switch (state_) {
        case State::SUBSCRIBED:
          state_ = State::COMPLETED;
          return;
        case State::HAS_SUBSCRIBER:
          SuperSubscription::onCompleteImpl();
          return;
        case State::EMPTY:
        case State::COMPLETED:
        case State::HAS_ERROR:
          LOG(FATAL) << "Invalid state";
      }
    }

    void onErrorImpl(folly::exception_wrapper error) override {
      error_ = std::move(error);
      switch (state_) {
        case State::SUBSCRIBED:
          state_ = State::HAS_ERROR;
          return;
        case State::HAS_SUBSCRIBER:
          SuperSubscription::onErrorImpl(std::move(error_));
          return;
        case State::EMPTY:
        case State::COMPLETED:
        case State::HAS_ERROR:
          LOG(FATAL) << "Invalid state";
      }
    }

    void onSubscribeImpl() override {
      switch (state_) {
        case State::EMPTY:
          state_ = State::SUBSCRIBED;
          return;
        case State::HAS_SUBSCRIBER:
          SuperSubscription::onSubscribeImpl();
          return;
        case State::SUBSCRIBED:
        case State::COMPLETED:
        case State::HAS_ERROR:
          LOG(FATAL) << "Invalid state";
      }
    }

    void setSubscriber(
        std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber) {
      this->init(std::move(subscriber));
      switch (state_) {
        case State::EMPTY:
          break;
        case State::SUBSCRIBED:
          SuperSubscription::onSubscribeImpl();
          break;
        case State::COMPLETED:
          SuperSubscription::onSubscribeImpl();
          SuperSubscription::onCompleteImpl();
          break;
        case State::HAS_ERROR:
          SuperSubscription::onSubscribeImpl();
          SuperSubscription::onErrorImpl(std::move(error_));
          break;
        case State::HAS_SUBSCRIBER:
          LOG(FATAL) << "Invalid state";
      }
      state_ = State::HAS_SUBSCRIBER;
    }

   private:
    enum class State {
      EMPTY,
      SUBSCRIBED,
      HAS_SUBSCRIBER,
      HAS_ERROR,
      COMPLETED
    };

    State state_{State::EMPTY};
    folly::exception_wrapper error_;
  };

  std::shared_ptr<Subscription> subscription_;
};
} // namespace detail

template <typename T>
StreamPublisher<T>::~StreamPublisher() {
  if (sharedState_) {
    sharedState_->release();
  }
}

template <typename T>
std::pair<Stream<T>, StreamPublisher<T>> StreamPublisher<T>::create(
    folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
    folly::Function<void()> onCanceled) {
  auto state =
      std::make_shared<detail::StreamPublisherState<T>>(std::move(onCanceled));
  auto flowable = state->toFlowable(yarpl::BackpressureStrategy::BUFFER);
  flowable =
      std::make_shared<detail::EagerSubscribeOperator<T>>(std::move(flowable));
  return {toStream(std::move(flowable), std::move(executor)),
          StreamPublisher<T>(std::move(state))};
}

template <typename T>
void StreamPublisher<T>::next(T&& value) const {
  sharedState_->next(std::move(value));
}

template <typename T>
void StreamPublisher<T>::next(const T& value) const {
  if (sharedState_) {
    sharedState_->next(value);
  }
}

template <typename T>
void StreamPublisher<T>::complete(folly::exception_wrapper e) && {
  auto sharedState = std::exchange(sharedState_, nullptr);
  sharedState->complete(std::move(e));
}

template <typename T>
void StreamPublisher<T>::complete() && {
  auto sharedState = std::exchange(sharedState_, nullptr);
  sharedState->complete();
}

template <typename T>
StreamPublisher<T>::StreamPublisher(
    std::shared_ptr<detail::StreamPublisherState<T>> sharedState)
    : sharedState_(std::move(sharedState)) {}
} // namespace thrift
} // namespace apache
