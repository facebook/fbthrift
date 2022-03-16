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

#include <folly/experimental/coro/Task.h>
#include <thrift/test/stresstest/if/gen-cpp2/StressTest.h>

namespace apache {
namespace thrift {
namespace stress {

/**
 * Wrapper around the generated StressTestAsyncClient to transparently collect
 * statistics of requests being sent
 */
class StressTestClient {
 public:
  explicit StressTestClient(std::unique_ptr<StressTestAsyncClient> client)
      : client_(std::move(client)) {}

  folly::coro::Task<void> co_ping();

  folly::coro::Task<void> co_requestResponseEb(const BasicRequest& req);

  folly::coro::Task<void> co_requestResponseTm(const BasicRequest& req);

 private:
  std::unique_ptr<StressTestAsyncClient> client_;
};

} // namespace stress
} // namespace thrift
} // namespace apache
