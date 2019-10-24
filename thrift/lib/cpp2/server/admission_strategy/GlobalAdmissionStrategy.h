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

#include <thrift/lib/cpp2/server/AdmissionController.h>
#include <thrift/lib/cpp2/server/QIAdmissionController.h>
#include <thrift/lib/cpp2/server/admission_strategy/AdmissionStrategy.h>

namespace apache {
namespace thrift {

/**
 * GlobalAdmissionStrategy treats any message request equally, independently
 * from their origin.
 */
class GlobalAdmissionStrategy : public AdmissionStrategy {
 public:
  template <class Clock = std::chrono::steady_clock>
  explicit GlobalAdmissionStrategy(typename Clock::duration processTimeout)
      : GlobalAdmissionStrategy(
            std::make_shared<QIAdmissionController<Clock>>(processTimeout)) {}

  explicit GlobalAdmissionStrategy(
      std::shared_ptr<AdmissionController> admissionController)
      : admissionController_(std::move(admissionController)) {}

  ~GlobalAdmissionStrategy() {}

  /**
   * Always return the same shared AdmissionController.
   */
  std::shared_ptr<AdmissionController> select(
      const std::string&,
      const transport::THeader*) override {
    return admissionController_;
  }

  void reportMetrics(
      const AdmissionStrategy::MetricReportFn& report,
      const std::string& prefix) override {
    admissionController_->reportMetrics(report, prefix + "global.");
  }

  Type getType() override {
    return AdmissionStrategy::GLOBAL;
  }

 private:
  std::shared_ptr<AdmissionController> admissionController_;
};

} // namespace thrift
} // namespace apache
