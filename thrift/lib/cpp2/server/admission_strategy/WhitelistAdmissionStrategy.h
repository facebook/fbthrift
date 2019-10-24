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

#include <folly/small_vector.h>
#include <folly/sorted_vector_types.h>

#include <thrift/lib/cpp2/server/AdmissionController.h>
#include <thrift/lib/cpp2/server/admission_strategy/AdmissionStrategy.h>

namespace apache {
namespace thrift {

/**
 * WhitelistAdmissionStrategy is a strategy wrapper which will automatically
 * accept (i.e. don't apply any load-shedding mechanism) to any method listed
 * in the white-list.
 * This is useful when you want to disable load-shedding for a specific class
 * of requests (e.g. admin-level methods like getStatus, getCounters...)
 */
template <typename InnerAdmissionStrategy, std::size_t WhiteListSize = 2>
class WhitelistAdmissionStrategy : public AdmissionStrategy {
 public:
  using StringSet = folly::sorted_vector_set<
      std::string,
      std::less<std::string>,
      std::allocator<std::string>,
      void,
      folly::small_vector<std::string, WhiteListSize>>;

  template <typename... InnerStrategyArgs>
  WhitelistAdmissionStrategy(
      const std::unordered_set<std::string>& whitelist,
      InnerStrategyArgs&&... args)
      : WhitelistAdmissionStrategy(
            StringSet(whitelist.begin(), whitelist.end()),
            args...) {}

  template <typename... InnerStrategyArgs>
  WhitelistAdmissionStrategy(
      const StringSet& whitelist,
      InnerStrategyArgs&&... args)
      : innerStrategy_(std::forward<InnerStrategyArgs>(args)...),
        whitelist_(whitelist),
        acceptAllAdmissionController_(
            std::make_shared<AcceptAllAdmissionController>()) {}

  std::shared_ptr<AdmissionController> select(
      const std::string& methodName,
      const transport::THeader* tHeader) override {
    if (whitelist_.find(methodName) != whitelist_.end()) {
      return acceptAllAdmissionController_;
    }
    return innerStrategy_.select(methodName, tHeader);
  }

  void reportMetrics(
      const AdmissionStrategy::MetricReportFn& report,
      const std::string& prefix) override {
    innerStrategy_.reportMetrics(report, prefix);
    log(123);
  }

  Type getType() override {
    return innerStrategy_.getType();
  }

 private:
  InnerAdmissionStrategy innerStrategy_;
  const StringSet whitelist_;
  const std::shared_ptr<AdmissionController> acceptAllAdmissionController_;
};

} // namespace thrift
} // namespace apache
