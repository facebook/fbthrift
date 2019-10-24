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

#include <folly/python/async_generator.h>
#include <thrift/lib/cpp2/async/SemiStream.h>

namespace thrift {
namespace py3 {

template <typename T>
class SemiStreamWrapper {
 public:
  SemiStreamWrapper() = default;
  explicit SemiStreamWrapper(
      apache::thrift::SemiStream<T>& s,
      int32_t buffer_size)
      : gen_{apache::thrift::toAsyncGenerator<T>(std::move(s), buffer_size)} {}

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

template <typename R, typename S>
class ResponseAndSemiStreamWrapper {
 public:
  ResponseAndSemiStreamWrapper() = default;
  explicit ResponseAndSemiStreamWrapper(
      apache::thrift::ResponseAndSemiStream<R, S>& rs)
      : response_{std::move(rs.response)}, stream_{std::move(rs.stream)} {}

  std::shared_ptr<R> getResponse() {
    return std::make_shared<R>(std::move(response_));
  }

  apache::thrift::SemiStream<S> getStream() {
    return std::move(stream_);
  }

 private:
  R response_;
  apache::thrift::SemiStream<S> stream_;
}; // namespace py3

} // namespace py3
} // namespace thrift
