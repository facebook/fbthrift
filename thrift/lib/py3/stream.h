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

#if FOLLY_HAS_COROUTINES

#include <folly/python/async_generator.h>
#include <thrift/lib/cpp2/async/ClientBufferedStream.h>
#include <thrift/lib/cpp2/async/ServerStream.h>

namespace thrift {
namespace py3 {

template <typename T>
class ClientBufferedStreamWrapper {
 public:
  ClientBufferedStreamWrapper() = default;
  explicit ClientBufferedStreamWrapper(
      apache::thrift::ClientBufferedStream<T>& s)
      : gen_{std::move(s).toAsyncGenerator()} {}

  folly::coro::Task<folly::Optional<T>> getNext() {
    auto res = co_await gen_.getNext();
    if (res.has_value()) {
      co_return std::move(res.value());
    }
    co_return folly::none;
  }

 private:
  folly::python::AsyncGeneratorWrapper<T&&> gen_;
};

template <typename Response, typename StreamElement>
apache::thrift::ResponseAndServerStream<Response, StreamElement>
createEmptyResponseAndServerStream() {
  return {{}, apache::thrift::ServerStream<StreamElement>::createEmpty()};
}

} // namespace py3
} // namespace thrift

#endif
