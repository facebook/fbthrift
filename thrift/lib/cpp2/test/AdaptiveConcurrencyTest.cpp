/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <thrift/lib/cpp2/server/AdaptiveConcurrency.h>

#include <folly/portability/GTest.h>

#include <folly/experimental/observer/SimpleObservable.h>

using namespace std::literals::chrono_literals;
using namespace apache::thrift;

using Clock = std::chrono::steady_clock;

namespace apache::thrift {
class AdaptiveConcurrencyTestHelper {
 public:
  explicit AdaptiveConcurrencyTestHelper(AdaptiveConcurrencyController& c)
      : c_(c) {}
  auto samplingPeriodStart() const { return c_.samplingPeriodStart(); }
  auto samplingPeriodStart(Clock::time_point v) { c_.samplingPeriodStart(v); }
  auto rttRecalcStart() const { return c_.rttRecalcStart(); }
  auto nextRttRecalcStart() const { return c_.nextRttRecalcStart(); }
  auto nextRttRecalcStart(Clock::time_point t) { c_.nextRttRecalcStart(t); }

 private:
  apache::thrift::AdaptiveConcurrencyController& c_;
};
} // namespace apache::thrift

// helper to be call private methods on the controller
AdaptiveConcurrencyTestHelper p(AdaptiveConcurrencyController& c) {
  return AdaptiveConcurrencyTestHelper(c);
}

template <size_t MinConcurrency = 5>
class AdaptiveConcurrencyBase : public testing::Test {
 protected:
  static constexpr Clock::duration kIdealRtt = 10ms;
  static constexpr size_t minConcurrency = MinConcurrency;

  void setConfig(size_t concurrency, double jitter = 0.0) {
    oConfig.setValue(makeConfig(concurrency, jitter));
    folly::observer_detail::ObserverManager::waitForAllUpdates();
  }

  static auto makeConfig(size_t concurrency, double jitter = 0.0) {
    AdaptiveConcurrencyController::Config config;
    config.minConcurrency = concurrency;
    config.recalcPeriodJitter = jitter;
    return config;
  }
  folly::observer::SimpleObservable<AdaptiveConcurrencyController::Config>
      oConfig{makeConfig(MinConcurrency)};

  AdaptiveConcurrencyController controller{oConfig.getObserver()};

  void makeRequest(Clock::duration latency = kIdealRtt) {
    auto now = Clock::now();
    controller.requestStarted(now);
    controller.requestFinished(now, now + latency);
  }

  enum class EndState {
    SamplingScheduled,
    RecalcScheduled,
  };
  void doSamplingRequests(
      Clock::duration latency = kIdealRtt,
      EndState endState = EndState::SamplingScheduled) {
    // execute 200 samples to collect stats, new ideal latency is recomputed
    // at the end
    auto before = Clock::now();

    for (int i = 0; i < 200; i++) {
      EXPECT_GT(p(controller).samplingPeriodStart(), Clock::time_point{});
      EXPECT_LT(p(controller).samplingPeriodStart(), Clock::now());
      makeRequest(latency);
    }
    switch (endState) {
      case EndState::RecalcScheduled:
        EXPECT_EQ(
            p(controller).rttRecalcStart(), p(controller).nextRttRecalcStart());
        break;
      case EndState::SamplingScheduled:
        EXPECT_GT(p(controller).samplingPeriodStart(), before + 500ms);
        EXPECT_LT(p(controller).samplingPeriodStart(), Clock::now() + 500ms);
        break;
    }
  }

  void performSampling(
      Clock::duration latency = kIdealRtt,
      EndState endState = EndState::SamplingScheduled) {
    auto concurrency = controller.getMaxRequests();
    p(controller).samplingPeriodStart(Clock::now());
    doSamplingRequests(latency, endState);
    double slope = 1.0 * controller.targetRtt() / latency;
    size_t expectConcurrency = slope * concurrency + sqrt(slope * concurrency);
    expectConcurrency = std::min(expectConcurrency, 1000ul);
    expectConcurrency = std::max(minConcurrency, expectConcurrency);
    EXPECT_EQ(controller.getMaxRequests(), expectConcurrency);
    // targetRtt is computed to be 2x the observed p95 latency
    EXPECT_EQ(controller.targetRtt(), 2 * kIdealRtt);
  }
};

class AdaptiveConcurrency : public AdaptiveConcurrencyBase<5> {};
class AdaptiveConcurrencyDisabled : public AdaptiveConcurrencyBase<0> {};

