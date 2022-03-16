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

#include <thrift/test/stresstest/client/ClientRunner.h>

#include <folly/experimental/coro/BlockingWait.h>
#include <thrift/test/stresstest/client/ClientFactory.h>

namespace apache {
namespace thrift {
namespace stress {

ClientThread::ClientThread(
    size_t numClients,
    const folly::SocketAddress& addr,
    folly::CancellationSource& cancelSource)
    : cancelSource_(cancelSource) {
  auto* evb = thread_.getEventBase();
  // create clients in event base thread
  evb->runInEventBaseThreadAndWait([&]() {
    for (size_t i = 0; i < numClients; i++) {
      clients_.emplace_back(std::make_unique<StressTestClient>(
          ClientFactory::createClient(addr, evb)));
    }
  });
}

ClientThread::~ClientThread() {
  // destroy clients in event base thread
  thread_.getEventBase()->runInEventBaseThreadAndWait(
      [clients = std::move(clients_)]() {});
}

void ClientThread::run(const StressTestBase* test) {
  for (auto& client : clients_) {
    scope_.add(co_withCancellation(
        cancelSource_.getToken(),
        runInternal(client.get(), test).scheduleOn(thread_.getEventBase())));
  }
}

void ClientThread::stop() {
  folly::coro::blocking_wait(
      scope_.joinAsync().scheduleOn(thread_.getEventBase()));
}

folly::coro::Task<void> ClientThread::runInternal(
    StressTestClient* client, const StressTestBase* test) {
  while (!(co_await folly::coro::co_current_cancellation_token)
              .isCancellationRequested()) {
    co_await test->runWorkload(client);
  }
}

ClientRunner::ClientRunner(
    const ClientConfig& config, const folly::SocketAddress& addr) {
  for (size_t i = 0; i < config.numClientThreads; i++) {
    clientThreads_.emplace_back(std::make_unique<ClientThread>(
        config.numClientsPerThread, addr, cancelSource_));
  }
}

void ClientRunner::run(const StressTestBase* test) {
  for (auto& clientThread : clientThreads_) {
    clientThread->run(test);
  }
}

void ClientRunner::stop() {
  cancelSource_.requestCancellation();
  for (auto& clientThread : clientThreads_) {
    clientThread->stop();
  }
}

} // namespace stress
} // namespace thrift
} // namespace apache
