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

#include <thrift/perf/cpp2/if/gen-cpp2/Benchmark.h>
#include <thrift/perf/cpp2/util/QPSStats.h>

namespace facebook {
namespace thrift {
namespace benchmarks {

using apache::thrift::HandlerCallback;
using apache::thrift::HandlerCallbackBase;

class BenchmarkHandler : virtual public BenchmarkSvIf {
 public:
  explicit BenchmarkHandler(QPSStats* stats) : stats_(stats) {
    stats_->registerCounter(kNoop_);
    stats_->registerCounter(kSum_);
    stats_->registerCounter(kTimeout_);
  }

  void async_eb_noop(std::unique_ptr<HandlerCallback<void>> callback) override {
    stats_->add(kNoop_);
    callback->done();
  }

  void async_eb_onewayNoop(std::unique_ptr<HandlerCallbackBase>) override {
    stats_->add(kNoop_);
  }

  // Make the async/worker thread sleep
  void timeout() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    stats_->add(kTimeout_);
  }

  void async_eb_sum(
      std::unique_ptr<HandlerCallback<std::unique_ptr<TwoInts>>> callback,
      std::unique_ptr<TwoInts> input) override {
    stats_->add(kSum_);
    auto result = std::make_unique<TwoInts>();
    result->x = input->x + input->y;
    result->__isset.x = true;
    result->y = input->x - input->y;
    result->__isset.y = true;
    callback->result(std::move(result));
  }

 private:
  QPSStats* stats_;
  std::string kNoop_ = "noop";
  std::string kSum_ = "sum";
  std::string kTimeout_ = "timeout";
};

} // namespace benchmarks
} // namespace thrift
} // namespace facebook
