/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <thrift/lib/cpp/test/loadgen/loadgen.h>
#include <thrift/lib/cpp/test/loadgen/QpsMonitor.h>
#include <thrift/lib/cpp/test/loadgen/RNG.h>
#include "thrift/perf/cpp/ClientLoadConfig.h"
#include "thrift/perf/cpp/ClientWorker.h"
#include "thrift/perf/cpp/ClientWorker2.h"
#include "thrift/perf/cpp/AsyncClientWorker.h"
#include "thrift/perf/cpp/AsyncClientWorker2.h"
#include "common/services/cpp/ServiceFramework.h"

#include "common/config/Flags.h"

#include <signal.h>

DEFINE_double(interval, 1.0, "number of seconds between statistics output");
DEFINE_bool(enable_service_framework, false,
            "Run ServiceFramework to track client stats");

using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::test;

int main(int argc, char* argv[]) {
  facebook::config::Flags::initFlags(&argc, &argv, true);

  signal(SIGINT, exit);

  // Optionally run under ServiceFramework to enable testing stats handlers
  // Once ServiceFramework is running, it will hook any new thrift clients that
  // get created, and attach a collect/report their fb303 stats.
  std::unique_ptr<facebook::services::ServiceFramework> fwk;
  if (FLAGS_enable_service_framework) {
    fwk.reset(new facebook::services::ServiceFramework("loadgen"));
    fwk->go(false /* waitUntilStop */);
  }

  if (argc != 1) {
    fprintf(stderr, "error: unhandled arguments:");
    for (int n = 1; n < argc; ++n) {
      fprintf(stderr, " %s", argv[n]);
    }
    fprintf(stderr, "\n");
    return 1;
  }

  // Use the current time to seed the RNGs
  // Not a great source of randomness, but good enough for load testing
  loadgen::RNG::setGlobalSeed(concurrency::Util::currentTimeUsec());

  std::shared_ptr<ClientLoadConfig> config(new ClientLoadConfig);
  if (config->useAsync()) {
    if (config->useCpp2()) {
      loadgen::runLoadGen<apache::thrift::AsyncClientWorker2>(
        config, FLAGS_interval);
    } else {
      loadgen::runLoadGen<AsyncClientWorker>(config, FLAGS_interval);
    }
  } else {
    if (config->useCpp2()) {
      loadgen::runLoadGen<ClientWorker2>(config, FLAGS_interval);
    } else {
      loadgen::runLoadGen<ClientWorker>(config, FLAGS_interval);
    }
  }

  fwk->stop();

  return 0;
}
