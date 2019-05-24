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
#include <thrift/lib/cpp2/transport/rocket/client/RocketClientFlowable.h>
#include <yarpl/Flowable.h>

namespace apache {
namespace thrift {
namespace detail {

using Funcs = std::vector<std::pair<
    folly::Function<::apache::thrift::detail::StreamImplIf::Value(
        ::apache::thrift::detail::StreamImplIf::Value)>,
    folly::Function<folly::exception_wrapper(folly::exception_wrapper&&)>>>;

class RocketStreamImpl : public StreamImplIf {
 public:
  explicit RocketStreamImpl(
      std::shared_ptr<apache::thrift::rocket::RocketClientFlowable> flowable)
      : flowable_(std::move(flowable)) {}

  RocketStreamImpl(RocketStreamImpl&&) = default;

  ~RocketStreamImpl() {
    class CancellingSubscriber
        : public yarpl::flowable::Subscriber<rocket::Payload> {
     public:
      CancellingSubscriber(folly::SequencedExecutor* subscribeExecutor)
          : subscribeExecutor_(subscribeExecutor) {}
      void onSubscribe(std::shared_ptr<yarpl::flowable::Subscription>
                           subscription) override {
        subscribeExecutor_->add([subscription]() { subscription->cancel(); });
      }
      void onNext(rocket::Payload) override {}
      void onComplete() override {}
      void onError(folly::exception_wrapper) override {}
      folly::SequencedExecutor* subscribeExecutor_;
    };

    if (auto flowable = std::move(flowable_)) {
      flowable->subscribe(
          std::make_shared<CancellingSubscriber>(subscribeExecutor_));
    }
  }

  std::unique_ptr<StreamImplIf> map(
      folly::Function<Value(Value)> mapFunc,
      folly::Function<folly::exception_wrapper(folly::exception_wrapper&&)>
          errormapFunc) &&
      override {
    mapFuncs_.push_back(
        std::make_pair(std::move(mapFunc), std::move(errormapFunc)));
    return std::make_unique<RocketStreamImpl>(std::move(*this));
  }

  std::unique_ptr<StreamImplIf> observeVia(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor) &&
      override {
    observeExecutor_ = std::move(executor);
    return std::make_unique<RocketStreamImpl>(std::move(*this));
  }
  std::unique_ptr<StreamImplIf>
      subscribeVia(folly::SequencedExecutor* executor) && override {
    if (!subscribeExecutor_) {
      subscribeExecutor_ = executor;
    }
    return std::make_unique<RocketStreamImpl>(std::move(*this));
  }

  void subscribe(std::unique_ptr<SubscriberIf<Value>> subscriber) && override {
    class SubscriberAdaptor
        : public yarpl::flowable::Subscriber<rocket::Payload>,
          public yarpl::enable_get_ref {
     public:
      explicit SubscriberAdaptor(
          std::unique_ptr<SubscriberIf<std::unique_ptr<ValueIf>>> impl,
          std::unique_ptr<RocketStreamImpl> streamImpl)
          : impl_(std::move(impl)), streamImpl_(std::move(streamImpl)) {}

      void onSubscribe(std::shared_ptr<yarpl::flowable::Subscription>
                           subscription) override {
        class SubscriptionAdaptor : public SubscriptionIf {
         public:
          explicit SubscriptionAdaptor(
              std::shared_ptr<yarpl::flowable::Subscription> impl,
              folly::SequencedExecutor* subscribeExecutor)
              : impl_(std::move(impl)), subscribeExecutor_(subscribeExecutor) {}

          void request(int64_t n) override {
            subscribeExecutor_->add([impl = impl_, n]() { impl->request(n); });
          }
          void cancel() override {
            subscribeExecutor_->add([impl = impl_]() { impl->cancel(); });
          }

         private:
          std::shared_ptr<yarpl::flowable::Subscription> impl_;
          folly::SequencedExecutor* subscribeExecutor_;
        };

        impl_->onSubscribe(std::make_unique<SubscriptionAdaptor>(
            std::move(subscription), streamImpl_->subscribeExecutor_));
      }
      void onComplete() override {
        streamImpl_->observeExecutor_->add(
            [self = this->ref_from_this(this)]() mutable {
              self->impl_->onComplete();
            });
      }

      void onError(folly::exception_wrapper ew) override {
        streamImpl_->observeExecutor_->add(
            [self = this->ref_from_this(this), ew = std::move(ew)]() mutable {
              for (auto& func : self->streamImpl_->mapFuncs_) {
                ew = func.second(std::move(ew));
              }
              self->impl_->onError(std::move(ew));
            });
      }

      void onNext(rocket::Payload payload) override {
        streamImpl_->observeExecutor_->add([self = this->ref_from_this(this),
                                            n = std::move(payload)]() mutable {
          ::apache::thrift::detail::StreamImplIf::Value value =
              std::make_unique<
                  ::apache::thrift::detail::Value<rocket::Payload>>(
                  std::move(n));
          for (auto& func : self->streamImpl_->mapFuncs_) {
            value = func.first(std::move(value));
          }
          if (self->impl_) {
            self->impl_->onNext(std::move(value));
          }
        });
      }

     private:
      std::unique_ptr<SubscriberIf<std::unique_ptr<ValueIf>>> impl_;
      std::unique_ptr<RocketStreamImpl> streamImpl_;
    };

    auto flowable = std::move(flowable_);
    flowable->subscribe(std::make_shared<SubscriberAdaptor>(
        std::move(subscriber),
        std::make_unique<RocketStreamImpl>(std::move(*this))));
  }

 private:
  std::shared_ptr<apache::thrift::rocket::RocketClientFlowable> flowable_;
  Funcs mapFuncs_;

  folly::SequencedExecutor* subscribeExecutor_ = nullptr;
  folly::Executor::KeepAlive<> observeExecutor_;
};

} // namespace detail
} // namespace thrift
} // namespace apache
