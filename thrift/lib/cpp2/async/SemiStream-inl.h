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

#include <atomic>
#include <cassert>
#include <deque>
#include <memory>
#include <mutex>

#include <folly/ScopeGuard.h>
#include <folly/Synchronized.h>
#include <folly/Try.h>
#include <folly/executors/SerialExecutor.h>
#include <folly/fibers/Semaphore.h>

namespace apache {
namespace thrift {

template <typename T>
template <typename F, typename EF>
SemiStream<folly::invoke_result_t<F, T&&>> SemiStream<T>::map(
    F&& f,
    EF&& ef) && {
  using U = folly::invoke_result_t<F, T&&>;

  SemiStream<U> result;
  result.impl_ = std::move(impl_);
  result.mapFuncs_ = std::move(mapFuncs_);
  result.mapFuncs_.push_back(
      {[mf = std::forward<F>(f)](std::unique_ptr<detail::ValueIf> value) mutable
       -> std::unique_ptr<detail::ValueIf> {
         assert(dynamic_cast<detail::Value<T>*>(value.get()));
         auto* valuePtr = static_cast<detail::Value<T>*>(value.get());
         return std::make_unique<detail::Value<U>>(
             std::forward<F>(mf)(std::move(valuePtr->value)));
       },
       std::forward<EF>(ef)});
  result.executor_ = std::move(executor_);
  return result;
}

template <typename T>
Stream<T> SemiStream<T>::via(
    folly::Executor::KeepAlive<folly::SequencedExecutor> executor) && {
  auto impl = std::move(impl_);
  auto executorPtr = executor.get();
  impl = std::move(*impl).observeVia(std::move(executor));
  for (auto& mapFunc : mapFuncs_) {
    impl = std::move(*impl).map(
        std::move(mapFunc.first), std::move(mapFunc.second));
  }
  return Stream<T>(std::move(impl), executorPtr);
}

template <typename T>
Stream<T> SemiStream<T>::via(folly::SequencedExecutor* executor) && {
  return std::move(*this).via(folly::getKeepAliveToken(executor));
}

template <typename T>
Stream<T> SemiStream<T>::via(folly::Executor* executor) && {
  return std::move(*this).via(
      folly::SerialExecutor::create(folly::getKeepAliveToken(executor)));
}

#if FOLLY_HAS_COROUTINES
template <typename T>
folly::coro::AsyncGenerator<T&&> SemiStream<T>::toAsyncGenerator(
    SemiStream<T> stream,
    int64_t bufferSize,
    folly::CancellationToken cancellationToken) {
  struct SharedState {
    explicit SharedState(int64_t size) : size_(size) {}
    folly::Synchronized<
        std::deque<folly::Try<std::unique_ptr<detail::ValueIf>>>,
        std::mutex>
        buffer_;
    const int64_t size_;
    folly::fibers::Semaphore sem_{0};
    bool terminated_{false};
    std::unique_ptr<SubscriptionIf> subscription_;
  };

  class Subscriber : public SubscriberIf<std::unique_ptr<detail::ValueIf>> {
   public:
    explicit Subscriber(std::shared_ptr<SharedState> sharedState)
        : sharedState_(std::move(sharedState)) {}

    void onSubscribe(std::unique_ptr<SubscriptionIf> subscription) override {
      if (sharedState_->terminated_) {
        subscription->cancel();
      } else {
        DCHECK(!sharedState_->subscription_);
        sharedState_->subscription_ = std::move(subscription);
        sharedState_->subscription_->request(sharedState_->size_);
      }
    }

    void onNext(std::unique_ptr<detail::ValueIf>&& v) override {
      if (sharedState_->terminated_) {
        return;
      }

      sharedState_->buffer_->emplace_back(std::move(v));
      sharedState_->sem_.signal();
    }

    void onError(folly::exception_wrapper error) override {
      if (sharedState_->terminated_) {
        return;
      }

      sharedState_->subscription_.reset();
      sharedState_->terminated_ = true;
      sharedState_->buffer_->emplace_back(std::move(error));
      sharedState_->sem_.signal();
    }

    void onComplete() override {
      if (sharedState_->terminated_) {
        return;
      }

      sharedState_->subscription_.reset();
      sharedState_->terminated_ = true;
      sharedState_->buffer_->emplace_back(
          folly::Try<std::unique_ptr<detail::ValueIf>>());
      sharedState_->sem_.signal();
    }

   private:
    std::shared_ptr<SharedState> sharedState_;
  };

  std::shared_ptr<SharedState> sharedState =
      std::make_shared<SharedState>(bufferSize);

  auto keepAlive = folly::getKeepAliveToken(stream.executor_);
  std::move(*(stream.impl_))
      .subscribe(std::make_unique<Subscriber>(sharedState));

  std::atomic<bool> cancellationRequested{false};

  auto requestStreamCancellation =
      [&, keepAlive = std::move(keepAlive)]() mutable {
        if (cancellationRequested.exchange(true, std::memory_order_relaxed)) {
          // Cancellation already requested
          return;
        }

        stream.executor_->add([keepAlive = std::move(keepAlive),
                               sharedStateWeak = std::weak_ptr<SharedState>(
                                   sharedState)]() mutable {
          if (auto sharedState = sharedStateWeak.lock()) {
            sharedState->terminated_ = true;
            if (sharedState->subscription_) {
              auto subscription = std::move(sharedState->subscription_);
              subscription->cancel();
            }
          }
        });
      };

  SCOPE_EXIT {
    // Cancel the stream on exit.
    requestStreamCancellation();
  };

  // Cancel the stream if cancellation is requested via the CancellationToken
  // but also wake up the coroutine if it was suspended waiting for the next
  // item.
  folly::CancellationCallback cancelCallback{cancellationToken, [&] {
                                               requestStreamCancellation();
                                               sharedState->sem_.signal();
                                             }};

  int64_t counter = 0;
  // application buffer
  std::deque<folly::Try<std::unique_ptr<detail::ValueIf>>> appBuffer;
  while (true) {
    co_await sharedState->sem_.co_wait();

    // If cancellation was requested then ignore any buffered results
    // and just complete immediately with the end-of-sequence.
    if (cancellationToken.isCancellationRequested()) {
      // QUESTION: Should we be throwing an OperationCancelled exception here
      // rather than truncating the stream? How will the consumer know that
      // the stream was truncated? The consumer will currently have to also
      // inspect the cancellation token to determine this.
      co_return;
    }

    if (appBuffer.size() == 0) {
      auto buffer = sharedState->buffer_.lock();
      std::swap(*buffer, appBuffer);
    }

    auto& ele = appBuffer.front();
    SCOPE_EXIT {
      appBuffer.pop_front();
    };

    if (ele.hasValue() || ele.hasException()) {
      for (auto& mapFunc : stream.mapFuncs_) {
        try {
          if (ele.hasValue()) {
            ele.emplace(mapFunc.first(std::move(ele.value())));
          } else {
            ele.emplaceException(mapFunc.second(std::move(ele.exception())));
          }
        } catch (const std::exception& ex) {
          ele.emplaceException(std::current_exception(), ex);
        } catch (...) {
          ele.emplaceException(std::current_exception());
        }
      }
      ele.throwIfFailed();
    } else {
      // folly::Try can be empty, !hasValue() && !hasException() which marked as
      // the end of a stream
      co_return;
    }

    counter++;
    // check if needed to request more
    if (counter > sharedState->size_ / 2) {
      stream.executor_->add([sharedState, counter] {
        if (!sharedState->terminated_) {
          sharedState->subscription_->request(counter);
        }
      });
      counter = 0;
    }

    assert(dynamic_cast<detail::Value<T>*>(ele.value().get()));
    auto* valuePtr = static_cast<detail::Value<T>*>(ele.value().get());
    co_yield std::move(valuePtr->value);
  }
}
#endif

} // namespace thrift
} // namespace apache
