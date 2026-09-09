/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <exception>
#include <utility>

#include <folly/Executor.h>
#include <folly/Portability.h>

#if FOLLY_HAS_COROUTINES
#include <folly/coro/Task.h>
#endif

#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/FastHandlerCallback.h>

namespace apache::thrift::fast_thrift::thrift::detail {

#if FOLLY_HAS_COROUTINES

/**
 * Runs a co_<name> handler body and completes the callback with its outcome.
 *
 * Kept out of FastHandlerCallback.h so that only the generated sources that
 * actually dispatch a coroutine pull in <folly/coro/Task.h>.
 *
 * Entered on the callback's handler executor by the generated dispatcher.
 * Starting inline avoids a second scheduling hop; TaskWithExecutor guarantees
 * that the completion continuation resumes on that bound executor. Servers
 * without a CPU pool bind to the connection's EventBase instead.
 */
template <typename T>
void fastRunCoro(
    FastHandlerCallbackPtr<T> callback, folly::coro::Task<T> task) {
  auto* executor = callback->getHandlerExecutor();
  folly::coro::co_withExecutor(
      executor != nullptr
          ? folly::Executor::KeepAlive<>(executor)
          : folly::Executor::KeepAlive<>(callback->getEventBase()),
      std::move(task))
      .startInlineUnsafe(
          [cb = std::move(callback), executor](auto&& result) mutable noexcept {
            HandlerExecutorScope scope(executor);
            if (result.hasException()) {
              cb->exception(std::move(result.exception()));
              return;
            }
            try {
              cb->result(std::move(result.value()));
            } catch (...) {
              cb->exception(folly::exception_wrapper(std::current_exception()));
            }
          });
}

inline void fastRunCoro(
    FastHandlerCallbackPtr<void> callback, folly::coro::Task<void> task) {
  auto* executor = callback->getHandlerExecutor();
  folly::coro::co_withExecutor(
      executor != nullptr
          ? folly::Executor::KeepAlive<>(executor)
          : folly::Executor::KeepAlive<>(callback->getEventBase()),
      std::move(task))
      .startInlineUnsafe(
          [cb = std::move(callback), executor](auto&& result) mutable noexcept {
            HandlerExecutorScope scope(executor);
            if (result.hasException()) {
              cb->exception(std::move(result.exception()));
              return;
            }
            try {
              cb->done();
            } catch (...) {
              cb->exception(folly::exception_wrapper(std::current_exception()));
            }
          });
}

#endif // FOLLY_HAS_COROUTINES

} // namespace apache::thrift::fast_thrift::thrift::detail
