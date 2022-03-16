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

#include <thrift/test/stresstest/StressTest.h>

#include <folly/SocketAddress.h>
#include <folly/init/Init.h>
#include <folly/portability/GFlags.h>
#include <thrift/test/stresstest/client/ClientRunner.h>
#include <thrift/test/stresstest/client/StressTestRegistry.h>

using namespace apache::thrift::stress;

DEFINE_string(host, "::1", "Host running the stress test server");
DEFINE_int32(port, 5000, "Port of the stress test server");

DEFINE_string(test_name, "", "Stress test to run");
DEFINE_int64(runtime_s, 10, "Runtime of test in seconds");
DEFINE_int64(client_threads, 1, "Nnumber of client threads");
DEFINE_int64(clients_per_thread, 1, "Number of clients per client thread");

void runStressTest(std::unique_ptr<StressTestBase> test) {
  // initialize the client runner
  folly::SocketAddress addr(FLAGS_host, FLAGS_port);
  ClientConfig cfg{
      .numClientThreads = static_cast<uint64_t>(FLAGS_client_threads),
      .numClientsPerThread = static_cast<uint64_t>(FLAGS_clients_per_thread)};
  ClientRunner runner(cfg, addr);

  runner.run(test.get());

  // sleep for duration of the test
  std::this_thread::sleep_until(
      std::chrono::steady_clock::now() + std::chrono::seconds(FLAGS_runtime_s));

  // stop runner
  runner.stop();
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
