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

#include <folly/Logging.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/perf/cpp2/if/gen-cpp2/ApiBase_types.h>
#include <thrift/perf/cpp2/util/QPSStats.h>
#include <random>

using apache::thrift::ClientReceiveState;
using apache::thrift::RequestCallback;
using facebook::thrift::benchmarks::QPSStats;
using facebook::thrift::benchmarks::TwoInts;

template <typename AsyncClient>
class Noop {
 public:
  Noop(QPSStats* stats) : stats_(stats) {
    stats_->registerCounter(op_name_);
    stats_->registerCounter(timeout_);
    stats_->registerCounter(error_);
    stats_->registerCounter(fatal_);
  }
  ~Noop() = default;

  void async(AsyncClient* client, std::unique_ptr<RequestCallback> cb) {
    client->noop(std::move(cb));
  }

  void onewayAsync(AsyncClient* client, std::unique_ptr<RequestCallback> cb) {
    client->onewayNoop(std::move(cb));
  }

  void asyncReceived(AsyncClient* client, ClientReceiveState&& rstate) {
    try {
      client->recv_noop(rstate);
      stats_->add(op_name_);
    } catch (const apache::thrift::TApplicationException& ex) {
      if (ex.getType() ==
          apache::thrift::TApplicationException::TApplicationExceptionType::
              TIMEOUT) {
        stats_->add(timeout_);
      } else {
        FB_LOG_EVERY_MS(ERROR, 1000)
            << "Error should have caused error() function to be called: "
            << ex.what();
        stats_->add(error_);
      }
    } catch (const std::exception& ex) {
      FB_LOG_EVERY_MS(ERROR, 1000) << "Critical error: " << ex.what();
      stats_->add(fatal_);
    }
  }

  void error(AsyncClient*, ClientReceiveState&& state) {
    if (state.isException()) {
      FB_LOG_EVERY_MS(INFO, 1000) << "Error is: " << state.exception().what();
    }
    stats_->add(error_);
  }

  void onewaySent() {
    stats_->add(op_name_);
  }

 private:
  QPSStats* stats_;
  std::string op_name_ = "noop";
  std::string timeout_ = "timeout";
  std::string error_ = "error";
  std::string fatal_ = "fatal";
};

template <typename AsyncClient>
class Sum {
 public:
  Sum(QPSStats* stats) : stats_(stats) {
    // TODO: Perform different additions per call and verify correctness
    stats_->registerCounter(op_name_);
    stats_->registerCounter(timeout_);
    stats_->registerCounter(error_);
    stats_->registerCounter(fatal_);
  }
  ~Sum() = default;

  void async(AsyncClient* client, std::unique_ptr<RequestCallback> cb) {
    request_.x = gen_();
    request_.__isset.x = true;
    request_.y = gen_();
    request_.__isset.y = true;

    client->sum(std::move(cb), request_);
  }

  void asyncReceived(AsyncClient* client, ClientReceiveState&& rstate) {
    try {
      client->recv_sum(response_, rstate);
      CHECK_EQ(request_.x + request_.y, response_.x);
      CHECK_EQ(request_.x - request_.y, response_.y);
      stats_->add(op_name_);
    } catch (const apache::thrift::TApplicationException& ex) {
      if (ex.getType() ==
          apache::thrift::TApplicationException::TApplicationExceptionType::
              TIMEOUT) {
        stats_->add(timeout_);
      } else {
        FB_LOG_EVERY_MS(ERROR, 1000)
            << "Error should have caused error() function to be called: "
            << ex.what();
        stats_->add(error_);
      }
    } catch (const std::exception& ex) {
      FB_LOG_EVERY_MS(ERROR, 1000) << "Critical error: " << ex.what();
      stats_->add(fatal_);
    }
  }

  void error(AsyncClient*, ClientReceiveState&& state) {
    if (state.isException()) {
      FB_LOG_EVERY_MS(INFO, 1000) << "Error is: " << state.exception().what();
    }
    stats_->add(error_);
  }

 private:
  QPSStats* stats_;
  std::string op_name_ = "sum";
  std::string timeout_ = "timeout";
  std::string error_ = "error";
  std::string fatal_ = "fatal";
  TwoInts request_;
  TwoInts response_;
  std::mt19937 gen_{std::random_device()()};
};

template <typename AsyncClient>
class Timeout {
 public:
  Timeout(QPSStats* stats) : stats_(stats) {
    stats_->registerCounter(op_name_);
    stats_->registerCounter(timeout_);
    stats_->registerCounter(error_);
    stats_->registerCounter(fatal_);
  }
  ~Timeout() = default;

  void async(AsyncClient* client, std::unique_ptr<RequestCallback> cb) {
    apache::thrift::RpcOptions rpcOptions;
    rpcOptions.setQueueTimeout(std::chrono::milliseconds(3));
    rpcOptions.setTimeout(std::chrono::milliseconds(3));
    client->timeout(rpcOptions, std::move(cb));
  }

  void asyncReceived(AsyncClient* client, ClientReceiveState&& rstate) {
    try {
      client->recv_timeout(rstate);
      stats_->add(op_name_);
    } catch (const apache::thrift::TApplicationException& ex) {
      if (ex.getType() ==
          apache::thrift::TApplicationException::TApplicationExceptionType::
              TIMEOUT) {
        stats_->add(timeout_);
      } else {
        FB_LOG_EVERY_MS(ERROR, 1000)
            << "Error should have caused error() function to be called: "
            << ex.what();
        stats_->add(error_);
      }
    } catch (const std::exception& ex) {
      FB_LOG_EVERY_MS(ERROR, 1000) << "Critical error: " << ex.what();
      stats_->add(fatal_);
    }
  }

  void error(AsyncClient*, ClientReceiveState&& state) {
    if (state.isException()) {
      FB_LOG_EVERY_MS(INFO, 1000) << "Error is: " << state.exception().what();
    }
    stats_->add(error_);
  }

 private:
  QPSStats* stats_;
  std::string op_name_ = "timeout_success";
  std::string timeout_ = "timeout";
  std::string error_ = "error";
  std::string fatal_ = "fatal";
  TwoInts request_;
  TwoInts response_;
};
