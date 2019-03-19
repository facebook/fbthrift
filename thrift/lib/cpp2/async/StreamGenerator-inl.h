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

#include <folly/Optional.h>
#include <folly/futures/Future.h>
#include <folly/futures/helpers.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

#include <yarpl/Flowable.h>
#include <yarpl/flowable/EmitterFlowable.h>

namespace apache {
namespace thrift {

namespace detail {

template <typename T>
class FutureEmitterWrapper : public yarpl::flowable::details::EmiterBase<T>,
                             public yarpl::flowable::Flowable<T> {
 public:
  template <typename F>
  explicit FutureEmitterWrapper(F&& emitter)
      : emitter_(std::forward<F>(emitter)) {}

  void subscribe(
      std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber) override {
    auto ef = std::make_shared<yarpl::flowable::details::EmiterSubscription<T>>(
        this->ref_from_this(this), std::move(subscriber));
    ef->init();
  }

  std::tuple<int64_t, bool> emit(
      std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber,
      int64_t requested) override {
    // done will always be false in async mode, since there is no way to know
    // whether the flow is done or not at the Future issuing time
    // We let the subscriber->onComplete() part to pass back the signal
    bool done = false;
    prev_ = emitter_(subscriber, requested, std::move(prev_));
    return std::make_tuple(requested, done);
  }

 private:
  // represent previous batch's state, will fullfil true (proceed) if previous
  // part of the stream doesn't encounter onComplete or onError
  folly::Future<bool> prev_ = folly::makeFuture(true);
  folly::Function<folly::Future<bool>(
      std::shared_ptr<yarpl::flowable::Subscriber<T>>,
      int64_t,
      folly::Future<bool>)>
      emitter_;
};

template <typename Generator, typename Element>
Stream<Element>
StreamGeneratorImpl<Generator, folly::Optional<Element>>::create(
    folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
    Generator&& generator) {
  auto flowable = yarpl::flowable::Flowable<Element>::create(
      [generator = std::forward<Generator>(generator)](
          yarpl::flowable::Subscriber<Element>& subscriber,
          int64_t requested) mutable {
        try {
          while (requested-- > 0) {
            if (auto value = generator()) {
              subscriber.onNext(std::move(*value));
            } else {
              subscriber.onComplete();
              break;
            }
          }
        } catch (const std::exception& ex) {
          subscriber.onError(
              folly::exception_wrapper(std::current_exception(), ex));
        } catch (...) {
          subscriber.onError(
              folly::exception_wrapper(std::current_exception()));
        }
      });
  return toStream(std::move(flowable), std::move(executor));
}

template <typename Generator, typename Element>
Stream<Element>
StreamGeneratorImpl<Generator, folly::SemiFuture<folly::Optional<Element>>>::
    create(
        folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
        Generator&& generator) {
  std::shared_ptr<yarpl::flowable::Flowable<Element>> flowable =
      std::make_shared<FutureEmitterWrapper<Element>>(
          [generator = std::forward<Generator>(generator),
           executor = executor.copy()](
              std::shared_ptr<yarpl::flowable::Subscriber<Element>> subscriber,
              int64_t requested,
              folly::Future<bool> prevBatch) mutable {
            std::vector<folly::Future<folly::Optional<Element>>> v;
            while (requested-- > 0) {
              auto f = generator();
              v.push_back(std::move(f).via(executor));
            }

            return std::move(prevBatch).then(
                [subscriber, v = std::move(v)](
                    folly::Try<bool>&& proceed) mutable -> folly::Future<bool> {
                  return folly::reduce(
                      std::move(v),
                      std::move(proceed).value(),
                      [subscriber](
                          bool proceed,
                          folly::Try<folly::Optional<Element>>&&
                              result) mutable {
                        if (!proceed) {
                          return false;
                        }

                        if (result.hasValue()) {
                          if (result.value().hasValue()) {
                            subscriber->onNext(
                                std::move(result).value().value());
                            return true;
                          } else {
                            subscriber->onComplete();
                            return false;
                          }
                        } else {
                          subscriber->onError(std::move(result).exception());
                          return false;
                        }
                      });
                });
          });

  return toStream(std::move(flowable), std::move(executor));
}
} // namespace detail

} // namespace thrift
} // namespace apache
