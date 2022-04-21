/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/conformance/stresstest/StressTest.h>

#include <fmt/core.h>
#include <folly/SocketAddress.h>
#include <folly/init/Init.h>
#include <folly/portability/GFlags.h>
#include <thrift/conformance/stresstest/client/ClientRunner.h>
#include <thrift/conformance/stresstest/client/StressTestRegistry.h>
#include <thrift/conformance/stresstest/util/Util.h>

using namespace apache::thrift::stress;

DEFINE_string(host, "::1", "Host running the stress test server");
DEFINE_int32(port, 5000, "Port of the stress test server");

DEFINE_string(test_name, "", "Stress test to run");
DEFINE_int64(runtime_s, 10, "Runtime of test in seconds");
DEFINE_int64(client_threads, 1, "Nnumber of client threads");
DEFINE_int64(clients_per_thread, 1, "Number of clients per client thread");

DEFINE_string(
    security,
    "",
    "Set to 'FIZZ' or 'TLS' to enable security (requires certPath, keyPath and caPath to be specified)");
DEFINE_string(
    certPath,
    "folly/io/async/test/certs/tests-cert.pem",
    "Path to client certificate file");
DEFINE_string(
    keyPath,
    "folly/io/async/test/certs/tests-key.pem",
    "Path to client key file");
DEFINE_string(
    caPath,
    "folly/io/async/test/certs/ca-cert.pem",
    "Path to client trusted CA file");

struct StressTestStats {
  ClientThreadMemoryStats memoryStats;
  ClientRpcStats rpcStats;

  void log() {
    LOG(INFO) << fmt::format(
        "Total requests:        {} ({} succeeded, {} failed)",
        (rpcStats.numSuccess + rpcStats.numFailure),
        rpcStats.numSuccess,
        rpcStats.numFailure);
    LOG(INFO) << fmt::format(
        "Average QPS:           {:.2f}",
        (static_cast<double>(rpcStats.numSuccess) / FLAGS_runtime_s));
    LOG(INFO) << fmt::format(
        "Request latency:       P50={:.2f}us, P99={:.2f}us, P100={:.2f}us",
        rpcStats.latencyHistogram.getPercentileEstimate(.5),
        rpcStats.latencyHistogram.getPercentileEstimate(.99),
        rpcStats.latencyHistogram.getPercentileEstimate(1.0));
    LOG(INFO) << "Allocated memory stats:";
    LOG(INFO) << fmt::format(
        "  Before test:         {} bytes", memoryStats.threadStart);
    LOG(INFO) << fmt::format(
        "  Clients connected:   {} bytes", memoryStats.connectionsEstablished);
    LOG(INFO) << fmt::format(
        "  During test:         P50={} bytes, P99={} bytes, P100={} bytes",
        memoryStats.p50,
        memoryStats.p99,
        memoryStats.p100);
    LOG(INFO) << fmt::format(
        "  Clients idle:        {} bytes", memoryStats.connectionsIdle);
  }
};

ClientConnectionConfig getClientConnectionConfig() {
  ClientSecurity security;
  if (FLAGS_security.empty()) {
    security = ClientSecurity::None;
  } else {
    if (FLAGS_security == "TLS") {
      security = ClientSecurity::TLS;
    } else if (FLAGS_security == "FIZZ") {
      security = ClientSecurity::FIZZ;
    } else {
      LOG(FATAL) << "Unrecognized option for security: " << FLAGS_security;
    }
  }
  return ClientConnectionConfig{
      .security = security,
      .certPath = FLAGS_certPath,
      .keyPath = FLAGS_keyPath,
      .trustedCertsPath = FLAGS_caPath,
  };
}

void runStressTest(std::unique_ptr<StressTestBase> test) {
  resetMemoryStats();

  // initialize the client runner
  folly::SocketAddress addr(FLAGS_host, FLAGS_port);
  ClientConfig cfg{
      .numClientThreads = static_cast<uint64_t>(FLAGS_client_threads),
      .numClientsPerThread = static_cast<uint64_t>(FLAGS_clients_per_thread),
      .connConfig = getClientConnectionConfig()};
  ClientRunner runner(cfg, addr);

  // run the test and sleep for the duration
  runner.run(test.get());
  std::this_thread::sleep_for(std::chrono::seconds(FLAGS_runtime_s));
  runner.stop();

  // collect and print statistics
  StressTestStats stats{
      .memoryStats = runner.getMemoryStats(),
      .rpcStats = runner.getRpcStats(),
  };
  stats.log();
}

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);

  // access stress test registry and make sure we have registered tests
  auto allTests = StressTestRegistry::getInstance().listAll();
  if (allTests.empty()) {
    LOG(FATAL) << "No tests found!";
  }

  // run the specified test (if provided and it exists), otherwise run all tests
  if (!FLAGS_test_name.empty()) {
    if (auto test = StressTestRegistry::getInstance().create(FLAGS_test_name)) {
      LOG(INFO) << "Running '" << FLAGS_test_name << "' for " << FLAGS_runtime_s
                << " seconds";
      runStressTest(std::move(test));
    } else {
      LOG(FATAL) << "Test '" << FLAGS_test_name << " not found!";
    }
  } else {
    for (auto& testName : allTests) {
      LOG(INFO) << "Running '" << testName << "' for " << FLAGS_runtime_s
                << " seconds";
      runStressTest(StressTestRegistry::getInstance().create(testName));
    }
  }
}
