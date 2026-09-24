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

#include <atomic>
#include <chrono>
#include <cmath>
#include <random>
#include <thread>
#include <folly/coro/SmallUnboundedQueue.h>
#include <thrift/conformance/stresstest/client/BaseLoadGenerator.h>

namespace apache::thrift::stress {

/**
 * Offers load as a Poisson process: each signal is one request, and the gaps
 * between them are drawn from an exponential distribution.
 *
 * targetQps is fractional because callers split an aggregate rate across
 * client threads, so a single generator often runs well below one request per
 * millisecond.
 *
 * Timing runs on a thread of its own and the signals reach the caller through
 * a queue. It has to stay that way: the last stretch before each arrival is
 * spun rather than slept, and spinning on the caller's EventBase would stall
 * the requests being measured.
 */
class PoissonLoadGenerator : public BaseLoadGenerator {
 public:
  explicit PoissonLoadGenerator(double targetQps);

  ~PoissonLoadGenerator() override;

  folly::coro::AsyncGenerator<Count> getRequestCount() override;
  void start() override;

 private:
  const double targetQps_;
  std::atomic<bool> running_{true};
  std::atomic<bool> started_{false};
  folly::coro::SmallUnboundedQueue<Count> queue_;
  std::mt19937_64 gen_{std::random_device()()};
  std::thread thread_;

  void generateRequestSignals();
};

} // namespace apache::thrift::stress
