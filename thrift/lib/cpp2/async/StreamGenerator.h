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
#include <folly/Portability.h>

#include <thrift/lib/cpp2/async/Stream.h>

#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#endif

namespace apache {
namespace thrift {

namespace detail {
template <typename Generator, typename Element>
struct StreamGeneratorImpl {
  static Stream<Element> create(
      folly::Executor::KeepAlive<folly::SequencedExecutor>,
      Generator&&,
      int64_t) {
    LOG(FATAL) << "Returned value should either be folly::Optional<T>, "
                  "folly::SemiFuture<folly::Optional<T>> or"
                  " folly::coro::AsyncGenerator<T?(&|&&)>";
  }
};

template <typename Generator, typename Element>
struct StreamGeneratorImpl<Generator, folly::Optional<Element>> {
  static Stream<Element> create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      Generator&& generator,
      int64_t maxBatchSize);
};

template <typename Generator, typename Element>
struct StreamGeneratorImpl<
    Generator,
    folly::SemiFuture<folly::Optional<Element>>> {
  static Stream<Element> create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      Generator&& generator,
      int64_t maxBatchSize);
};

#if FOLLY_HAS_COROUTINES
template <typename Generator, typename Element>
struct StreamGeneratorImpl<Generator, folly::coro::AsyncGenerator<Element>> {
  static Stream<typename folly::coro::AsyncGenerator<Element>::value_type>
  create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      Generator&& generator,
      int64_t maxBatchSize);
};
#endif
} // namespace detail

class StreamGenerator {
 public:
  template <
      typename Generator,
      typename Element =
          typename folly::invoke_result_t<std::decay_t<Generator>&>>
  static auto create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      Generator&& generator,
      int64_t maxBatchSize = StreamGenerator::kDefaultBatchSize) {
    return detail::StreamGeneratorImpl<Generator, Element>::create(
        executor, std::forward<Generator>(generator), maxBatchSize);
  }
  static constexpr auto kDefaultBatchSize = 100;

 private:
  StreamGenerator() = default;
};

} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/async/StreamGenerator-inl.h>
