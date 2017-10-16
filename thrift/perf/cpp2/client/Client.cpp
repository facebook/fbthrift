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

#include <thrift/perf/cpp2/client/Client.h>

// Server Settings
DEFINE_string(host, "::1", "Server host");
DEFINE_int32(port, 7777, "Server port");

// Client Settings
DEFINE_int32(num_clients, 0, "Number of clients to use. (Default: 1 per core)");
DEFINE_string(transport, "header", "Transport to use: header, rsocket, http2");

// General Settings
DEFINE_int32(stats_interval_sec, 1, "Seconds between stats");
DEFINE_int32(terminate_sec, 0, "How long to run client (0 means forever)");

// Async Operations Settings
DEFINE_bool(async, false, "Perform asynchronous calls to the server");
DEFINE_int32(max_outstanding_ops, 100, "Max number of outstanding async ops");

// Operations
DEFINE_int32(noop_weight, 0, "Test with a no operation");
DEFINE_int32(sum_weight, 0, "Test with a sum operation");

Runner::Runner(QPSStats* stats, int32_t max_outstanding_ops)
    : stats_(stats), max_outstanding_ops_(max_outstanding_ops) {
  auto addr = folly::SocketAddress(FLAGS_host, FLAGS_port);
  client_ = newClient<BenchmarkAsyncClient>(&eb_, addr, FLAGS_transport);

  // Add every Operation object here
  std::vector<int32_t> weights;
  operations_.push_back(std::make_unique<Noop>(stats_));
  weights.push_back(FLAGS_noop_weight);
  operations_.push_back(std::make_unique<Sum>(stats_));
  weights.push_back(FLAGS_sum_weight);

  d_ = std::make_unique<std::discrete_distribution<int32_t>>(
      weights.begin(), weights.end());
}

void Runner::run() {
  // TODO: Implement sync calls.
  // Currently you can "simulate" sync calls by only creating 1 client.
  if (FLAGS_async) {
    while (outstanding_ops_ < max_outstanding_ops_) {
      asyncCalls();
    }
  }
}

void Runner::asyncCalls() {
  // Get a random operation given the operator weights
  auto op = operations_[(*d_)(gen_)].get();
  op->async(
      client_.get(), std::make_unique<LoadCallback>(this, client_.get(), op));
  ++outstanding_ops_;
}

void Runner::loopEventBase() {
  eb_.loopForever();
}

void Runner::finishCall() {
  --outstanding_ops_;
  // Attempt to perform more async calls
  run();
}

void LoadCallback::replyReceived(ClientReceiveState&& rstate) {
  op_->asyncReceived(client_, std::move(rstate));
  runner_->finishCall();
}

/*
 * This starts num_clients threads with a unique client in each thread.
 * Each client also contains its own eventbase which handles both
 * outgoing and incoming connections.
 */
int main(int argc, char** argv) {
  folly::init(&argc, &argv);
  if (FLAGS_num_clients == 0) {
    int32_t numCores = sysconf(_SC_NPROCESSORS_ONLN);
    FLAGS_num_clients = numCores;
  }

  // Initialize a client per number of threads specified
  QPSStats stats;
  std::vector<std::thread> threads;
  for (int i = 0; i < FLAGS_num_clients; ++i) {
    threads.push_back(std::thread([&]() {
      auto r = std::make_unique<Runner>(&stats, FLAGS_max_outstanding_ops);
      r->run();
      if (FLAGS_async) {
        r->loopEventBase();
      }
    }));
  }

  // Closing connections
  int32_t elapsedTimeSec = 0;
  if (FLAGS_terminate_sec == 0) {
    // Essentially infinite time.
    FLAGS_terminate_sec = 100000000;
  }
  while (true) {
    int32_t sleepTimeSec = std::min(
        FLAGS_terminate_sec - elapsedTimeSec, FLAGS_stats_interval_sec);
    /* sleep override */
    std::this_thread::sleep_for(std::chrono::seconds(sleepTimeSec));
    stats.printStats(sleepTimeSec);
    elapsedTimeSec += sleepTimeSec;
    if (elapsedTimeSec >= FLAGS_terminate_sec) {
      break;
    }
  }
  for (auto& thr : threads) {
    thr.join();
  }
  LOG(INFO) << "Client terminating";
  return 0;
}
