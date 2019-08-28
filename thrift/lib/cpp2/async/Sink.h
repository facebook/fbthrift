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

#ifdef FOLLY_HAS_COROUTINES
#include <folly/Try.h>
#include <folly/executors/SequencedExecutor.h>
#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/Task.h>

#include <thrift/lib/cpp2/async/ClientSinkBridge.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>

namespace apache {
namespace thrift {

template <typename T, typename R>
class ClientSink {
  using PayloadSerializer =
      std::function<std::unique_ptr<folly::IOBuf>(folly::Try<T>&&)>;
  using FinalResponseDeserializer =
      folly::Try<R> (*)(folly::Try<StreamPayload>&&);

 public:
  ClientSink() = default;

  ClientSink(
      detail::ClientSinkBridge::Ptr impl,
      PayloadSerializer serializer,
      FinalResponseDeserializer deserializer)
      : impl_(std::move(impl)),
        serializer_(std::move(serializer)),
        deserializer_(std::move(deserializer)) {}

  folly::coro::Task<R> sink(folly::coro::AsyncGenerator<T&&> generator) {
    folly::exception_wrapper ew;
    auto finalResponse = co_await impl_->sink(
        [ this, &ew, generator = std::move(generator) ]() mutable
        -> folly::coro::AsyncGenerator<folly::Try<StreamPayload>&&> {
          try {
            while (auto item = co_await generator.next()) {
              co_yield folly::Try<StreamPayload>(StreamPayload(
                  serializer_(folly::Try<T>(std::move(*item))), {}));
            }
          } catch (std::exception& e) {
            ew = folly::exception_wrapper(std::current_exception(), e);
          } catch (...) {
            ew = folly::exception_wrapper(std::current_exception());
          }

          if (ew) {
            co_yield folly::Try<StreamPayload>(rocket::RocketException(
                rocket::ErrorCode::APPLICATION_ERROR,
                serializer_(folly::Try<T>(ew))));
          }
        }());

    if (ew) {
      ew.throw_exception();
    }

    // may throw
    co_return deserializer_(std::move(finalResponse)).value();
  }

 private:
  detail::ClientSinkBridge::Ptr impl_;
  PayloadSerializer serializer_;
  FinalResponseDeserializer deserializer_;
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

} // namespace thrift
} // namespace apache
#endif
