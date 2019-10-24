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

#include <stdint.h>
#include <chrono>
#include <unordered_map>

#include <thrift/lib/cpp2/server/AdmissionController.h>
#include <thrift/lib/cpp2/server/QIAdmissionController.h>
#include <thrift/lib/cpp2/server/admission_strategy/AdmissionStrategy.h>

namespace apache {
namespace thrift {

/**
 * PriorityAdmissionStrategy asigns priorities to clientId, and guarantee
 * that a specific clientId has a relative priority compared to other clientIds.
 *
 * For instance, a map like this one:
 * {"A": 1, "B": 5, "C": 1, "*": 1}
 * Note: The sum of priorities is 1 + 5 + 1 + 1 = 8
 * It means that "A" receives a 1/8th of the resources
 * "B" receives 5/8th of them... and so on
 * "*" is the catch all priority (any clientId not specified explicitly
 * will be caught by the "*" priority).
 * If you don't specify it, all requests with a non-recognized clientId will
 * be rejected.
 * Note: A priority of 0 means that every requests will be rejected.
 */
class PriorityAdmissionStrategy : public AdmissionStrategy {
  struct Priority {
    Priority() : offset(0), index(0), priority(0) {}

    Priority(uint32_t offset_, uint8_t priority_)
        : offset(offset_), index(0), priority(priority_) {}

    // only used during initialization of priorities_
    Priority(const Priority& other)
        : offset(other.offset), index(0), priority(other.priority) {
      index = other.index.load(std::memory_order_seq_cst);
    }

    uint32_t offset;
    std::atomic<uint32_t> index;
    uint8_t priority;
  };

 public:
  template <class Clock = std::chrono::steady_clock>
  explicit PriorityAdmissionStrategy(
      typename Clock::duration processTimeout,
      const std::string& clientIdHeaderName)
      : PriorityAdmissionStrategy(
            [processTimeout]() {
              return std::make_shared<QIAdmissionController<Clock>>(
                  processTimeout);
            },
            clientIdHeaderName) {}

  ~PriorityAdmissionStrategy() {}

  /**
   * Create an PriorityAdmissionStrategy
   *
   * `priorities` is a map of clientId to absolute priority.
   * `factory` is the function used to create a new admission controller
   * `clientIdHeaderName` is the header read from request headers to
   * identify a client
   */
  PriorityAdmissionStrategy(
      const std::unordered_map<std::string, uint8_t>& priorities,
      folly::Function<std::shared_ptr<AdmissionController>()> factory,
      const std::string& clientIdHeaderName)
      : clientIdHeaderName_(clientIdHeaderName) {
    auto offset = 0U;
    for (const auto& entry : priorities) {
      priorities_.insert({entry.first, Priority(offset, entry.second)});
      offset += entry.second;
    }
    admissionControllers_.reserve(offset);
    for (auto i = 0U; i < offset; i++) {
      admissionControllers_.emplace_back(factory());
    }

    denyAdmissionController_ = std::make_shared<DenyAllAdmissionController>();

    // If the user doesn't provide a wildcard controller, we add one which
    // denies all requests
    if (priorities_.find(kWildcard) == priorities_.end()) {
      priorities_.insert({kWildcard, Priority(offset, 0)});
    }
  }

  /**
   * Select an AdmissionController to be used for this specific request.
   * It returns a shared AdmissionController based on the clientId and
   * the priority of the clientId.
   */
  std::shared_ptr<AdmissionController> select(
      const std::string&,
      const transport::THeader* tHeader) override {
    auto bucketIndex = computeBucketIndex(tHeader);
    if (bucketIndex < 0) {
      return denyAdmissionController_;
    }
    return admissionControllers_[bucketIndex];
  }

  void reportMetrics(
      const AdmissionStrategy::MetricReportFn& report,
      const std::string& prefix) override {
    for (const auto& entry : priorities_) {
      auto clientId = entry.first;
      auto priority = entry.second;

      std::unordered_map<std::string, double> metrics;

      const auto offset = priority.offset;
      const auto newPrefix = prefix + "priority." + clientId + ".";
      for (auto i = 0; i < priority.priority; i++) {
        const auto& controller = admissionControllers_[offset + i];

        controller->reportMetrics(
            [&metrics](const auto& key, auto value) { metrics[key] = value; },
            newPrefix,
            metrics,
            i + 1);
      }

      for (const auto& entry : metrics) {
        report(entry.first, entry.second);
      }
      report(newPrefix + "priority", priority.priority);
    }
  }

  Type getType() override {
    return AdmissionStrategy::PRIORITY;
  }

 private:
  /**
   * Compute a bucket index based on the client-id
   *
   * This method compute a bucket index based on the priorities.
   * E.g. for priorities like this: {"A": 1, "B": 3, WILDCARD: 1}
   * It will generates indexes like these:
   * "A" -> 0
   * "B" -> 1, 2, 3 (returned in a round-robin way)
   * WILDCARD -> 4
   */
  int computeBucketIndex(const transport::THeader* theader) {
    auto& priority = getPriority(theader);
    if (priority.priority == 0) {
      return -1; // deny all requests if priority == 0
    }
    priority.index++;
    auto index = priority.offset + priority.index % priority.priority;
    return index;
  }

  Priority& getPriority(const transport::THeader* theader) {
    if (theader != nullptr) {
      const auto& headers = theader->getHeaders();
      auto clientIdIt = headers.find(clientIdHeaderName_);
      if (clientIdIt != headers.end()) {
        auto priorityIt = priorities_.find(clientIdIt->second);
        if (priorityIt != priorities_.end()) {
          return priorityIt->second;
        }
      }
    }
    // Garanteed to exist (created if absent in the constructor)
    auto it = priorities_.find(kWildcard);
    CHECK(it != priorities_.end());
    return it->second;
  }

  std::vector<std::shared_ptr<AdmissionController>> admissionControllers_;
  std::shared_ptr<AdmissionController> denyAdmissionController_;
  std::unordered_map<std::string, Priority> priorities_;
  const std::string clientIdHeaderName_;
};

} // namespace thrift
} // namespace apache
