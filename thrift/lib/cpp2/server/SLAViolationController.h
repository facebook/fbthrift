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

#pragma once

#include <chrono>

#include <folly/Random.h>
#include <glog/logging.h>

#include <thrift/lib/cpp2/server/AdmissionController.h>
#include <thrift/lib/cpp2/util/Ewma.h>

namespace apache {
namespace thrift {

/**
 * AdmissionController which measures the rate of SLA violations, i.e. the
 * percentage of time the processing of a response exceed the SLA specified
 * in argument.
 * Here SLA means "Service Level Aggreement", i.e. the latency of the responses
 * the server is returning.
 */
template <
    typename InnerAdmissionController,
    class Clock = std::chrono::steady_clock>
class SLAViolationController : public AdmissionController {
 public:
  using clock = Clock;
  using duration = typename clock::duration;
  using time_point = typename clock::time_point;

  /**
   * ratioThreshold: percentage of violation after which the controller will
   *                 start rejecting.
   * sla: latency objective that the server expect to respect, i.e. repsonse
   *      latencies should be below that value.
   * window: Duration of the window used for computing statistics
   */
  SLAViolationController(
      double ratioThreshold,
      duration sla,
      duration window,
      InnerAdmissionController&& innerController)
      : innerController_(innerController),
        sla_(std::chrono::duration_cast<std::chrono::nanoseconds>(sla)),
        ratioThreshold_(ratioThreshold),
        denominator_(1.0 / (1.0 - ratioThreshold_)),
        slaRatio_(window, 0.0),
        window_(window),
        lastAdmissionDate_(Clock::now()) {}

  bool admit() override {
    const auto now = Clock::now();
    if (ratioThreshold_ == 0.0) {
      return innerController_.admit();
    }
    // In case ratio fall to low, we want to recover from that
    if (now - lastAdmissionDate_ > window_) {
      lastAdmissionDate_ = now;
      return innerController_.admit();
    }

    // TODO: check time of fresh data
    auto ratio = slaRatio_.estimate();
    auto underThreshold = ratio < ratioThreshold_;
    auto accept = underThreshold;
    if (!accept) {
      // rescale the ratio so that the probability of rejection after the
      // threshold is 0% and 100% when ratio = 1.0
      auto scaledRatio = (ratio - ratioThreshold_) * denominator_;
      accept = folly::Random::randDouble01() > scaledRatio;
    }
    return accept && innerController_.admit();
  }

  void dequeue() override {
    innerController_.dequeue();
  }

  double ewma() {
    return slaRatio_.estimate();
  }

  void returnedResponse(std::chrono::nanoseconds latency) override {
    if (ratioThreshold_ != 0.0) {
      slaRatio_.add(latency > sla_ ? 1.0 : 0);
    }
    innerController_.returnedResponse(latency);
  }

 private:
  InnerAdmissionController innerController_;
  const std::chrono::nanoseconds sla_;
  const double ratioThreshold_;
  const double denominator_;
  Ewma<Clock> slaRatio_;
  const typename Clock::duration window_;
  typename Clock::time_point lastAdmissionDate_;
};

} // namespace thrift
} // namespace apache
