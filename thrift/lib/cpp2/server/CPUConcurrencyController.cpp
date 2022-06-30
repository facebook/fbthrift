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

#include <thrift/lib/cpp2/server/CPUConcurrencyController.h>

#include <algorithm>

namespace apache::thrift {

namespace detail {
THRIFT_PLUGGABLE_FUNC_REGISTER(
    folly::observer::Observer<CPUConcurrencyController::Config>,
    makeCPUConcurrencyControllerConfig) {
  return folly::observer::makeStaticObserver(
      CPUConcurrencyController::Config{});
}

THRIFT_PLUGGABLE_FUNC_REGISTER(
    int64_t, getCPULoadCounter, std::chrono::milliseconds, bool) {
  return -1;
}
} // namespace detail

void CPUConcurrencyController::cycleOnce() {
  if (config().mode == Mode::DISABLED) {
    return;
  }

  auto load = getLoad();
  if (load >= config().cpuTarget) {
    lastOverloadStart_ = std::chrono::steady_clock::now();
    auto lim = serverConfigs_.getMaxRequests();
    auto newLim =
        lim -
        std::max<int64_t>(
            static_cast<int64_t>(lim * config().decreaseMultiplier), 1);
    serverConfigs_.setMaxRequests(
        std::max<int64_t>(newLim, config().concurrencyLowerBound));
  } else {
    // TODO: We should exclude fb303 methods.
    auto activeReq = serverConfigs_.getActiveRequests();
    if (activeReq <= 0 || load <= 0) {
      return;
    }

    // Estimate stable concurrency only if we haven't been overloaded recently
    // (and thus current concurrency/CPU is not a good indicator).
    //
    // Note: estimating concurrency is fairly lossy as it's a gauge
    // metric and we can't use techniques to measure it over a duration.
    // We may be able to get much better estimates if we switch to use QPS
    // and a token bucket for rate limiting.
    if (!isRefractoryPeriod() &&
        config().collectionSampleSize > stableConcurrencySamples_.size()) {
      auto concurrencyEstimate =
          static_cast<double>(activeReq) / load * config().cpuTarget;

      stableConcurrencySamples_.push_back(
          config().initialEstimateFactor * concurrencyEstimate);
      if (stableConcurrencySamples_.size() >= config().collectionSampleSize) {
        // Take percentile to hopefully account for inaccuracies. We don't
        // need a very accurate value as we will adjust this later in the
        // algorithm.
        //
        // The purpose of stable concurrency estimate is to optimistically
        // avoid causing too much request ingestion during initial overload, and
        // thus causing high tail latencies during initial overload.
        auto pct = stableConcurrencySamples_.begin() +
            static_cast<size_t>(
                       stableConcurrencySamples_.size() *
                       config().initialEstimatePercentile);
        std::nth_element(
            stableConcurrencySamples_.begin(),
            pct,
            stableConcurrencySamples_.end());
        stableEstimate_.store(*pct, std::memory_order_relaxed);
        serverConfigs_.setMaxRequests(*pct);
        return;
      }
    }

    auto lim = serverConfigs_.getMaxRequests();
    if (activeReq >= (1.0 - config().increaseDistanceRatio) * lim) {
      auto newLim =
          lim +
          std::max<int64_t>(
              static_cast<int64_t>(lim * config().additiveMultiplier), 1);
      serverConfigs_.setMaxRequests(
          std::min<int64_t>(config().concurrencyUpperBound, newLim));
    }
  }
}

void CPUConcurrencyController::schedule() {
  using time_point = std::chrono::steady_clock::time_point;
  if (config().mode == Mode::DISABLED) {
    return;
  }

  LOG(INFO) << "Enabling CPUConcurrencyController. CPU Target: "
            << this->config().cpuTarget
            << " Refresh Period Ms: " << this->config().refreshPeriodMs.count();
  scheduler_.addFunctionGenericNextRunTimeFunctor(
      [this] { this->cycleOnce(); },
      [this](time_point, time_point now) {
        return now + this->config().refreshPeriodMs;
      },
      "cpu-shed-loop",
      "cpu-shed-interval",
      this->config().refreshPeriodMs);
}

void CPUConcurrencyController::cancel() {
  scheduler_.cancelAllFunctionsAndWait();
}
} // namespace apache::thrift
