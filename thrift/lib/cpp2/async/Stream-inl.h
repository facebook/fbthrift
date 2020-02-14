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

#include <cassert>

#include <folly/io/IOBuf.h>

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

  void onError(folly::exception_wrapper e) override {
    impl_->onError(std::move(e));
  }

 private:
  std::unique_ptr<SubscriberIf<T>> impl_;
};

struct EncodedError : std::exception {
  explicit EncodedError(std::unique_ptr<folly::IOBuf> buf)
      : encoded(std::move(buf)) {}

  EncodedError(const EncodedError& oth) : encoded(oth.encoded->clone()) {}
  EncodedError& operator=(const EncodedError& oth) {
    encoded = oth.encoded->clone();
    return *this;
  }
  EncodedError(EncodedError&&) = default;
  EncodedError& operator=(EncodedError&&) = default;

  std::unique_ptr<folly::IOBuf> encoded;
};

} // namespace detail

template <typename T>
Stream<T>::~Stream() {
  if (impl_) {
    impl_ = std::move(*impl_).subscribeVia(executor_);
  }
}

template <typename T>
template <typename F, typename EF>
Stream<folly::invoke_result_t<F, T&&>> Stream<T>::map(F&& f, EF&& ef) && {
  if (!impl_) {
    return {};
  }
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
          },
          std::forward<EF>(ef)),
      executor_);
}

template <typename T>
void Stream<T>::subscribe(std::unique_ptr<SubscriberIf<T>> subscriber) && {
  if (!impl_) { // empty stream
    subscriber->onSubscribe({});
    subscriber->onComplete();
  } else {
    impl_ = std::move(*impl_).subscribeVia(executor_);
    auto impl = std::move(impl_);
    std::move(*impl).subscribe(
        std::make_unique<detail::SubscriberAdaptor<T>>(std::move(subscriber)));
  }
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
        : onNext_(std::forward<OnNext>(onNext)),
          onError_(std::forward<OnError>(onError)),
          onComplete_(std::forward<OnComplete>(onComplete)) {}

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
    std::decay_t<OnNext> onNext_;
    std::decay_t<OnError> onError_;
    std::decay_t<OnComplete> onComplete_;
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
          subscriber_(std::forward<Subscriber>(subscriber)),
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
          (*rExecutor)->add([sharedState] {
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
      explicit SharedState(folly::Executor& ex) : executor(&ex) {}

      std::unique_ptr<SubscriptionIf> subscription;
      bool canceled{false};
      folly::Synchronized<folly::Executor*> executor;
    };

    std::shared_ptr<SharedState> sharedState_;

    std::decay_t<Subscriber> subscriber_;
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

template <typename T>
StreamServerCallbackPtr Stream<T>::toStreamServerCallbackPtr(
    folly::EventBase& evb) && {
  class StreamServerCallbackAdaptor final : public StreamServerCallback,
                                            public SubscriberIf<T> {
   public:
    // StreamServerCallback implementation
    bool onStreamRequestN(uint64_t tokens) override {
      if (!subscription_) {
        tokensBeforeSubscribe_ += tokens;
      } else {
        DCHECK_EQ(0, tokensBeforeSubscribe_);
        subscription_->request(tokens);
      }
      return clientCallback_;
    }
    void onStreamCancel() override {
      clientCallback_ = nullptr;
      if (auto subscription = std::move(subscription_)) {
        subscription->cancel();
      }
    }
    void resetClientCallback(StreamClientCallback& clientCallback) override {
      clientCallback_ = &clientCallback;
    }

    // SubscriberIf implementation
    void onSubscribe(std::unique_ptr<SubscriptionIf> subscription) override {
      if (!clientCallback_) {
        return subscription->cancel();
      }

      subscription_ = std::move(subscription);
      if (auto tokens = std::exchange(tokensBeforeSubscribe_, 0)) {
        subscription_->request(tokens);
      }
    }
    void onNext(T&& next) override {
      if (clientCallback_) {
        std::ignore =
            clientCallback_->onStreamNext(StreamPayload{std::move(next), {}});
      }
    }
    void onError(folly::exception_wrapper ew) override {
      if (!clientCallback_) {
        return;
      }
      std::exchange(clientCallback_, nullptr)->onStreamError(std::move(ew));
    }
    void onComplete() override {
      if (auto* clientCallback = std::exchange(clientCallback_, nullptr)) {
        clientCallback->onStreamComplete();
      }
    }

   private:
    StreamClientCallback* clientCallback_{nullptr};
    // TODO subscription_ and tokensBeforeSubscribe_ can be packed into a
    // union
    std::unique_ptr<SubscriptionIf> subscription_;
    uint32_t tokensBeforeSubscribe_{0};
  };

  auto serverCallback = std::make_unique<StreamServerCallbackAdaptor>();
  StreamServerCallbackPtr serverCallbackPtr(serverCallback.get());

  impl_ = std::move(*impl_).observeVia(&evb);
  std::move(*this).subscribe(std::move(serverCallback));

  return serverCallbackPtr;
}
} // namespace thrift
} // namespace apache
