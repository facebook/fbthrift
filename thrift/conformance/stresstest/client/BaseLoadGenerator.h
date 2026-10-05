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

#include <chrono>

#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/Coroutine.h>

namespace apache::thrift::stress {

/**
 * Supplies request counts to stress-test runners.
 *
 * Detailed signals also carry a planned arrival time. Generators that only
 * implement getRequestCount() use the time when the runner consumes the count.
 */
class BaseLoadGenerator {
 public:
  virtual ~BaseLoadGenerator() = default;
  using Count = int32_t;

  struct RequestSignal {
    Count count;
    std::chrono::steady_clock::time_point plannedArrival;
  };

  virtual folly::coro::AsyncGenerator<Count> getRequestCount() = 0;
  virtual folly::coro::AsyncGenerator<RequestSignal> getRequestSignals() {
    folly::coro::AsyncGenerator<Count> counts = getRequestCount();
    while (const folly::coro::AsyncGenerator<Count>::NextResult count =
               co_await counts.next()) {
      co_yield RequestSignal{
          .count = *count,
          .plannedArrival = std::chrono::steady_clock::now(),
      };
    }
  }
  virtual void start() = 0;
  virtual void stop() = 0;
};

} // namespace apache::thrift::stress
