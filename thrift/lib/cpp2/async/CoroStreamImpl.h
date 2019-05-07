/*
 * Copyright 2019-present Facebook, Inc.
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

#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/Baton.h>
#include <folly/experimental/coro/Task.h>

#include <thrift/lib/cpp2/async/Stream.h>

namespace apache {
namespace thrift {
namespace detail {
// this StreamImpl is for converting a coro::AsyncGenerator into
// stream on sender side
template <typename T>
class CoroStreamImpl : public StreamImplIf {
 public:
  explicit CoroStreamImpl(folly::coro::AsyncGenerator<T>&& generator)
      : generator_(std::move(generator)),
        sharedState_(std::make_shared<SharedState>()) {}

  std::unique_ptr<StreamImplIf> map(
      folly::Function<Value(Value)> mapFunc,
      folly::Function<folly::exception_wrapper(folly::exception_wrapper&&)>
          errorMapFunc) &&
      override {
    mapFuncs_.push_back(
        std::make_pair(std::move(mapFunc), std::move(errorMapFunc)));
    return std::make_unique<CoroStreamImpl>(std::move(*this));
  }

  std::unique_ptr<StreamImplIf> subscribeVia(folly::SequencedExecutor* exe) &&
      override {
    if (!sharedState_->subscribeExecutor_) {
      sharedState_->subscribeExecutor_ = folly::getKeepAliveToken(exe);
    }
    return std::make_unique<CoroStreamImpl>(std::move(*this));
  }

  std::unique_ptr<StreamImplIf> observeVia(
      folly::Executor::KeepAlive<folly::SequencedExecutor> exe) &&
      override {
    sharedState_->observeExecutor_ = std::move(exe);
    return std::make_unique<CoroStreamImpl>(std::move(*this));
  }

  void subscribe(std::unique_ptr<SubscriberIf<Value>> subscriber) && override {
    class Subscription : public SubscriptionIf {
     public:
      explicit Subscription(std::weak_ptr<SharedState> state)
          : state_(std::move(state)) {}

      void request(int64_t n) override {
        if (auto state = state_.lock()) {
          state->subscribeExecutor_->add(
              [n, state = std::move(state)]() mutable {
                state->requested_ += n;
                state->baton_.post();
              });
        }
      }

      void cancel() override {
        if (auto state = state_.lock()) {
          state->subscribeExecutor_->add([state = std::move(state)]() mutable {
            state->canceled_ = true;
            state->baton_.post();
          });
        }
      }

     private:
      std::weak_ptr<SharedState> state_;
    };
    auto sharedSubscriber =
        std::shared_ptr<SubscriberIf<Value>>(std::move(subscriber));
    auto subscription = std::make_unique<Subscription>(
        std::weak_ptr<SharedState>(sharedState_));
    sharedState_->observeExecutor_->add(
        [keepAlive = sharedState_->observeExecutor_.copy(),
         subscriber = sharedSubscriber,
         subscription = std::move(subscription)]() mutable {
          subscriber->onSubscribe(std::move(subscription));
        });
    auto executor = sharedState_->subscribeExecutor_.get();
    folly::coro::co_invoke(
        [subscriber = std::move(sharedSubscriber),
         self = std::move(*this)]() mutable -> folly::coro::Task<void> {
          typename folly::coro::AsyncGenerator<T>::async_iterator iter;
          bool started = false;
          while (true) {
            while (self.sharedState_->requested_ == 0 &&
                   !self.sharedState_->canceled_) {
              co_await self.sharedState_->baton_;
              self.sharedState_->baton_.reset();
            }

            if (self.sharedState_->canceled_) {
              co_return;
            }

            size_t i = 0;
            folly::Try<Value> value;
            try {
              if (!started) {
                iter = co_await self.generator_.begin();
                started = true;
              } else {
                co_await(++iter);
              }

              if (iter != self.generator_.end()) {
                using valueType =
                    typename folly::coro::AsyncGenerator<T>::value_type;
                value = folly::Try<Value>(
                    std::make_unique<
                        ::apache::thrift::detail::Value<valueType>>(*iter));

                for (; i < self.mapFuncs_.size(); i++) {
                  value.emplace(
                      self.mapFuncs_[i].first(std::move(value).value()));
                }
              }
            } catch (const std::exception& ex) {
              value.emplaceException(std::current_exception(), ex);
            } catch (...) {
              value.emplaceException(std::current_exception());
            }

            if (value.hasValue()) {
              self.sharedState_->observeExecutor_->add(
                  [subscriber,
                   keepAlive = self.sharedState_->observeExecutor_.copy(),
                   value = std::move(value)]() mutable {
                    subscriber->onNext(std::move(value).value());
                  });
            } else if (value.hasException()) {
              for (; i < self.mapFuncs_.size(); i++) {
                try {
                  value.emplaceException(
                      self.mapFuncs_[i].second(std::move(value).exception()));
                } catch (const std::exception& ex) {
                  value.emplaceException(std::current_exception(), ex);
                } catch (...) {
                  value.emplaceException(std::current_exception());
                }
              }
              self.sharedState_->observeExecutor_->add(
                  [subscriber,
                   keepAlive = self.sharedState_->observeExecutor_.copy(),
                   value = std::move(value)]() mutable {
                    subscriber->onError(std::move(value).exception());
                  });
              co_return;
            } else {
              self.sharedState_->observeExecutor_->add(
                  [subscriber,
                   keepAlive =
                       self.sharedState_->observeExecutor_.copy()]() mutable {
                    subscriber->onComplete();
                  });
              co_return;
            }

            if (self.sharedState_->requested_ != Stream<T>::kNoFlowControl) {
              self.sharedState_->requested_--;
            }
          }
        })
        .scheduleOn(std::move(executor))
        .start();
  }

 private:
  struct SharedState {
    folly::Executor::KeepAlive<folly::SequencedExecutor> subscribeExecutor_;
    folly::Executor::KeepAlive<folly::SequencedExecutor> observeExecutor_;
    bool canceled_{false};
    int64_t requested_{0};
    folly::coro::Baton baton_{0};
  };

  folly::coro::AsyncGenerator<T> generator_;
  std::vector<std::pair<
      folly::Function<::apache::thrift::detail::StreamImplIf::Value(
          ::apache::thrift::detail::StreamImplIf::Value)>,
      folly::Function<folly::exception_wrapper(folly::exception_wrapper&&)>>>
      mapFuncs_;

  std::shared_ptr<SharedState> sharedState_;
};
} // namespace detail

template <typename T, typename U>
Stream<typename folly::coro::AsyncGenerator<T, U>::value_type> toStream(
    folly::coro::AsyncGenerator<T, U>&& generator,
    folly::Executor::KeepAlive<folly::SequencedExecutor> executor) {
  return Stream<std::decay_t<T>>::create(
      std::make_unique<detail::CoroStreamImpl<T>>(std::move(generator)),
      std::move(executor));
}

} // namespace thrift
} // namespace apache
