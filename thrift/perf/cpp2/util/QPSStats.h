/*
 * Copyright 2017-present Facebook, Inc.
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

#include <folly/ThreadCachedInt.h>
#include <glog/logging.h>
#include <algorithm>
#include <map>
#include <string>
#include <utility>

namespace facebook {
namespace thrift {
namespace benchmarks {

class Counter {
 public:
  explicit Counter(std::string name)
      : name_(name), value_(0, 10000), lastQueryCount_(0), maxPerSec_(0) {}

  Counter& operator+=(uint32_t inc) {
    value_ += inc;
    return *this;
  }

  Counter& operator++() {
    ++value_;
    return *this;
  }

  double print(double secsSinceLastPrint) {
    double queryCount_ = value_.readFull();
    double lastSecAvg = (queryCount_ - lastQueryCount_) / secsSinceLastPrint;
    lastQueryCount_ = queryCount_;
    maxPerSec_ = std::max(maxPerSec_, lastSecAvg);
    LOG(INFO) << std::scientific << " | QPS: " << lastSecAvg
              << " | Max QPS: " << maxPerSec_
              << " | Total Queries: " << queryCount_
              << " | Operation: " << name_;
    return lastSecAvg;
  }

 private:
  std::string name_;
  folly::ThreadCachedInt<uint32_t> value_;
  double lastQueryCount_;
  double maxPerSec_;
};

class QPSStats {
 public:
  void printStats(double secsSinceLastPrint) {
    double totalQPS = 0;
    for (auto& pair : counters_) {
      totalQPS += pair.second->print(secsSinceLastPrint);
    }
    LOG(INFO) << std::scientific << " | TOTAL QPS: " << totalQPS;
  }

  void registerCounter(std::string name) {
    counters_.emplace(name, std::make_unique<Counter>(name));
  }

  void add(std::string& name) {
    ++(*counters_[name]);
  }

 private:
  std::map<std::string, std::unique_ptr<Counter>> counters_;
};

} // namespace benchmarks
} // namespace thrift
} // namespace facebook
