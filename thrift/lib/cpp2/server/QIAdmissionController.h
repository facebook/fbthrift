/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <mutex>
#include <thread>

#include <glog/logging.h>

#include <folly/stats/BucketedTimeSeries-defs.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/server/AdmissionController.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>

namespace apache {
namespace thrift {

/**
 * This admission controller uses the Q-Integral algorithm to shed load based on
 * the length of the queue of unprocessed messages.
 *
 * It is configured by three parameters (two of them have reasonable defaults)
 * - `processTimeout` indicates the avergae latency the server wants to stay
 *    below
 * - `window` indicates the sliding window on which the statistics are computed
 * - `minQueueLength` indicates the minimum size of the queue, the server will
 *    never decrease the queue limit below that value.
 *
 * It works by measuring the rate of responses and using it to
 * dynamically configure the maximum number of queued messages. It also
 * accumulates the integral of the queue size over time and uses that value
 * to decrease the queue limit.
 *
 * It is somewhat inspired by a PID controller which would be continuously
 * and dynamically tuned (without a derivative component).
 */
template <class Clock = std::chrono::steady_clock>
class QIAdmissionController : public AdmissionController {
 public:
  using Duration = typename Clock::duration;
  using TimePoint = typename Clock::time_point;

  virtual ~QIAdmissionController() {}

  explicit QIAdmissionController(
      Duration processTimeout,
      Duration window = std::chrono::seconds(10),
      size_t minQueueLength = 10)
      : windowSec_(toDoubleSecond(window)),
        processTimeoutSec_(toDoubleSecond(processTimeout)),
        minQueueLength_(minQueueLength),
        outgoingRate_(folly::BucketedTimeSeries<double, Clock>(128U, window)),
        integral_(folly::BucketedTimeSeries<double, Clock>(128U, window)),
        queueSize_(0) {}

  /**
   * Return true if the message should be admitted.
   * If true is returned, the queue size has been incremented, otherwise the
   * queueSize is unchanged.
   */
  bool admit() override {
    std::lock_guard<std::mutex> guard(mutex_);
    updateIntegral(queueSize_);
    const auto qLimit = getQueueLimit();
    if (queueSize_ >= qLimit) {
      FB_LOG_EVERY_MS(INFO, 1000) << "LoadShedding: q(" << queueSize_
                                  << ") >= qlimit(" << qLimit << ")";
      return reject();
    }
    return accept();
  }

  /**
   * Indicate to the controller that the server has dequeued 1 request and is
   * currently processing it.
   */
  void dequeue() override {
    std::lock_guard<std::mutex> guard(mutex_);
    CHECK(queueSize_ >= 1);
    updateIntegral(queueSize_);
    queueSize_ -= 1;
  }

  /**
   * Indicate to the controller that the server has finished processing the
   * request, and it returned a response to the client.
   */
  void returnedResponse() override {
    std::lock_guard<std::mutex> guard(mutex_);
    outgoingRate_.addValue(Clock::now(), 1.0);
  }

 private:
  double getResponseRate() const {
    return outgoingRate_.sum() / windowSec_;
  }

  size_t getQueueSize() const {
    return queueSize_;
  }

  double getIntegral() const {
    return integral_.sum();
  }

  /**
   * maxQueue represents the maximum number of elements we allow in the queue.
   *
   * In practice the queue latency (how much time a message stays in the
   * queue) shouldn't be more than processTimeout.
   */
  double getMaxQueue() const {
    const auto responsePerSec = std::max(1.0, getResponseRate());
    return std::max(minQueueLength_, processTimeoutSec_ * responsePerSec);
  }

  double getMaxIntegral() const {
    return getMaxQueue() * windowSec_;
  }

  /**
   * The queue limit represents the number of elements we allow in the queue,
   * based on the processTimeout but also based on the queue usage over time.
   *
   * i.e.: if the queue is full all the time, the integral ratio will increase
   * and the algorithm will decrease the queue limit.
   */
  double getQueueLimit() const {
    const auto maxQ = getMaxQueue();

    // Integral ratio (integral / max integral) is used to reduce the limit
    // i.e. if we use too much of the queue for too long, then we become
    // more aggresive and reduce the queue limit.
    const auto maxIntegral = maxQ * windowSec_;
    const auto integralRatio = std::min(0.99, getIntegral() / maxIntegral);
    const auto k = std::max(0.01, 1.0 / (1.0 - integralRatio));
    return std::max(minQueueLength_, maxQ / k);
  }

  double getIntegralRatio() const {
    return getIntegral() / getMaxIntegral();
  }

  virtual void reportMetrics(
      const AdmissionController::MetricReportFn& report,
      const std::string& prefix) override {
    report(prefix + "queue_size", getQueueSize());
    report(prefix + "queue_max", getMaxQueue());
    report(prefix + "queue_limit", getQueueLimit());
    report(prefix + "response_rate", getResponseRate());
    report(prefix + "integral", getIntegral());
    report(prefix + "integral_ratio", getIntegralRatio());
  }

 private:
  bool reject() {
    return false;
  }

  bool accept() {
    queueSize_ += 1;
    return true;
  }

  /**
   * Update the integral value with the queue value for the last interval
   */
  void updateIntegral(size_t valueForLastInterval) {
    const auto now = Clock::now();
    double dt =
        std::chrono::duration<double>(now - integral_.getLatestTime()).count();
    integral_.addValue(now, valueForLastInterval * dt);
  }

  static double toDoubleSecond(Duration duration) {
    return std::chrono::duration_cast<std::chrono::duration<double>>(duration)
        .count();
  }

  const double windowSec_;
  const double processTimeoutSec_;
  const double minQueueLength_;

  std::mutex mutex_;
  // Accesses to the following members should lock mutex_
  folly::BucketedTimeSeries<double, Clock> outgoingRate_;
  folly::BucketedTimeSeries<double, Clock> integral_;
  size_t queueSize_;
};

} // namespace thrift
} // namespace apache
