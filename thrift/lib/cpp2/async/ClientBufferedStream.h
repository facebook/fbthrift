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

#include <folly/Portability.h>
#include <folly/futures/Future.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp2/async/ClientStreamBridge.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/Baton.h>
#endif // FOLLY_HAS_COROUTINES

namespace yarpl {
namespace flowable {
class ThriftStreamShim;
}
} // namespace yarpl
namespace apache {
namespace thrift {

template <typename T>
class ClientBufferedStream {
 public:
  ClientBufferedStream() {}
  ClientBufferedStream(
      detail::ClientStreamBridge::ClientPtr streamBridge,
      folly::Try<T> (*decode)(folly::Try<StreamPayload>&&),
      int32_t bufferSize)
      : streamBridge_(std::move(streamBridge)),
        decode_(decode),
        bufferSize_(bufferSize) {}

  template <typename OnNextTry>
  void subscribeInline(OnNextTry&& onNextTry) && {
    auto streamBridge = std::move(streamBridge_);

    int32_t outstanding = bufferSize_;
    int32_t payloadDataSize = 0;

    apache::thrift::detail::ClientStreamBridge::ClientQueue queue;
    class ReadyCallback : public apache::thrift::detail::ClientStreamConsumer {
     public:
      void consume() override {
        baton.post();
      }

      void canceled() override {
        std::terminate();
      }

      void wait() {
        baton.wait();
      }

     private:
      folly::Baton<> baton;
    };

    while (true) {
      if (queue.empty()) {
        if (outstanding == 0) {
          streamBridge->requestN(1);
          ++outstanding;
        }

        ReadyCallback callback;
        if (streamBridge->wait(&callback)) {
          callback.wait();
        }
        queue = streamBridge->getMessages();
      }

      {
        auto& payload = queue.front();
        if (!payload.hasValue() && !payload.hasException()) {
          onNextTry(folly::Try<T>());
          break;
        }
        if (payload.hasValue()) {
          payloadDataSize += payload->payload->computeChainDataLength();
        }
        auto value = decode_(std::move(payload));
        queue.pop();
        const auto hasException = value.hasException();
        onNextTry(std::move(value));
        if (hasException) {
          break;
        }
      }

      outstanding--;
      // we request credits only if bufferSize_ > 0.
      // For bufferSize_ = 0, the loop requests 1 credit at a time.
      if ((outstanding <= bufferSize_ / 2) ||
          (payloadDataSize >= kRequestCreditPayloadSize)) {
        streamBridge->requestN(bufferSize_ - outstanding);
        outstanding = bufferSize_;
        payloadDataSize = 0;
      }
    }
  }

#if FOLLY_HAS_COROUTINES
  folly::coro::AsyncGenerator<T&&> toAsyncGenerator() && {
    return toAsyncGeneratorImpl(std::move(streamBridge_), bufferSize_, decode_);
  }
#endif // FOLLY_HAS_COROUTINES

  template <typename Callback>
  auto subscribeExTry(folly::Executor::KeepAlive<> e, Callback&& onNextTry) && {
    auto c = new Continuation<std::decay_t<Callback>>(
        e,
        std::forward<Callback>(onNextTry),
        std::move(streamBridge_),
        decode_,
        bufferSize_);
    Subscription sub(c->state_);
    e->add([c]() { (*c)(); });
    return sub;
  }

 private:
#if FOLLY_HAS_COROUTINES
  static folly::coro::AsyncGenerator<T&&> toAsyncGeneratorImpl(
      detail::ClientStreamBridge::ClientPtr streamBridge,
      int32_t bufferSize,
      folly::Try<T> (*decode)(folly::Try<StreamPayload>&&)) {
    int32_t outstanding = bufferSize;
    int32_t payloadDataSize = 0;

    apache::thrift::detail::ClientStreamBridge::ClientQueue queue;
    class ReadyCallback : public apache::thrift::detail::ClientStreamConsumer {
     public:
      void consume() override {
        baton.post();
      }

      void canceled() override {
        baton.post();
      }

      folly::coro::Baton baton;
    };

    int8_t iterationsSinceLastReschedule = 0;
    constexpr int8_t rescheduleEvery = 64;

    while (true) {
      if ((co_await folly::coro::co_current_cancellation_token)
              .isCancellationRequested()) {
        throw folly::OperationCancelled();
      }
      if (queue.empty()) {
        if (outstanding == 0) {
          streamBridge->requestN(1);
          ++outstanding;
        }

        ReadyCallback callback;
        if (streamBridge->wait(&callback)) {
          folly::CancellationCallback cb{
              co_await folly::coro::co_current_cancellation_token,
              [&] { streamBridge->cancel(); }};
          co_await callback.baton;
        }
        queue = streamBridge->getMessages();
        if (queue.empty()) {
          // we've been cancelled
          detail::ClientStreamBridge::Ptr(streamBridge.release());
          throw folly::OperationCancelled();
        }
      }

      // Coroutine TS symmetric transfer is broken in some build modes. Avoid
      // stack overflow by periodically re-scheduling a task onto current
      // executor.
      if (++iterationsSinceLastReschedule == rescheduleEvery) {
        iterationsSinceLastReschedule = 0;
        co_await folly::coro::co_reschedule_on_current_executor;
      }

      {
        auto& payload = queue.front();
        if (!payload.hasValue() && !payload.hasException()) {
          break;
        }
        if (payload.hasValue()) {
          payloadDataSize += payload->payload->computeChainDataLength();
        }
        auto value = decode(std::move(payload));
        queue.pop();
        // yield value or rethrow exception
        co_yield std::move(value).value();
      }

      outstanding--;
      // we request credits only if bufferSize_ > 0.
      // For bufferSize_ = 0, the loop requests 1 credit at a time.
      if ((outstanding <= bufferSize / 2) ||
          (payloadDataSize >= kRequestCreditPayloadSize)) {
        streamBridge->requestN(bufferSize - outstanding);
        outstanding = bufferSize;
        payloadDataSize = 0;
      }
    }
  }
#endif // FOLLY_HAS_COROUTINES

