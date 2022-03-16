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

#include <folly/experimental/coro/AsyncScope.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/test/stresstest/client/StressTestBase.h>
#include <thrift/test/stresstest/if/gen-cpp2/StressTest.h>

namespace apache {
namespace thrift {
namespace stress {

struct ClientConfig {
  uint64_t numClientThreads;
  uint64_t numClientsPerThread;
};

class ClientThread {
 public:
  ClientThread(
      size_t numClients,
      const folly::SocketAddress& addr,
      folly::CancellationSource& cancelSource);
  ~ClientThread();

  void run(const StressTestBase* test);
  void stop();

 private:
  folly::coro::Task<void> runInternal(
      StressTestClient* client, const StressTestBase* test);

  folly::ScopedEventBaseThread thread_;
  folly::coro::AsyncScope scope_;
  std::vector<std::unique_ptr<StressTestClient>> clients_;
  folly::CancellationSource& cancelSource_;
};

class ClientRunner {
 public:
  ClientRunner(const ClientConfig& config, const folly::SocketAddress& addr);

  void run(const StressTestBase* test);
  void stop();

 private:
  folly::CancellationSource cancelSource_;
  std::vector<std::unique_ptr<ClientThread>> clientThreads_;
};

} // namespace stress
} // namespace thrift
} // namespace apache
