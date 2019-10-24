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
#include <string>
#include <unordered_map>

#include <folly/Function.h>

namespace apache {
namespace thrift {

class AdmissionController {
 public:
  using MetricReportFn =
      folly::Function<void(const std::string&, double) const>;

  enum AggregationType { SUM, AVG };

  virtual ~AdmissionController() {}

  /**
   * Return true if the message should be admitted.
   */
  virtual bool admit() = 0;

  /**
   * Indicate to the controller that the server has dequeued 1 request and is
   * currently processing it.
   */
  virtual void dequeue() = 0;

  /**
   * Indicate to the controller that the server has finished processing the
   * request, and it returned a response to the client.
   */
  virtual void returnedResponse(std::chrono::nanoseconds) = 0;

  /**
   * Delegate reporting the metrics to the underlying implementation
   * The last argument must contain the current metric values in case the
   * implementation needs to do aggregation.
   */
  virtual void reportMetrics(
      const MetricReportFn& /*reportFunction*/,
      const std::string& /*prefix*/,
      const std::unordered_map<std::string, double>& /*previousValues*/ = {},
      uint32_t /*previousValueCount*/ = 0) {}
};

class DenyAllAdmissionController : public AdmissionController {
 public:
  ~DenyAllAdmissionController() {}

  bool admit() override {
    return false;
  }

  void dequeue() override {}

  void returnedResponse(std::chrono::nanoseconds) override {}
};

class AcceptAllAdmissionController : public AdmissionController {
 public:
  ~AcceptAllAdmissionController() {}

  bool admit() override {
    return true;
  }

  void dequeue() override {}

  void returnedResponse(std::chrono::nanoseconds) override {}
};

} // namespace thrift
} // namespace apache
