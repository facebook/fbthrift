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

#include <thrift/lib/cpp2/async/Stream.h>
#include <yarpl/Flowable.h>

namespace apache {
namespace thrift {
namespace detail {

template <typename T>
class EagerSubscribeOnOperator
    : public yarpl::flowable::FlowableOperator<T, T> {
  using Super = yarpl::flowable::FlowableOperator<T, T>;

 public:
  EagerSubscribeOnOperator(
      std::shared_ptr<yarpl::flowable::Flowable<T>> upstream,
      folly::SequencedExecutor& executor)
      : subscription_(std::make_shared<Subscription>(executor)) {
    executor.add(
        [upstream = std::move(upstream), subscription = subscription_] {
          upstream->subscribe(subscription);
        });
  }

  void subscribe(
      std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber) override {
    auto subscription = std::move(subscription_);
    subscription->setSubscriber(std::move(subscriber));
  }

  ~EagerSubscribeOnOperator() {
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
    explicit Subscription(folly::SequencedExecutor& executor)
        : executor_(&executor) {}

    void request(int64_t delta) override {
      auto rExecutor = executor_.rlock();
      if (auto executorPtr = *rExecutor) {
        executorPtr->add([delta, this, self = this->ref_from_this(this)] {
          this->callSuperRequest(delta);
        });
      }
    }

    void cancel() override {
      auto rExecutor = executor_.rlock();
      if (auto executorPtr = *rExecutor) {
        executorPtr->add([this, self = this->ref_from_this(this)] {
          this->callSuperCancel();
        });
      }
    }

    void onNextImpl(T value) override {
      DCHECK(state_ == State::HAS_SUBSCRIBER);
      SuperSubscription::subscriberOnNext(std::move(value));
    }

    void onCompleteImpl() override {
      *executor_.wlock() = nullptr;
      auto state = state_.load();
      switch (state) {
        case State::SUBSCRIBED:
          if (state_.compare_exchange_strong(
                  state,
                  State::COMPLETED,
                  std::memory_order_release,
                  std::memory_order_acquire)) {
            return;
          }
          DCHECK(state == State::HAS_SUBSCRIBER);
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
      *executor_.wlock() = nullptr;
      error_ = std::move(error);
      auto state = state_.load();
      switch (state) {
        case State::SUBSCRIBED:
          if (state_.compare_exchange_strong(
                  state,
                  State::HAS_ERROR,
                  std::memory_order_release,
                  std::memory_order_acquire)) {
            return;
          }
          DCHECK(state == State::HAS_SUBSCRIBER);
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
      auto state = state_.load();
      switch (state) {
        case State::EMPTY:
          if (state_.compare_exchange_strong(
                  state,
                  State::SUBSCRIBED,
                  std::memory_order_release,
                  std::memory_order_acquire)) {
            return;
          }
          DCHECK(state == State::HAS_SUBSCRIBER);
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
      auto state = state_.load();
      bool subscribed = false;
      do {
        switch (state) {
          case State::EMPTY:
            break;
          case State::SUBSCRIBED:
            SuperSubscription::onSubscribeImpl();
            subscribed = true;
            break;
          case State::COMPLETED:
            if (!subscribed) {
              SuperSubscription::onSubscribeImpl();
            }
            SuperSubscription::onCompleteImpl();
            return;
          case State::HAS_ERROR:
            if (!subscribed) {
              SuperSubscription::onSubscribeImpl();
            }
            SuperSubscription::onErrorImpl(std::move(error_));
            return;
          case State::HAS_SUBSCRIBER:
            LOG(FATAL) << "Invalid state";
        }
      } while (!state_.compare_exchange_weak(
          state,
          State::HAS_SUBSCRIBER,
          std::memory_order_release,
          std::memory_order_acquire));
    }

   private:
    // Trampoline to call superclass method; gcc bug 58972.
    void callSuperRequest(int64_t delta) {
      SuperSubscription::request(delta);
    }

    // Trampoline to call superclass method; gcc bug 58972.
    void callSuperCancel() {
      SuperSubscription::cancel();
    }

    enum class State {
      EMPTY,
      SUBSCRIBED,
      HAS_SUBSCRIBER,
      HAS_ERROR,
      COMPLETED
    };

    std::atomic<State> state_{State::EMPTY};
    folly::Synchronized<folly::Executor*> executor_;
    folly::exception_wrapper error_;
  };

  std::shared_ptr<Subscription> subscription_;
};

class YarplStreamImpl : public StreamImplIf {
 public:
  explicit YarplStreamImpl(
      std::shared_ptr<yarpl::flowable::Flowable<Value>> flowable)
      : flowable_(std::move(flowable)) {}

  std::unique_ptr<StreamImplIf> map(folly::Function<Value(Value)> mapFunc) &&
      override {
    return std::make_unique<YarplStreamImpl>(
        flowable_->map(std::move(mapFunc)));
  }
  std::unique_ptr<StreamImplIf> observeVia(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor) &&
      override {
    return std::make_unique<YarplStreamImpl>(
        flowable_->observeOn(std::move(executor)));
  }
  std::unique_ptr<StreamImplIf>
      subscribeVia(folly::SequencedExecutor* executor) && override {
    return std::make_unique<YarplStreamImpl>(
        std::make_shared<EagerSubscribeOnOperator<Value>>(
            std::move(flowable_), *executor));
  }

  void subscribe(std::unique_ptr<SubscriberIf<Value>> subscriber) && override {
    class SubscriberAdaptor
        : public yarpl::flowable::Subscriber<std::unique_ptr<ValueIf>> {
     public:
      explicit SubscriberAdaptor(
          std::unique_ptr<SubscriberIf<std::unique_ptr<ValueIf>>> impl)
          : impl_(std::move(impl)) {}

      void onSubscribe(std::shared_ptr<yarpl::flowable::Subscription>
                           subscription) override {
        class SubscriptionAdaptor : public SubscriptionIf {
         public:
          explicit SubscriptionAdaptor(
              std::shared_ptr<yarpl::flowable::Subscription> impl)
              : impl_(std::move(impl)) {}

          void request(int64_t n) override {
            impl_->request(n);
          }
          void cancel() override {
            impl_->cancel();
          }

         private:
          std::shared_ptr<yarpl::flowable::Subscription> impl_;
        };

        impl_->onSubscribe(
            std::make_unique<SubscriptionAdaptor>(std::move(subscription)));
      }
      void onComplete() override {
        impl_->onComplete();
      }
      void onError(folly::exception_wrapper e) {
        impl_->onError(std::move(e));
      }
      void onNext(std::unique_ptr<ValueIf> value) {
        impl_->onNext(std::move(value));
      }

     private:
      std::unique_ptr<SubscriberIf<std::unique_ptr<ValueIf>>> impl_;
    };

    auto flowable = std::move(flowable_);
    flowable->subscribe(
        std::make_shared<SubscriberAdaptor>(std::move(subscriber)));
  }

 private:
  std::shared_ptr<yarpl::flowable::Flowable<Value>> flowable_;
};
} // namespace detail

template <typename T>
Stream<T> toStream(
    std::shared_ptr<yarpl::flowable::Flowable<T>> flowable,
    folly::SequencedExecutor* executor) {
  return Stream<T>::create(
      std::make_unique<detail::YarplStreamImpl>(
          flowable->map([](T&& value) -> std::unique_ptr<detail::ValueIf> {
            return std::make_unique<detail::Value<T>>(std::move(value));
          })),
      executor);
}

template <typename T>
std::shared_ptr<yarpl::flowable::Flowable<T>> toFlowable(Stream<T> stream) {
  return yarpl::flowable::internal::flowableFromSubscriber<T>(
      [stream = std::move(stream)](
          std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber) mutable {
        class SubscriberAdaptor : public SubscriberIf<T> {
         public:
          explicit SubscriberAdaptor(
              std::shared_ptr<yarpl::flowable::Subscriber<T>> impl)
              : impl_(std::move(impl)) {}
          void onSubscribe(
              std::unique_ptr<SubscriptionIf> subscription) override {
            class SubscriptionAdaptor : public yarpl::flowable::Subscription {
             public:
              explicit SubscriptionAdaptor(std::unique_ptr<SubscriptionIf> impl)
                  : impl_(std::move(impl)) {}
              void request(int64_t n) override {
                impl_->request(n);
              }
              void cancel() override {
                if (auto impl = std::move(impl_)) {
                  impl->cancel();
                }
              }

             private:
              std::unique_ptr<SubscriptionIf> impl_;
            };
            impl_->onSubscribe(
                std::make_unique<SubscriptionAdaptor>(std::move(subscription)));
          }
          void onNext(T&& value) override {
            impl_->onNext(std::move(value));
          }
          void onComplete() override {
            impl_->onComplete();
          }
          void onError(folly::exception_wrapper e) {
            impl_->onError(std::move(e));
          }

         private:
          std::shared_ptr<yarpl::flowable::Subscriber<T>> impl_;
        };

        std::move(stream).subscribe(
            std::make_unique<SubscriberAdaptor>(std::move(subscriber)));
      });
}
} // namespace thrift
} // namespace apache
