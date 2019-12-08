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

#include <folly/Optional.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/Task.h>
#endif
#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

#if FOLLY_HAS_COROUTINES
#include <thrift/lib/cpp2/async/CoroStreamImpl.h>
#endif

#include <yarpl/Flowable.h>
#include <yarpl/flowable/EmitterFlowable.h>

namespace apache {
namespace thrift {

namespace detail {
constexpr int64_t kStreamEnd = -1;
template <typename T>
class FutureEmitterWrapper : public yarpl::flowable::details::EmitterBase<T>,
                             public yarpl::flowable::Flowable<T> {
 public:
  template <typename F>
  FutureEmitterWrapper(F&& emitter, int64_t maxBatchSize)
      : emitter_(std::forward<F>(emitter)), maxBatchSize_(maxBatchSize) {}

  void subscribe(
      std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber) override {
    auto ef = std::make_shared<yarpl::flowable::details::EmiterSubscription<T>>(
        this->ref_from_this(this), std::move(subscriber));
    ef->init();
  }

  std::tuple<int64_t, bool> emit(
      std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber,
      int64_t requested) override {
    if (requested == 0) {
      return std::make_tuple(requested /* emitted */, false /* done */);
    }

    if (prev_.isReady() && prev_.hasValue() && prev_.value() == kStreamEnd) {
      return std::make_tuple(0 /* emitted */, true /* done */);
    }

    prev_ =
        std::move(prev_)
            .thenValue(
                [requested,
                 subscriber,
                 selfWeak = std::weak_ptr<FutureEmitterWrapper<T>>(
                     this->ref_from_this(this))](
                    int64_t credit) mutable -> folly::Future<int64_t> {
                  if (credit == kStreamEnd) {
                    return kStreamEnd;
                  }

                  DCHECK(credit == 0)
                      << "previous batch should have been consumed over";
                  if (auto self = selfWeak.lock()) {
                    return self->batchProcessCredits(
                        requested, std::move(subscriber));
                  } else {
                    return kStreamEnd;
                  }
                })
            .thenError(
                [subscriber](folly::exception_wrapper&& ew) mutable -> int64_t {
                  subscriber->onError(std::move(ew));
                  return kStreamEnd;
                });

    return std::make_tuple(requested /* emitted */, false /* done */);
  }

 private:
  // return credits lefted over after this batch process
  folly::Future<int64_t> batchProcessCredits(
      int64_t remainingCredits,
      std::shared_ptr<yarpl::flowable::Subscriber<T>> subscriber) {
    if (remainingCredits == kStreamEnd) {
      return kStreamEnd;
    }

    auto batchSize = std::min(maxBatchSize_, remainingCredits);
    if (remainingCredits != Stream<T>::kNoFlowControl) {
      remainingCredits = remainingCredits - batchSize;
    }

    folly::Optional<folly::Future<int64_t>> f;
    DCHECK(batchSize > 0) << "batchSize value must > 0, batchSize: "
                          << batchSize;

    std::vector<folly::Future<folly::Optional<T>>> v;
    for (int i = 0; i < batchSize; i++) {
      v.push_back(emitter_());
    }

    f = folly::reduce(
        std::move(v),
        std::move(remainingCredits),
        [subscriber](
            int64_t credits,
            folly::Try<folly::Optional<T>>&& result) mutable -> int64_t {
          if (credits == kStreamEnd) {
            return kStreamEnd;
          }

          if (result.hasValue()) {
            if (result.value().hasValue()) {
              subscriber->onNext(std::move(result).value().value());
              return credits;
            } else {
              subscriber->onComplete();
              return kStreamEnd;
            }
          } else {
            subscriber->onError(std::move(result).exception());
            return kStreamEnd;
          }
        });

    if (remainingCredits == 0) {
      return std::move(f).value();
    } else {
      return std::move(f)
          .value()
          .thenValue(
              [subscriber,
               selfWeak = std::weak_ptr<FutureEmitterWrapper<T>>(
                   this->ref_from_this(this))](
                  int64_t credits) mutable -> folly::Future<int64_t> {
                if (auto self = selfWeak.lock()) {
                  return self->batchProcessCredits(
                      credits, std::move(subscriber));
                } else {
                  return kStreamEnd;
                }
              })
          .thenError(
              [subscriber](folly::exception_wrapper&& ew) mutable -> int64_t {
                subscriber->onError(std::move(ew));
                return kStreamEnd;
              });
    }
  }

  // represent previous batch's state, how many credit left from the previous
  // round. use kStreamEnd represent encountering complete/error signal
  folly::Future<int64_t> prev_ = folly::makeFuture(0);
  folly::Function<folly::Future<folly::Optional<T>>()> emitter_;

  const int64_t maxBatchSize_;
};

} // namespace detail

template <
    typename Generator,
    std::enable_if_t<
        detail::is_optional<
            typename folly::invoke_result_t<std::decay_t<Generator>&>>::value,
        int>>
auto StreamGenerator::create(
    folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
    Generator&& generator)
    -> Stream<
        typename folly::invoke_result_t<std::decay_t<Generator>&>::value_type> {
  using Element =
      typename folly::invoke_result_t<std::decay_t<Generator>&>::value_type;
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

template <
    typename Generator,
    std::enable_if_t<
        detail::is_future_optional<
            typename folly::invoke_result_t<std::decay_t<Generator>&>>::value,
        int>>
auto StreamGenerator::create(
    folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
    Generator&& generator,
    int maxBatchSize)
    -> Stream<typename folly::invoke_result_t<
        std::decay_t<Generator>&>::value_type::value_type> {
  using Element = typename folly::invoke_result_t<
      std::decay_t<Generator>&>::value_type::value_type;

  DCHECK(maxBatchSize > 0) << "maxBatchSize must > 0";
  std::shared_ptr<yarpl::flowable::Flowable<Element>> flowable =
      std::make_shared<detail::FutureEmitterWrapper<Element>>(
          [generator = std::forward<Generator>(generator),
           executor = executor.copy()]() mutable
          -> folly::Future<folly::Optional<Element>> {
            return generator().via(executor);
          },
          maxBatchSize);

  return toStream(std::move(flowable), std::move(executor));
}

#if FOLLY_HAS_COROUTINES
template <
    typename Generator,
    std::enable_if_t<
        folly::detail::is_instantiation_of_v<
            folly::coro::AsyncGenerator,
            typename folly::invoke_result_t<std::decay_t<Generator>&>>,
        int>>
auto StreamGenerator::create(
    folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
    Generator&& generator)
    -> Stream<
        typename folly::invoke_result_t<std::decay_t<Generator>&>::value_type> {
  return toCoroStream(
      folly::coro::co_invoke(std::forward<Generator>(generator)),
      std::move(executor));
}

template <
    typename Generator,
    std::enable_if_t<
        folly::detail::is_instantiation_of_v<
            folly::coro::AsyncGenerator,
            typename folly::invoke_result_t<
                std::decay_t<Generator>&,
                folly::CancellationToken>>,
        int>>
auto StreamGenerator::create(
    folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
    Generator&& generator)
    -> Stream<typename folly::invoke_result_t<
        std::decay_t<Generator>&,
        folly::CancellationToken>::value_type> {
  folly::CancellationSource source;
  auto token = source.getToken();
  return toCoroStream(
      folly::coro::co_invoke(
          std::forward<Generator>(generator), std::move(token)),
      std::move(executor),
      std::move(source));
}
#endif

} // namespace thrift
} // namespace apache
