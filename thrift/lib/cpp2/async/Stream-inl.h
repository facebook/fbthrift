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

#include <cassert>

namespace apache {
namespace thrift {
namespace detail {

template <typename T>
class SubscriberAdaptor : public SubscriberIf<std::unique_ptr<ValueIf>> {
 public:
  explicit SubscriberAdaptor(std::unique_ptr<SubscriberIf<T>> impl)
      : impl_(std::move(impl)) {}

  void onSubscribe(std::unique_ptr<SubscriptionIf> subscription) override {
    impl_->onSubscribe(std::move(subscription));
  }

  void onNext(std::unique_ptr<ValueIf>&& value) override {
    assert(dynamic_cast<detail::Value<T>*>(value.get()));
    auto* valuePtr = static_cast<detail::Value<T>*>(value.get());
    impl_->onNext(std::move(valuePtr->value));
  }

  void onComplete() override {
    impl_->onComplete();
  }

  void onError(folly::exception_wrapper e) {
    impl_->onError(std::move(e));
  }

 private:
  std::unique_ptr<SubscriberIf<T>> impl_;
};

} // namespace detail

template <typename T>
template <typename F>
Stream<folly::invoke_result_t<F, T&&>> Stream<T>::map(F&& f) && {
  using U = folly::invoke_result_t<F, T&&>;
  auto impl = std::move(impl_);
  return Stream<U>(
      std::move(*impl).map(
          [f = std::forward<F>(f)](
              std::unique_ptr<detail::ValueIf> value) mutable
          -> std::unique_ptr<detail::ValueIf> {
            assert(dynamic_cast<detail::Value<T>*>(value.get()));
            auto* valuePtr = static_cast<detail::Value<T>*>(value.get());
            return std::make_unique<detail::Value<U>>(
                std::forward<F>(f)(std::move(valuePtr->value)));
          }),
      executor_);
}

template <typename T>
void Stream<T>::subscribe(std::unique_ptr<SubscriberIf<T>> subscriber) && {
  impl_ = std::move(*impl_).subscribeVia(executor_);
  auto impl = std::move(impl_);
  std::move(*impl).subscribe(
      std::make_unique<detail::SubscriberAdaptor<T>>(std::move(subscriber)));
}

template <typename T>
template <typename OnNext, typename>
Subscription Stream<T>::subscribe(OnNext&& onNext, int64_t batch) && {
  return std::move(*this).subscribe(
      std::forward<OnNext>(onNext), [](folly::exception_wrapper) {}, batch);
}

template <typename T>
template <typename OnNext, typename OnError, typename>
Subscription
Stream<T>::subscribe(OnNext&& onNext, OnError&& onError, int64_t batch) && {
  return std::move(*this).subscribe(
      std::forward<OnNext>(onNext),
      std::forward<OnError>(onError),
      [] {},
      batch);
}

template <typename T>
template <typename OnNext, typename OnError, typename OnComplete, typename>
Subscription Stream<T>::subscribe(
    OnNext&& onNext,
    OnError&& onError,
    OnComplete&& onComplete,
    int64_t batch) && {
  class Subscriber {
   public:
    Subscriber(OnNext&& onNext, OnError&& onError, OnComplete&& onComplete)
        : onNext_(std::move(onNext)),
          onError_(std::move(onError)),
          onComplete_(std::move(onComplete)) {}

    void onNext(T&& value) {
      onNext_(std::move(value));
    }

    void onError(folly::exception_wrapper error) {
      onError_(std::move(error));
    }

    void onComplete() {
      onComplete_();
    }

   private:
    OnNext onNext_;
    OnError onError_;
    OnComplete onComplete_;
  };

  return std::move(*this).subscribe(
      Subscriber(
          std::forward<OnNext>(onNext),
          std::forward<OnError>(onError),
          std::forward<OnComplete>(onComplete)),
      batch);
}

template <typename T>
template <typename Subscriber, typename, typename>
Subscription Stream<T>::subscribe(Subscriber&& subscriber, int64_t batch) && {
  class SubscriberImpl : public SubscriberIf<T> {
   public:
    SubscriberImpl(
        folly::SequencedExecutor& executor,
        Subscriber&& subscriber,
        int64_t batch)
        : sharedState_(std::make_shared<SharedState>(executor)),
          subscriber_(std::move(subscriber)),
          batch_(batch) {
      sharedState_->executor = &executor;
    }

    ~SubscriberImpl() {
      completePromise_.setValue(folly::unit);
    }

    void onSubscribe(std::unique_ptr<SubscriptionIf> subscription) override {
      if (sharedState_->canceled) {
        subscription->cancel();
        return;
      }
      sharedState_->subscription = std::move(subscription);
      pending_ = batch_;
      sharedState_->subscription->request(batch_);
    }

    void onNext(T&& value) override {
      if (sharedState_->canceled) {
        return;
      }
      subscriber_.onNext(std::move(value));
      if (--pending_ <= batch_ / 2) {
        const auto delta = batch_ - pending_;
        pending_ += delta;
        sharedState_->subscription->request(delta);
      }
    }

    void onError(folly::exception_wrapper error) override {
      if (sharedState_->canceled) {
        return;
      }
      sharedState_->executor = nullptr;
      sharedState_->canceled = true;
      sharedState_->subscription.reset();
      subscriber_.onError(std::move(error));
    }

    void onComplete() override {
      if (sharedState_->canceled) {
        return;
      }
      sharedState_->executor = nullptr;
      sharedState_->canceled = true;
      sharedState_->subscription.reset();
      subscriber_.onComplete();
    }

    folly::Future<folly::Unit> getCompleteFuture() {
      return completePromise_.getFuture();
    }

    folly::Function<void()> getCancelFunctor() {
      return [sharedStateWeak = std::weak_ptr<SharedState>(sharedState_)] {
        if (auto sharedState = sharedStateWeak.lock()) {
          auto rExecutor = sharedState->executor.rlock();
          if (!(*rExecutor)) {
            return;
          }
          (*rExecutor)->add([sharedState = std::move(sharedState)] {
            if (sharedState->canceled) {
              return;
            }
            sharedState->canceled = true;
            if (auto subscription =
                    std::exchange(sharedState->subscription, nullptr)) {
              subscription->cancel();
            }
          });
        }
      };
    }

   private:
    struct SharedState {
      explicit SharedState(folly::Executor& executor_) : executor(&executor_) {}

      std::unique_ptr<SubscriptionIf> subscription;
      bool canceled{false};
      folly::Synchronized<folly::Executor*> executor;
    };

    std::shared_ptr<SharedState> sharedState_;

    Subscriber subscriber_;
    const int64_t batch_;
    int64_t pending_{0};
    folly::Promise<folly::Unit> completePromise_;
  };

  auto subscriberImpl = std::make_unique<SubscriberImpl>(
      *executor_, std::forward<Subscriber>(subscriber), batch);

  Subscription subscription(
      subscriberImpl->getCancelFunctor(), subscriberImpl->getCompleteFuture());
  std::move(*this).subscribe(std::move(subscriberImpl));
  return subscription;
}
} // namespace thrift
} // namespace apache
