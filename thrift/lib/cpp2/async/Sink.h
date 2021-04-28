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

#include <chrono>
#include <memory>

#include <folly/Portability.h>

#include <folly/Try.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/Task.h>
#endif

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/async/ClientSinkBridge.h>
#include <thrift/lib/cpp2/async/ServerSinkBridge.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>

namespace apache {
namespace thrift {

class SinkThrew : public TApplicationException {};

template <typename T, typename R>
class ClientSink {
#if FOLLY_HAS_COROUTINES
  using PayloadSerializer = folly::Try<StreamPayload> (*)(folly::Try<T>&&);
  using FinalResponseDeserializer =
      folly::Try<R> (*)(folly::Try<StreamPayload>&&);

 public:
  ClientSink() = default;

  ClientSink(
      apache::thrift::detail::ClientSinkBridge::Ptr impl,
      PayloadSerializer serializer,
      FinalResponseDeserializer deserializer)
      : impl_(std::move(impl)),
        serializer_(std::move(serializer)),
        deserializer_(std::move(deserializer)) {}

  ~ClientSink() { cancel(); }

  ClientSink(const ClientSink&) = delete;
  ClientSink& operator=(const ClientSink&) = delete;

  ClientSink(ClientSink&&) = default;
  ClientSink& operator=(ClientSink&& sink) noexcept {
    cancel();
    impl_ = std::move(sink.impl_);
    serializer_ = std::move(sink.serializer_);
    deserializer_ = std::move(sink.deserializer_);
    return *this;
  }

  folly::coro::Task<R> sink(folly::coro::AsyncGenerator<T&&> generator) {
    bool sinkThrew = false;
    auto finalResponse =
        co_await std::exchange(impl_, nullptr)
            ->sink(
                [this, &sinkThrew](auto _generator)
                    -> folly::coro::AsyncGenerator<
                        folly::Try<StreamPayload>&&> {
                  while (true) {
                    auto item =
                        co_await folly::coro::co_awaitTry(_generator.next());
                    if (!item.hasException() && !item.hasValue()) {
                      break;
                    }
                    sinkThrew = item.hasException();
                    co_yield serializer_(std::move(item));
                  }
                }(std::move(generator)));

    if (sinkThrew) {
      throw SinkThrew();
    }

    // may throw
    co_return deserializer_(std::move(finalResponse)).value();
  }

 private:
  void cancel() {
    if (impl_) {
      impl_->cancel(serializer_(
          folly::Try<T>(folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::TApplicationExceptionType::INTERRUPTION,
              "never called sink object"))));
    }
  }

  apache::thrift::detail::ClientSinkBridge::Ptr impl_;
  PayloadSerializer serializer_;
  FinalResponseDeserializer deserializer_;
#endif
};

struct SinkOptions {
  std::chrono::milliseconds chunkTimeout;
};

template <typename SinkElement, typename FinalResponse>
struct SinkConsumer {
#if FOLLY_HAS_COROUTINES
  using Consumer = folly::Function<folly::coro::Task<FinalResponse>(
      folly::coro::AsyncGenerator<SinkElement&&>)>;
  Consumer consumer;
  uint64_t bufferSize;
  SinkOptions sinkOptions{std::chrono::milliseconds(0)};
  SinkConsumer&& setChunkTimeout(const std::chrono::milliseconds& timeout) && {
    sinkOptions.chunkTimeout = timeout;
    return std::move(*this);
  }
  SinkConsumer& setChunkTimeout(const std::chrono::milliseconds& timeout) & {
    sinkOptions.chunkTimeout = timeout;
    return *this;
  }
#endif
};

template <typename Response, typename SinkElement, typename FinalResponse>
class ResponseAndClientSink {
 public:
  using ResponseType = Response;
  using SinkElementType = SinkElement;
  using FinalResponseType = FinalResponse;

  Response response;
  ClientSink<SinkElement, FinalResponse> sink;
};

template <typename Response, typename SinkElement, typename FinalResponse>
class ResponseAndSinkConsumer {
 public:
  using ResponseType = Response;
  using SinkElementType = SinkElement;
  using FinalResponseType = FinalResponse;

  Response response;
  SinkConsumer<SinkElement, FinalResponse> sinkConsumer;
};

} // namespace thrift
} // namespace apache
