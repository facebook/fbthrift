/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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
  StreamPublisherState(
      folly::Function<void()> onCompleteOrCanceled,
      folly::Executor::KeepAlive<> executor)
      : state_(folly::in_place, std::move(executor)),
        onCompleteOrCanceled_(std::move(onCompleteOrCanceled)) {}

  std::shared_ptr<yarpl::observable::Subscription> subscribe(
      std::shared_ptr<yarpl::observable::Observer<T>> observer) override {
    auto self = this->ref_from_this(this);
    state_.wlock()->init(std::move(observer), self);
    return self;
  }

  void cancel() override {
    if (state_.wlock()->cancel()) {
      std::exchange(onCompleteOrCanceled_, nullptr)();
    }
  }

  void next(T&& value) {
    state_.rlock()->next(std::move(value));
  }

  void next(const T& value) {
    state_.rlock()->next(value);
  }

  void complete(folly::exception_wrapper e) {
    if (state_.wlock()->complete(std::move(e))) {
      std::exchange(onCompleteOrCanceled_, nullptr)();
    }
  }

  void complete() {
    if (state_.wlock()->complete()) {
      std::exchange(onCompleteOrCanceled_, nullptr)();
    }
  }

  void release() {
    if (onCompleteOrCanceled_) {
      LOG(FATAL) << "StreamPublisher has to be completed or canceled.";
    }
  }

 private:
  class State {
   public:
    explicit State(folly::Executor::KeepAlive<> executor)
        : executor_(std::move(executor)) {}

    bool cancel() {
      if (!executor_) {
        return false;
      }
      executor_.reset();
      observer_ = nullptr;
      return true;
    }

    void init(
        std::shared_ptr<yarpl::observable::Observer<T>> observer,
        std::shared_ptr<yarpl::observable::Subscription> subscription) {
      DCHECK(!observer_);
      DCHECK(executor_);

      executor_->add(
          [observer, subscription = std::move(subscription)]() mutable {
            observer->onSubscribe(std::move(subscription));
          });
      observer_ = std::move(observer);
    }

    void next(T&& value) const {
      if (executor_) {
        executor_->add(
            [observer = observer_, value = std::move(value)]() mutable {
              observer->onNext(std::move(value));
            });
      }
    }

    void next(const T& value) const {
      if (executor_) {
        executor_->add([observer = observer_, value]() mutable {
          observer->onNext(std::move(value));
        });
      }
    }

    bool complete(folly::exception_wrapper e) {
      if (executor_) {
        executor_->add([observer = observer_, e = std::move(e)]() mutable {
          observer->onError(std::move(e));
        });
      }
      return cancel();
    }

    bool complete() {
      if (executor_) {
        executor_->add(
            [observer = observer_]() mutable { observer->onComplete(); });
      }
      return cancel();
    }

   private:
    folly::Executor::KeepAlive<> executor_;
    std::shared_ptr<yarpl::observable::Observer<T>> observer_;
  };
  folly::Synchronized<State> state_;
  folly::Function<void()> onCompleteOrCanceled_;
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
    folly::Function<void()> onCanceled,
    size_t bufferSizeLimit) {
  auto state = std::make_shared<detail::StreamPublisherState<T>>(
      std::move(onCanceled), folly::getKeepAliveToken(executor.get()));
  auto flowable = state->toFlowable(
      std::make_shared<yarpl::BufferBackpressureStrategy<T>>(bufferSizeLimit));
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
