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

#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/perf/cpp2/util/QPSStats.h>
#include <thrift/perf/cpp2/util/SimpleOps.h>

using apache::thrift::ClientReceiveState;
using apache::thrift::RequestCallback;
using facebook::thrift::benchmarks::QPSStats;

enum OP_TYPE {
  NOOP = 0,
  NOOP_ONEWAY = 1,
  SUM = 2,
};

template <typename AsyncClient>
class Operation {
 public:
  Operation(std::unique_ptr<AsyncClient> client, QPSStats* stats)
      : client_(std::move(client)),
        noop_(std::make_unique<Noop<AsyncClient>>(stats)),
        sum_(std::make_unique<Sum<AsyncClient>>(stats)) {}
  ~Operation() = default;

  void async(OP_TYPE op, std::unique_ptr<RequestCallback> cb) {
    switch (op) {
      case NOOP:
        noop_->async(client_.get(), std::move(cb));
        break;
      case SUM:
        sum_->async(client_.get(), std::move(cb));
        break;
      default:
        break;
    }
  }

  void asyncReceived(OP_TYPE op, ClientReceiveState&& rstate) {
    switch (op) {
      case NOOP:
        noop_->asyncReceived(client_.get(), std::move(rstate));
        break;
      case SUM:
        sum_->asyncReceived(client_.get(), std::move(rstate));
        break;
      default:
        break;
    }
  }

 private:
  std::unique_ptr<AsyncClient> client_;
  std::unique_ptr<Noop<AsyncClient>> noop_;
  std::unique_ptr<Sum<AsyncClient>> sum_;
};
