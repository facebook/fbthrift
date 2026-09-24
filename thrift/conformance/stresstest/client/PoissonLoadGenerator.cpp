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

#include <thrift/conformance/stresstest/client/PoissonLoadGenerator.h>

#include <algorithm>

#include <folly/portability/Asm.h>

namespace apache::thrift::stress {

namespace {
// Shortest gap we are willing to sleep through. Longer gaps are slept,
// shorter ones are spun.
//
// sleep_until wakes up late: measured on a devserver, 55 us at the median and
// 83 us at p99, almost regardless of the duration asked for. Most of that is a
// fixed offset, and a fixed offset cancels between one arrival and the next,
// so a gap is perturbed only by the spread around it.
//
// Replaying this loop at several thresholds gives the table below. "clumped"
// is the share of gaps that came out under 10 us, and "ideal" is what a true
// Poisson process produces at that rate. The difference between those two
// columns is clumping the generator invented.
//
//   qps/thread  threshold    spin   clumped   ideal
//          500       0 us    0.0%      2.9%    0.5%
//          500     150 us    0.3%      3.0%    0.5%
//          500     250 us    0.8%      3.0%    0.5%
//         4000     150 us   11.5%     14.1%    3.9%
//         4000     250 us   25.4%     11.1%    3.9%
//        10000     150 us   40.1%     19.8%    9.5%
//        10000     250 us   67.3%     14.0%    9.5%
//
// The suites run at a few hundred qps per thread, and there the threshold
// changes nothing and only costs cpu, so it is set low. Above a few thousand
// qps per thread no threshold gets near ideal without burning most of a core,
// and the answer there is more client threads rather than more spinning.
constexpr std::chrono::microseconds kMinSleepGap{150};

// Longest a single sleep may run. Bounds how long the destructor waits for
// the timing thread to notice that it should stop.
constexpr std::chrono::milliseconds kMaxSleepSlice{20};
} // namespace

PoissonLoadGenerator::PoissonLoadGenerator(double targetQps)
    : targetQps_(targetQps) {}

folly::coro::AsyncGenerator<PoissonLoadGenerator::Count>
PoissonLoadGenerator::getRequestCount() {
  while (running_) {
    auto request = co_await queue_.dequeue();
    co_yield request;
  }
}

void PoissonLoadGenerator::generateRequestSignals() {
  // Callers build a generator whether or not load was asked for, so a rate of
  // zero has to mean no requests rather than an error.
  if (targetQps_ <= 0.0) {
    return;
  }
  std::exponential_distribution<double> gaps(targetQps_);

  // Arrival times accumulate rather than restart from now(), so a late wakeup
  // is paid back on the following gaps and the long-run rate still holds.
  std::chrono::steady_clock::time_point nextArrival =
      std::chrono::steady_clock::now();
  while (running_.load(std::memory_order_relaxed)) {
    const std::chrono::duration<double> gap(gaps(gen_));
    nextArrival +=
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(gap);

    // Sleeping is done in slices. sleep_until cannot be woken, so a single
    // sleep over a long gap would hold up the destructor for that whole gap;
    // slices cap teardown at kMaxSleepSlice instead.
    while (running_.load(std::memory_order_relaxed)) {
      const std::chrono::steady_clock::time_point now =
          std::chrono::steady_clock::now();
      if (now + kMinSleepGap >= nextArrival) {
        break;
      }
      // @lint-ignore CLANGTIDY facebook-hte-BadCall-sleep_until
      // The sleep is what paces the load. This is a generator, not a test
      // waiting on someone else's work, so there is no event to wait on.
      std::this_thread::sleep_until(
          std::min(nextArrival, now + kMaxSleepSlice));
    }

    // The last stretch is too short to sleep on: the wakeup lands tens of
    // microseconds late, which over a gap this short is most of the gap, and
    // the arrival would miss its slot. Burning the time keeps it on time.
    while (running_.load(std::memory_order_relaxed) &&
           std::chrono::steady_clock::now() < nextArrival) {
      // Pause rather than yield. Yielding hands the core away and getting it
      // back can cost more than the gap we are waiting out, which defeats the
      // point of spinning.
      folly::asm_volatile_pause();
    }

    queue_.enqueue(1);
  }
}

void PoissonLoadGenerator::start() {
  bool f = false;
  if (started_.compare_exchange_strong(f, true)) {
    thread_ = std::thread([this] { generateRequestSignals(); });
  }
}

PoissonLoadGenerator::~PoissonLoadGenerator() {
  running_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
}

} // namespace apache::thrift::stress
