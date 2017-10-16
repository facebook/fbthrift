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

#include <thrift/perf/cpp2/util/Operation.h>

using apache::thrift::ClientReceiveState;
using apache::thrift::RequestCallback;
using facebook::thrift::benchmarks::BenchmarkAsyncClient;
using facebook::thrift::benchmarks::QPSStats;
using facebook::thrift::benchmarks::TwoInts;

class Noop : public Operation {
 public:
  Noop(QPSStats* stats);
  virtual ~Noop() = default;

  virtual void async(
      BenchmarkAsyncClient* client,
      std::unique_ptr<RequestCallback> cb) override;

  virtual void asyncReceived(
      BenchmarkAsyncClient* client,
      ClientReceiveState&& rstate) override;

 private:
  std::string op_name_ = "noop";
};

class Sum : public Operation {
 public:
  Sum(QPSStats* stats);
  virtual ~Sum() = default;

  virtual void async(
      BenchmarkAsyncClient* client,
      std::unique_ptr<RequestCallback> cb) override;

  virtual void asyncReceived(
      BenchmarkAsyncClient* client,
      ClientReceiveState&& rstate) override;

 private:
  std::string op_name_ = "sum";
  TwoInts request_;
  TwoInts response_;
};
