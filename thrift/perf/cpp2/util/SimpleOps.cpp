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

#include <thrift/perf/cpp2/util/SimpleOps.h>

Noop::Noop(QPSStats* stats) : Operation(stats) {
  stats_->registerCounter(op_name_);
}

void Noop::async(
    BenchmarkAsyncClient* client,
    std::unique_ptr<RequestCallback> cb) {
  client->noop(std::move(cb));
}

void Noop::asyncReceived(
    BenchmarkAsyncClient* client,
    ClientReceiveState&& rstate) {
  stats_->add(op_name_);
  client->recv_noop(rstate);
}

Sum::Sum(QPSStats* stats) : Operation(stats) {
  // TODO: Perform different additions per call and verify correctness
  stats_->registerCounter(op_name_);
  request_.x = 0;
  request_.__isset.x = true;
  request_.y = 0;
  request_.__isset.y = true;
}

void Sum::async(
    BenchmarkAsyncClient* client,
    std::unique_ptr<RequestCallback> cb) {
  client->sum(std::move(cb), request_);
}

void Sum::asyncReceived(
    BenchmarkAsyncClient* client,
    ClientReceiveState&& rstate) {
  stats_->add(op_name_);
  client->recv_sum(response_, rstate);
}
