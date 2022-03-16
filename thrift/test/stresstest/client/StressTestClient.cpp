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

#include <thrift/test/stresstest/client/StressTestClient.h>

namespace apache {
namespace thrift {
namespace stress {

namespace {
constexpr double kHistogramMax = 1000.0 * 60.0; // 1 minute
} // namespace

ClientRpcStats::ClientRpcStats() : latencyHistogram(50, 0.0, kHistogramMax) {}

void ClientRpcStats::combine(const ClientRpcStats& other) {
  latencyHistogram.merge(other.latencyHistogram);
  numSuccess += other.numSuccess;
  numFailure += other.numFailure;
}

folly::coro::Task<void> StressTestClient::co_ping() {
  co_await timedExecute([&]() { return client_->co_ping(); });
}

folly::coro::Task<void> StressTestClient::co_requestResponseEb(
    const BasicRequest& req) {
  co_await timedExecute([&]() { return client_->co_requestResponseEb(req); });
}

folly::coro::Task<void> StressTestClient::co_requestResponseTm(
    const BasicRequest& req) {
  co_await timedExecute([&]() { return client_->co_requestResponseTm(req); });
}

template <class Fn>
folly::coro::Task<void> StressTestClient::timedExecute(Fn&& fn) {
  if (!connectionGood_) {
    co_return;
  }
  auto start = std::chrono::steady_clock::now();
  try {
    co_await fn();
  } catch (folly::OperationCancelled&) {
    // cancelled requests do not count as failures
    throw;
  } catch (transport::TTransportException& e) {
    // assume fatal issue with connection, stop using this client
    // TODO: Improve handling of connection issues
    LOG(ERROR) << e.what();
    connectionGood_ = false;
    co_return;
  } catch (std::exception& e) {
    LOG(WARNING) << "Request failed: " << e.what();
    stats_.numFailure++;
    co_return;
  }
  auto elapsed = std::chrono::steady_clock::now() - start;
  stats_.latencyHistogram.addValue(
      std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
  stats_.numSuccess++;
}

} // namespace stress
} // namespace thrift
} // namespace apache
