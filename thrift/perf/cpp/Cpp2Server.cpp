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
#include "thrift/perf/cpp/AsyncLoadHandler2.h"
#include "thrift/perf/cpp/LoadHandler.h"

#include "thrift/lib/cpp2/server/ThriftServer.h"

#include "thrift/lib/cpp/async/TSyncToAsyncProcessor.h"
#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/protocol/THeaderProtocol.h"
#include "thrift/lib/cpp/transport/TSSLSocket.h"

#include "common/init/Init.h"
#include "common/services/cpp/ServiceFramework.h"

#include <iostream>
#include <signal.h>

using std::cout;
using namespace boost;
using namespace apache::thrift::async;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;

DEFINE_bool(enable_service_framework, false,
            "Run ServiceFramework to track service stats");
DEFINE_int32(port, 1234, "server port");
DEFINE_int64(num_threads, 4, "number of worker threads");
DEFINE_int64(num_queue_threads, 4, "number of task queue threads");
DEFINE_int32(max_conn_pool_size, 0, "maximum size of idle connection pool");
DEFINE_int32(idle_timeout, 0, "idle timeout (in milliseconds)");
DEFINE_int32(task_timeout, 0, "task timeout (in milliseconds)");
DEFINE_int32(max_connections, 0, "max active connections");
DEFINE_int32(max_requests, 0, "max active requests");
DEFINE_string(cert, "", "SSL certificate file");
DEFINE_string(key, "", "SSL private key file");
DEFINE_bool(queue_sends, true, "Queue sends for better throughput");

void setTunables(ThriftServer* server) {
  if (FLAGS_idle_timeout > 0) {
    server->setIdleTimeout(std::chrono::milliseconds(FLAGS_idle_timeout));
  }
  if (FLAGS_task_timeout > 0) {
    server->setTaskExpireTime(std::chrono::milliseconds(FLAGS_task_timeout));
  }
}

ThriftServer *g_server = nullptr;

void sigHandler(int signo) {
  g_server->stop();
  exit(0);
}

int main(int argc, char* argv[]) {
  facebook::initFacebook(&argc, &argv);

  if (argc != 1) {
    fprintf(stderr, "error: unhandled arguments:");
    for (int n = 1; n < argc; ++n) {
      fprintf(stderr, " %s", argv[n]);
    }
    fprintf(stderr, "\n");
    return 1;
  }

  // Optionally run under ServiceFramework to enable testing stats handlers
  // Once ServiceFramework is running, it will hook any new thrift services that
  // get created, and attach a collect/report their fb303 stats.
  std::unique_ptr<facebook::services::ServiceFramework> fwk;
  if (FLAGS_enable_service_framework) {
    fwk.reset(
        new facebook::services::ServiceFramework("ThriftServer Load Tester"));
    fwk->go(false /* waitUntilStop */);
  }

  auto handler = std::make_shared<AsyncLoadHandler2>();

  std::shared_ptr<ThriftServer> server;
  server.reset(new ThriftServer());
  server->setInterface(handler);
  server->setPort(FLAGS_port);
  server->setNWorkerThreads(FLAGS_num_threads);
  server->setNPoolThreads(FLAGS_num_queue_threads);
  server->setMaxConnections(FLAGS_max_connections);
  server->setMaxRequests(FLAGS_max_requests);
  server->setQueueSends(FLAGS_queue_sends);

  if (FLAGS_cert.length() > 0 && FLAGS_key.length() > 0) {
    std::shared_ptr<SSLContext> sslContext(new SSLContext());
    sslContext->loadCertificate(FLAGS_cert.c_str());
    sslContext->loadPrivateKey(FLAGS_key.c_str());
    server->setSSLContext(sslContext);
  }

  // Set tunable server parameters
  setTunables(server.get());

  g_server = server.get();
  signal(SIGINT, sigHandler);

  cout << "Serving requests on port " << FLAGS_port << "...\n";
  if (FLAGS_enable_service_framework) {
    fwk.reset(
        new facebook::services::ServiceFramework("ThriftServer Load Tester"));
    fwk->addThriftService(server, handler.get(), FLAGS_port);
    fwk->go();
  } else {
    server->serve();
  }

  cout << "Exiting normally" << std::endl;
  fwk->stop();

  return 0;
}
