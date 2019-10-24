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

#include <folly/Synchronized.h>

#include <thrift/lib/cpp2/server/AdmissionController.h>
#include <thrift/lib/cpp2/server/QIAdmissionController.h>
#include <thrift/lib/cpp2/server/admission_strategy/AdmissionStrategy.h>

namespace apache {
namespace thrift {

/**
 * PerClientIdAdmissionStrategy asigns equal priority for each clientId it sees.
 */
class PerClientIdAdmissionStrategy : public AdmissionStrategy {
 public:
  using AdmissionControllerFactoryFn =
      folly::Function<std::shared_ptr<AdmissionController>(
          const std::string& clientId)>;

  template <class Clock = std::chrono::steady_clock>
  explicit PerClientIdAdmissionStrategy(
      typename Clock::duration processTimeout,
      const std::string& clientIdHeaderName)
      : PerClientIdAdmissionStrategy(
            [processTimeout](auto&) {
              return std::make_shared<QIAdmissionController<Clock>>(
                  processTimeout);
            },
            clientIdHeaderName) {}

  explicit PerClientIdAdmissionStrategy(
      AdmissionControllerFactoryFn factory,
      const std::string& clientIdHeaderName)
      : factory_(std::move(factory)),
        wildcardController_(factory_(kWildcard)),
        clientIdHeaderName_(clientIdHeaderName) {}

  ~PerClientIdAdmissionStrategy() {}

  /**
   * Select an AdmissionController to be used for this specific request.
   * It returns one shared AdmissionController per clientId.
   */
  std::shared_ptr<AdmissionController> select(
      const std::string&,
      const transport::THeader* theader) override {
    if (theader == nullptr) {
      return wildcardController_;
    }

    const auto& headers = theader->getHeaders();
    auto headersIt = headers.find(clientIdHeaderName_);
    if (headersIt == headers.end() || headersIt->second == kWildcard) {
      return wildcardController_;
    }

    const auto& clientId = headersIt->second;
    {
      // Fast path
      auto readOnlyAdmController = admissionControllers_.rlock();
      auto it = readOnlyAdmController->find(clientId);
      if (it != readOnlyAdmController->end()) {
        return it->second;
      }
    }

    // Slow path, initialization of the admission controller
    auto readWriteAdmController = admissionControllers_.wlock();
    auto it = readWriteAdmController->find(clientId);
    if (it != readWriteAdmController->end()) {
      return it->second;
    }
    auto admController = factory_(clientId);
    // TODO: garbage collect when admissionControllers_ is too big
    readWriteAdmController->insert({clientId, admController});
    return admController;
  }

  void reportMetrics(
      const AdmissionStrategy::MetricReportFn& report,
      const std::string& prefix) override {
    auto controllers = admissionControllers_.rlock();

    for (auto it = controllers->begin(); it != controllers->end(); it++) {
      const auto& clientId = it->first;
      const auto& controller = it->second;

      controller->reportMetrics(report, prefix + "." + clientId + ".");
    }
  }

  Type getType() override {
    return AdmissionStrategy::PER_CLIENT_ID;
  }

 private:
  using ControllerMap =
      std::unordered_map<std::string, std::shared_ptr<AdmissionController>>;
  AdmissionControllerFactoryFn factory_;
  folly::Synchronized<ControllerMap> admissionControllers_;
  std::shared_ptr<AdmissionController> wildcardController_;
  const std::string clientIdHeaderName_;
};

} // namespace thrift
} // namespace apache
