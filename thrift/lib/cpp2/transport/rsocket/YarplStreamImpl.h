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

class YarplStreamImpl : public StreamImplIf {
 public:
  explicit YarplStreamImpl(
      std::shared_ptr<yarpl::flowable::Flowable<Value>> flowable)
      : flowable_(std::move(flowable)) {}

  std::unique_ptr<StreamImplIf> map(
      folly::Function<Value(Value)> mapFunc) override {
    return std::make_unique<YarplStreamImpl>(
        flowable_->map(std::move(mapFunc)));
  }
  void subscribe(std::unique_ptr<SubscriberIf<Value>> subscriber) override {
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

    flowable_->subscribe(
        std::make_shared<SubscriberAdaptor>(std::move(subscriber)));
  }
  void observeVia(folly::SequencedExecutor* executor) {
    flowable_ = flowable_->observeOn(*executor);
  }
  void subscribeVia(folly::SequencedExecutor* executor) {
    flowable_ = flowable_->subscribeOn(*executor);
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
                impl_->cancel();
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
