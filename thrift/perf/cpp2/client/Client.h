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

#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <random>
#include <thrift/perf/cpp2/if/gen-cpp2/Benchmark.h>
#include <thrift/perf/cpp2/util/QPSStats.h>
#include <thrift/perf/cpp2/util/SimpleOps.h>
#include <thrift/perf/cpp2/util/Util.h>

using apache::thrift::ClientConnectionIf;
using apache::thrift::ClientReceiveState;
using apache::thrift::RequestCallback;
using facebook::thrift::benchmarks::BenchmarkAsyncClient;
using facebook::thrift::benchmarks::QPSStats;

class LoadCallback;

class Runner {
 public:
  friend class LoadCallback;

  Runner(QPSStats* stats, int32_t max_outstanding_ops);

  void run();
  void asyncCalls();
  void loopEventBase();
  void finishCall();

 private:
  QPSStats* stats_;
  int32_t max_outstanding_ops_;

  int32_t outstanding_ops_ = 0;
  std::mt19937 gen_{std::random_device()()};
  folly::EventBase eb_;
  std::unique_ptr<BenchmarkAsyncClient> client_;
  std::vector<std::unique_ptr<Operation>> operations_;
  std::unique_ptr<std::discrete_distribution<int32_t>> d_;
};

class LoadCallback : public RequestCallback {
 public:
  LoadCallback(Runner* runner, BenchmarkAsyncClient* client, Operation* op)
      : runner_(runner), client_(client), op_(op) {}

  // TODO: Properly handle errors and exceptions
  void requestSent() override {}
  void replyReceived(ClientReceiveState&& rstate) override;
  void requestError(ClientReceiveState&&) override {}

 private:
  Runner* runner_;
  BenchmarkAsyncClient* client_;
  Operation* op_;
};