TEST_F(AdaptiveConcurrency, IdealRttCalc) {
  // first request primes the system to start ideal rtt calibration
  EXPECT_EQ(p(controller).samplingPeriodStart(), Clock::time_point{});
  {
    auto now = Clock::now();
    makeRequest();
    EXPECT_GT(p(controller).samplingPeriodStart(), now);
  }

  // test latency computations w/ changing input
  p(controller).samplingPeriodStart(Clock::now());
  for (int i = 0; i < 200; i++) {
    makeRequest(std::chrono::milliseconds(i) + 100ms);
  }
  // target should now be twice the p95 of the sampled latencies
  EXPECT_EQ(controller.targetRtt(), 2 * 290ms);
}

TEST_F(AdaptiveConcurrency, Sampling) {
  // calibrate ideal rtt
  makeRequest();
  doSamplingRequests(kIdealRtt);

  // imitate low request latencies
  // concurrency limit grows until reaching max of 1000
  auto lowLatency = kIdealRtt;
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 13);
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 31);
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 69);
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 149);
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 315);
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 655);
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 1000);
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 1000);
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 1000);

  // imitate higher latencies
  // concurrency limit drops until reaching min of 5
  auto highLatency = 4 * kIdealRtt;
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 522);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 277);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 150);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 83);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 47);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 28);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 17);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 11);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 7);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 5);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 5);
  performSampling(highLatency);
  EXPECT_EQ(controller.getMaxRequests(), 5);

  // validate making requests before sampling periods does not affect
  // the sampling
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 13);
  for (int i = 0; i < 200; ++i) {
    makeRequest(highLatency);
  }
  performSampling(lowLatency);
  EXPECT_EQ(controller.getMaxRequests(), 31);
}

TEST_F(AdaptiveConcurrency, RttRecalibration) {
  // calibrate ideal rtt
  setConfig(5, 0.2);

  auto before = Clock::now();
  makeRequest();
  auto after = Clock::now();
  doSamplingRequests(kIdealRtt, EndState::SamplingScheduled);
  EXPECT_GT(p(controller).nextRttRecalcStart(), before + 4min);
  EXPECT_LT(p(controller).nextRttRecalcStart(), after + 6min);

  performSampling(kIdealRtt, EndState::SamplingScheduled);
  EXPECT_EQ(controller.getMaxRequests(), 13);

  // force entering calibration mode after the next sampling round
  // and perform sampling round
  p(controller).nextRttRecalcStart(Clock::now());
  performSampling(kIdealRtt, EndState::RecalcScheduled);

  // next request will cause concurrency to reset to the minimum
  // also resets next recalibration target time
  EXPECT_EQ(controller.getMaxRequests(), 31);
  before = Clock::now();
  EXPECT_LT(p(controller).nextRttRecalcStart(), before);
  makeRequest();
  EXPECT_GT(p(controller).nextRttRecalcStart(), before + 4min);
  EXPECT_LT(p(controller).nextRttRecalcStart(), Clock::now() + 6min);
  EXPECT_EQ(controller.getMaxRequests(), 5);

  // run recalibration, at the end of the calibration concurrency
  // should be return to pre-calibration value
  doSamplingRequests(kIdealRtt * 2);
  EXPECT_EQ(controller.getMaxRequests(), 31);

  // targetRtt is computed to be 2x the observed p95 latency
  EXPECT_EQ(controller.targetRtt(), 4 * kIdealRtt);
}

TEST_F(AdaptiveConcurrencyDisabled, Enabling) {
  makeRequest();
  //
  EXPECT_EQ(controller.getMaxRequests(), 0);
  EXPECT_EQ(p(controller).samplingPeriodStart(), Clock::time_point{});

  setConfig(5);

  // first request primes the system to start ideal rtt calibration
  EXPECT_EQ(p(controller).samplingPeriodStart(), Clock::time_point{});
  {
    auto now = Clock::now();
    makeRequest();
    EXPECT_GT(p(controller).samplingPeriodStart(), now);
  }
  EXPECT_EQ(controller.getMaxRequests(), 5);
}

TEST_F(AdaptiveConcurrency, Disabling) {
  makeRequest();

  // disable controller
  setConfig(0);
  EXPECT_EQ(controller.getMaxRequests(), 0);

  // re-enable controller
  setConfig(minConcurrency);
  EXPECT_EQ(controller.getMaxRequests(), 0);
  makeRequest();
  EXPECT_EQ(controller.getMaxRequests(), 5);

  doSamplingRequests(kIdealRtt, EndState::SamplingScheduled);

  // disable controller again
  setConfig(0);
  EXPECT_EQ(controller.getMaxRequests(), 0);
}