  struct SharedState {
    explicit SharedState(detail::ClientStreamBridge::ClientPtr sb)
        : streamBridge(sb.release()) {}
    folly::Promise<folly::Unit> promise;
    detail::ClientStreamBridge::Ptr streamBridge;
  };

 public:
  class Subscription {
    explicit Subscription(std::shared_ptr<SharedState> state)
        : state_(std::move(state)) {}

   public:
    Subscription(Subscription&& s) = default;
    Subscription& operator=(Subscription&& s) {
      if (std::exchange(state_, std::move(s.state_))) {
        LOG(FATAL) << "Subscription has to be joined/detached";
      }
      return *this;
    }
    ~Subscription() {
      if (state_) {
        LOG(FATAL) << "Subscription has to be joined/detached";
      }
    }

    void cancel() {
      state_->streamBridge->cancel();
    }

    void detach() && {
      state_.reset();
    }

    void join() && {
      std::move(*this).futureJoin().wait();
    }

    folly::SemiFuture<folly::Unit> futureJoin() && {
      return std::exchange(state_, nullptr)->promise.getSemiFuture();
    }

   private:
    std::shared_ptr<SharedState> state_;
    friend class ClientBufferedStream;
  };

 private:
  template <typename OnNextTry>
  // Ownership model: caller owns until wait returns true.
  // Argument is released ("leaked") when wait() succeeds. It is transferred
  // to the new execution frame in consume(). If wait() returns false its
  // argument is not released and the caller frees it as normal. If wait() is
  // interrupted by cancel() the memory is freed in canceled()
  class Continuation : public apache::thrift::detail::ClientStreamConsumer {
   public:
    Continuation(
        folly::Executor::KeepAlive<> e,
        OnNextTry onNextTry,
        detail::ClientStreamBridge::ClientPtr streamBridge,
        folly::Try<T> (*decode)(folly::Try<StreamPayload>&&),
        int32_t bufferSize)
        : e_(e),
          onNextTry_(std::move(onNextTry)),
          decode_(decode),
          bufferSize_(bufferSize),
          state_(std::make_shared<SharedState>(std::move(streamBridge))) {
      outstanding_ = bufferSize_;
    }

    ~Continuation() {
      state_->promise.setValue();
    }

    // takes ownerhsip of pointer on success
    static bool wait(std::unique_ptr<Continuation>& cb) {
      bool ret = cb->state_->streamBridge->wait(cb.get());
      if (ret) {
        cb.release();
      }
      return ret;
    }

    void consume() override {
      e_->add([this]() { (*this)(); });
    }

    void canceled() override {
      delete this;
    }

    void operator()() noexcept {
      std::unique_ptr<Continuation> cb(this);
      apache::thrift::detail::ClientStreamBridge::ClientQueue queue;

      while (!state_->streamBridge->isCanceled()) {
        if (queue.empty()) {
          if (outstanding_ == 0) {
            state_->streamBridge->requestN(1);
            ++outstanding_;
          }

          if (Continuation::wait(cb)) {
            // The filler will schedule us back on the executor once the queue
            // is refilled so we return here
            return;
          }
          // Otherwise messages are now available (or we've been cancelled)
          queue = state_->streamBridge->getMessages();
          if (queue.empty()) {
            // we've been cancelled
            return;
          }
        }

        {
          auto& payload = queue.front();
          if (!payload.hasValue() && !payload.hasException()) {
            onNextTry_(folly::Try<T>());
            return;
          }
          if (payload.hasValue()) {
            payloadDataSize_ += payload->payload->computeChainDataLength();
          }
          auto value = decode_(std::move(payload));
          queue.pop();
          const auto hasException = value.hasException();
          onNextTry_(std::move(value));
          if (hasException) {
            return;
          }
        }

        outstanding_--;
        // we request credits only if bufferSize_ > 0.
        // For bufferSize_ = 0, the loop requests 1 credit at a time.
        if ((outstanding_ <= bufferSize_ / 2) ||
            (payloadDataSize_ >= kRequestCreditPayloadSize)) {
          state_->streamBridge->requestN(bufferSize_ - outstanding_);
          outstanding_ = bufferSize_;
          payloadDataSize_ = 0;
        }
      }
    }

   private:
    folly::Executor::KeepAlive<> e_;
    OnNextTry onNextTry_;
    folly::Try<T> (*decode_)(folly::Try<StreamPayload>&&);
    int32_t bufferSize_;
    int32_t outstanding_;
    int32_t payloadDataSize_{0};
    std::shared_ptr<SharedState> state_;
    friend class ClientBufferedStream;
  };

  detail::ClientStreamBridge::ClientPtr streamBridge_;
  folly::Try<T> (*decode_)(folly::Try<StreamPayload>&&) = nullptr;
  int32_t bufferSize_{0};
  static constexpr int32_t kRequestCreditPayloadSize = 16384;

  friend class yarpl::flowable::ThriftStreamShim;
}; // namespace thrift

template <typename Response, typename StreamElement>
struct ResponseAndClientBufferedStream {
  using ResponseType = Response;
  using StreamElementType = StreamElement;

  Response response;
  ClientBufferedStream<StreamElement> stream;
};
} // namespace thrift
} // namespace apache
