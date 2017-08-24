/*
 * Copyright 2017-present Facebook, Inc.
 *
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
#include <signal.h>
#include <iostream>

#include <folly/String.h>

#include <thrift/lib/cpp/transport/TSSLSocket.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/perf/cpp/AsyncLoadHandler2.h>

#include <thrift/lib/cpp2/transport/http2/server/H2ThriftServer.h>
#include "common/init/Init.h"
#include "common/services/cpp/ServiceFramework.h"

using std::cout;
using namespace boost;
using namespace apache::thrift::async;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;

DEFINE_bool(
    enable_service_framework,
    false,
    "Run ServiceFramework to track service stats");
DEFINE_int32(port, 1234, "server port");
DEFINE_int64(num_threads, 4, "number of worker threads");
DEFINE_int64(num_queue_threads, 4, "number of task queue threads");
DEFINE_int32(max_conn_pool_size, 0, "maximum size of idle connection pool");
DEFINE_int32(idle_timeout, 0, "idle timeout (in milliseconds)");
DEFINE_int32(task_timeout, 0, "task timeout (in milliseconds)");
DEFINE_int32(max_connections, 0, "max active connections");
DEFINE_int32(max_requests, 0, "max active requests");
DEFINE_string(cert, "", "server SSL certificate file");
DEFINE_string(key, "", "server SSL private key file");
DEFINE_string(client_ca_list, "", "file pointing to a client CA or list");
DEFINE_string(ticket_seeds, "", "server Ticket seeds file");
DEFINE_bool(queue_sends, true, "Queue sends for better throughput");
DEFINE_string(
    ecc_curve,
    "prime256v1",
    "The ECC curve to use for EC handshakes");
DEFINE_bool(enable_tfo, true, "Enable TFO");
DEFINE_int32(tfo_queue_size, 1000, "TFO queue size");

void setTunables(H2ThriftServer* server) {
  if (FLAGS_idle_timeout > 0) {
    server->setIdleTimeout(std::chrono::milliseconds(FLAGS_idle_timeout));
  }
  if (FLAGS_task_timeout > 0) {
    server->setTaskExpireTime(std::chrono::milliseconds(FLAGS_task_timeout));
  }
}

H2ThriftServer* g_server = nullptr;

[[noreturn]] void sigHandler(int /* signo */) {
  g_server->stop();
  exit(0);
}

int main(int argc, char* argv[]) {
  facebook::initFacebook(&argc, &argv);
  auto handler = std::make_shared<AsyncLoadHandler2>();

  auto cpp2PFac =
      std::make_shared<ThriftServerAsyncProcessorFactory<AsyncLoadHandler2>>(
          handler);
  auto server = std::make_shared<H2ThriftServer>();
  server->setPort(FLAGS_port);
  server->setProcessorFactory(cpp2PFac);
  server->setNumIOWorkerThreads(FLAGS_num_threads);
  server->setNumCPUWorkerThreads(FLAGS_num_queue_threads);
  server->setMaxConnections(FLAGS_max_connections);
  server->setMaxRequests(FLAGS_max_requests);

  // Set tunable server parameters
  setTunables(server.get());

  g_server = server.get();
  signal(SIGINT, sigHandler);

  cout << "Serving requests on port " << FLAGS_port << "...\n";
  server->serve();

  cout << "Exiting normally" << std::endl;

  return 0;
}
