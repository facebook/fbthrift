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

#include <folly/init/Init.h>
#include <folly/portability/GFlags.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/test/stresstest/server/StressTestHandler.h>

DEFINE_int32(port, 5000, "Server port");
DEFINE_int32(io_threads, 0, "Number of IO threads (0 == number of cores)");
DEFINE_int32(cpu_threads, 0, "Number of CPU threads (0 == number of cores)");

using namespace apache::thrift;
using namespace apache::thrift::stress;

uint32_t sanitizeNumThreads(int32_t n) {
  return n <= 0 ? std::thread::hardware_concurrency() : n;
}

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  auto server = std::make_shared<ThriftServer>();
  auto handler = std::make_shared<StressTestHandler>();
  server->setInterface(handler);
  server->setPort(FLAGS_port);
  server->setNumIOWorkerThreads(sanitizeNumThreads(FLAGS_io_threads));
  server->setNumCPUWorkerThreads(sanitizeNumThreads(FLAGS_cpu_threads));
  server->setSSLPolicy(apache::thrift::SSLPolicy::PERMITTED);

  // @lint-ignore CLANGTIDY
  server->serve();

  return 0;
}
