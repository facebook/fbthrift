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

#include <folly/Portability.h>
#include <thrift/lib/cpp2/async/ClientStreamBridge.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#endif // FOLLY_HAS_COROUTINES

namespace apache {
namespace thrift {

template <typename T>
class ClientBufferedStream {
 public:
  ClientBufferedStream() {}
  ClientBufferedStream(
      detail::ClientStreamBridge::Ptr streamBridge,
      folly::Try<T> (*decode)(folly::Try<StreamPayload>&&),
      int32_t bufferSize)
      : streamBridge_(std::move(streamBridge)),
        decode_(decode),
        bufferSize_(bufferSize) {}

  template <typename OnNextTry>
  void subscribeInline(OnNextTry&& onNextTry) && {
    auto streamBridge = std::move(streamBridge_);
    SCOPE_EXIT {
      streamBridge->cancel();
    };

    int32_t outstanding = bufferSize_;

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
      folly::fibers::Baton baton;
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
          break;
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
      if (outstanding <= bufferSize_ / 2) {
        streamBridge->requestN(bufferSize_ - outstanding);
        outstanding = bufferSize_;
      }
    }
  }

#if FOLLY_HAS_COROUTINES
  folly::coro::AsyncGenerator<T&&> toAsyncGenerator() && {
    return toAsyncGeneratorImpl(std::move(*this));
  }
#endif // FOLLY_HAS_COROUTINES

 private:
#if FOLLY_HAS_COROUTINES
  static folly::coro::AsyncGenerator<T&&> toAsyncGeneratorImpl(
      ClientBufferedStream me) {
    SCOPE_EXIT {
      me.streamBridge_->cancel();
    };

    int32_t bufferSize =
        me.bufferSize_ ? me.bufferSize_ : std::numeric_limits<int32_t>::max();
    me.streamBridge_->requestN(bufferSize);
    int32_t outstanding = bufferSize;

    apache::thrift::detail::ClientStreamBridge::ClientQueue queue;
    class ReadyCallback : public apache::thrift::detail::ClientStreamConsumer {
     public:
      void consume() override {
        baton.post();
      }

      void canceled() override {
        std::terminate();
      }

      folly::coro::Baton baton;
    };

    while (true) {
      if (queue.empty()) {
        ReadyCallback callback;
        if (me.streamBridge_->wait(&callback)) {
          co_await callback.baton;
        }
        queue = me.streamBridge_->getMessages();
      }

      {
        auto& payload = queue.front();
        if (!payload.hasValue() && !payload.hasException()) {
          break;
        }
        auto value = me.decode_(std::move(payload));
        queue.pop();
        // yield value or rethrow exception
        co_yield std::move(value).value();
      }

      outstanding--;
      if (outstanding <= bufferSize / 2) {
        me.streamBridge_->requestN(bufferSize - outstanding);
        outstanding = bufferSize;
      }
    }
  }
#endif // FOLLY_HAS_COROUTINES

  detail::ClientStreamBridge::Ptr streamBridge_;
  folly::Try<T> (*decode_)(folly::Try<StreamPayload>&&) = nullptr;
  int32_t bufferSize_{0};
};

template <typename Response, typename StreamElement>
struct ResponseAndClientBufferedStream {
  using ResponseType = Response;
  using StreamElementType = StreamElement;

  Response response;
  ClientBufferedStream<StreamElement> stream;
};
} // namespace thrift
} // namespace apache
