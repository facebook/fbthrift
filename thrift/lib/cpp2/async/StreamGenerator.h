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
#include <type_traits>

#include <folly/Portability.h>

#include <thrift/lib/cpp2/async/Stream.h>

#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#endif

namespace folly {
class CancellationToken;
}

namespace apache {
namespace thrift {

namespace detail {

template <typename>
struct is_optional : std::false_type {};
template <typename T>
struct is_optional<folly::Optional<T>> : std::true_type {};

template <typename>
struct is_future_optional : std::false_type {};
template <typename T>
struct is_future_optional<folly::SemiFuture<folly::Optional<T>>>
    : std::true_type {};
} // namespace detail

class StreamGenerator {
 public:
  template <
      typename Generator,
      std::enable_if_t<
          detail::is_optional<
              typename folly::invoke_result_t<std::decay_t<Generator>&>>::value,
          int> = 0>
  static auto create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      Generator&& generator)
      -> Stream<typename folly::invoke_result_t<
          std::decay_t<Generator>&>::value_type>;

  template <
      typename Generator,
      std::enable_if_t<
          detail::is_future_optional<
              typename folly::invoke_result_t<std::decay_t<Generator>&>>::value,
          int> = 0>
  static auto create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      Generator&& generator,
      int maxBatchSize = StreamGenerator::kDefaultBatchSize)
      -> Stream<typename folly::invoke_result_t<
          std::decay_t<Generator>&>::value_type::value_type>;
#ifdef FOLLY_HAS_COROUTINES
  template <
      typename Generator,
      std::enable_if_t<
          folly::coro::detail::is_async_generator_v<
              typename folly::invoke_result_t<std::decay_t<Generator>&>>,
          int> = 0>
  static auto create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      Generator&& generator)
      -> Stream<typename folly::invoke_result_t<
          std::decay_t<Generator>&>::value_type>;

  template <
      typename Generator,
      std::enable_if_t<
          folly::coro::detail::is_async_generator_v<
              typename folly::invoke_result_t<
                  std::decay_t<Generator>&,
                  folly::CancellationToken>>,
          int> = 0>
  static auto create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      Generator&& generator)
      -> Stream<typename folly::invoke_result_t<
          std::decay_t<Generator>&,
          folly::CancellationToken>::value_type>;
#endif

  static constexpr auto kDefaultBatchSize = 100;

 private:
  StreamGenerator() = default;
};

} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/async/StreamGenerator-inl.h>
