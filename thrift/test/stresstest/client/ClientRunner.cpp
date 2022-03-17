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
#include <thrift/test/stresstest/util/Util.h>

namespace apache {
namespace thrift {
namespace stress {

void ClientThreadMemoryStats::combine(const ClientThreadMemoryStats& other) {
  threadStart += other.threadStart;
  connectionsEstablished += other.connectionsEstablished;
  p50 += other.p50;
  p99 += other.p99;
  p100 += other.p100;
  connectionsIdle += other.connectionsIdle;
}

class ClientThread : public folly::HHWheelTimer::Callback {
 public:
  ClientThread(
      const ClientConfig& cfg,
      const folly::SocketAddress& addr,
      folly::CancellationSource& cancelSource)
      : memoryHistogram_(50, 0, 1024 * 1024 * 1024 /* 1GB */),
        cancelSource_(cancelSource) {
    auto* evb = thread_.getEventBase();
    // create clients in event base thread
    evb->runInEventBaseThreadAndWait([&]() {
      // capture baseline memory usage
      memoryStats_.threadStart = getThreadMemoryUsage();
      for (size_t i = 0; i < cfg.numClientsPerThread; i++) {
        clients_.emplace_back(std::make_unique<StressTestClient>(
            ClientFactory::createClient(addr, evb, cfg.connConfig), rpcStats_));
      }
      // capture memory usage after connections are established
      memoryStats_.connectionsEstablished = getThreadMemoryUsage();
      // start memory monitoring
      evb->timer().scheduleTimeout(this, std::chrono::seconds(1));
    });
  }

  ~ClientThread() {
    // destroy clients in event base thread
    thread_.getEventBase()->runInEventBaseThreadAndWait(
        [clients = std::move(clients_)]() {});
  }

  void run(const StressTestBase* test) {
    for (auto& client : clients_) {
      scope_.add(co_withCancellation(
          cancelSource_.getToken(),
          runInternal(client.get(), test).scheduleOn(thread_.getEventBase())));
    }
  }

  void stop() {
    folly::coro::blocking_wait(
        scope_.joinAsync().scheduleOn(thread_.getEventBase()));
    thread_.getEventBase()->runInEventBaseThreadAndWait([&]() {
      // cancel memory monitoring timeout and capture residual memory usage
      cancelTimeout();
      memoryStats_.connectionsIdle = getThreadMemoryUsage();
      // record memory usage percentiles collected during test
      memoryStats_.p50 = memoryHistogram_.getPercentileEstimate(.5);
      memoryStats_.p99 = memoryHistogram_.getPercentileEstimate(.99);
      memoryStats_.p100 = memoryHistogram_.getPercentileEstimate(1.0);
    });
  }

  // HHWheelTimer callback interface
  void timeoutExpired() noexcept override {
    memoryHistogram_.addValue(getThreadMemoryUsage());
    // reschedule the timeout
    thread_.getEventBase()->timer().scheduleTimeout(
        this, std::chrono::seconds(1));
  }

  const ClientRpcStats& getRpcStats() const { return rpcStats_; }
  const ClientThreadMemoryStats& getMemoryStats() const { return memoryStats_; }

 private:
  folly::coro::Task<void> runInternal(
      StressTestClient* client, const StressTestBase* test) {
    while (!(co_await folly::coro::co_current_cancellation_token)
                .isCancellationRequested() &&
           client->connectionGood()) {
      co_await test->runWorkload(client);
    }
  }

  ClientRpcStats rpcStats_;
  ClientThreadMemoryStats memoryStats_;
  folly::Histogram<size_t> memoryHistogram_;
  folly::coro::AsyncScope scope_;
  std::vector<std::unique_ptr<StressTestClient>> clients_;
  folly::CancellationSource& cancelSource_;
  folly::ScopedEventBaseThread thread_;
};

ClientRunner::ClientRunner(
    const ClientConfig& config, const folly::SocketAddress& addr)
    : clientThreads_() {
  for (size_t i = 0; i < config.numClientThreads; i++) {
    clientThreads_.emplace_back(
        std::make_unique<ClientThread>(config, addr, cancelSource_));
  }
}

ClientRunner::~ClientRunner() {
  // need destructor here so that ClientThread definition is available
}

void ClientRunner::run(const StressTestBase* test) {
  CHECK(!started_) << "ClientRunner was already started";
  for (auto& clientThread : clientThreads_) {
    clientThread->run(test);
  }
  started_ = true;
}

void ClientRunner::stop() {
  CHECK(started_ && !stopped_)
      << "ClientRunner was not started or is already stopped";
  cancelSource_.requestCancellation();
  for (auto& clientThread : clientThreads_) {
    clientThread->stop();
  }
  stopped_ = true;
}

ClientRpcStats ClientRunner::getRpcStats() const {
  CHECK(stopped_) << "ClientRunner must be stopped before accessing statistics";
  ClientRpcStats combinedStats;
  for (auto& clientThread : clientThreads_) {
    combinedStats.combine(clientThread->getRpcStats());
  }
  return combinedStats;
}

ClientThreadMemoryStats ClientRunner::getMemoryStats() const {
  CHECK(stopped_) << "ClientRunner must be stopped before accessing statistics";
  ClientThreadMemoryStats combinedStats;
  for (auto& clientThread : clientThreads_) {
    combinedStats.combine(clientThread->getMemoryStats());
  }
  return combinedStats;
}

} // namespace stress
} // namespace thrift
} // namespace apache
