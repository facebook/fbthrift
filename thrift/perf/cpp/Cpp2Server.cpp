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

#include <folly/Random.h>
#include <folly/String.h>
#include <folly/ssl/Init.h>

#include <thrift/lib/cpp/transport/TSSLSocket.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/perf/cpp/AsyncLoadHandler2.h>

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
DEFINE_int64(
    num_ssl_handshake_threads,
    0,
    "number of ssl handshake threads. Default is 0, which indicates no "
    "additional threads will be spawned to handle SSL handshakes; "
    "handshakes will instead be performed in the acceptor thread");
DEFINE_int32(max_conn_pool_size, 0, "maximum size of idle connection pool");
DEFINE_int32(idle_timeout, 0, "idle timeout (in milliseconds)");
DEFINE_int32(task_timeout, 0, "task timeout (in milliseconds)");
DEFINE_int32(
    handshake_timeout,
    0,
    "SSL handshake timeout (in milliseconds). "
    "Default is 0, which is to not set a granular timeout for SSL handshakes. "
    "Connections that stall during handshakes may still be timed out "
    "with --idle_timeout");
DEFINE_int32(max_connections, 0, "DEPRECATED (REMOVE ME)");
DEFINE_int32(max_requests, 0, "max active requests");
DEFINE_string(cert, "", "server SSL certificate file");
DEFINE_string(key, "", "server SSL private key file");
DEFINE_string(client_ca_list, "", "file pointing to a client CA or list");
DEFINE_string(ticket_seeds, "", "server Ticket seeds file");
DEFINE_bool(queue_sends, true, "Queue sends for better throughput");
DEFINE_string(ecc_curve, "prime256v1",
    "The ECC curve to use for EC handshakes");
DEFINE_bool(enable_tfo, true, "Enable TFO");
DEFINE_int32(tfo_queue_size, 1000, "TFO queue size");

void setTunables(ThriftServer* server) {
  if (FLAGS_idle_timeout > 0) {
    server->setIdleTimeout(std::chrono::milliseconds(FLAGS_idle_timeout));
  }
  if (FLAGS_task_timeout > 0) {
    server->setTaskExpireTime(std::chrono::milliseconds(FLAGS_task_timeout));
  }
  if (FLAGS_handshake_timeout > 0) {
    server->setSSLHandshakeTimeout(
        std::chrono::milliseconds(FLAGS_handshake_timeout));
  }
}

ThriftServer *g_server = nullptr;

[[noreturn]] void sigHandler(int /* signo */) {
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

  auto handler = std::make_shared<AsyncLoadHandler2>();

  folly::ssl::setLockTypes({
#ifdef CRYPTO_LOCK_EVP_PKEY
      {CRYPTO_LOCK_EVP_PKEY, folly::ssl::LockType::NONE},
#endif
#ifdef CRYPTO_LOCK_SSL_SESSION
      {CRYPTO_LOCK_SSL_SESSION, folly::ssl::LockType::SPINLOCK},
#endif
#ifdef CRYPTO_LOCK_SSL_CTX
      {CRYPTO_LOCK_SSL_CTX, folly::ssl::LockType::NONE},
#endif
#ifdef CRYPTO_LOCK_ERR
      {CRYPTO_LOCK_ERR, folly::ssl::LockType::SPINLOCK}
#endif
  });

  std::shared_ptr<ThriftServer> server;
  server.reset(new ThriftServer());
  server->setInterface(handler);
  server->setPort(FLAGS_port);
  server->setNumIOWorkerThreads(FLAGS_num_threads);
  server->setNumCPUWorkerThreads(FLAGS_num_queue_threads);
  server->setNumSSLHandshakeWorkerThreads(FLAGS_num_ssl_handshake_threads);
  server->setMaxRequests(FLAGS_max_requests);
  server->setQueueSends(FLAGS_queue_sends);
  server->setFastOpenOptions(FLAGS_enable_tfo, FLAGS_tfo_queue_size);

  if (FLAGS_cert.length() > 0 && FLAGS_key.length() > 0) {
    auto sslContext = std::make_shared<wangle::SSLContextConfig>();
    sslContext->setCertificate(FLAGS_cert, FLAGS_key, "");
    sslContext->clientCAFile = FLAGS_client_ca_list;
    sslContext->eccCurveName = FLAGS_ecc_curve;
    server->setSSLConfig(sslContext);
    server->watchCertForChanges(FLAGS_cert);
  }
  if (!FLAGS_ticket_seeds.empty()) {
    server->watchTicketPathForChanges(FLAGS_ticket_seeds, true);
  } else {
    // Generate random seeds to use for all workers.  If no seeds are set, then
    // each worker gets its own random seeds, so session resumptions fail across
    // workers.
    wangle::TLSTicketKeySeeds seeds;
    for (auto* seed : {&seeds.oldSeeds, &seeds.currentSeeds, &seeds.newSeeds}) {
      auto randomData = folly::Random::secureRandom<uint64_t>();
      auto asHex = folly::hexlify(folly::ByteRange(
          (const unsigned char*)&randomData, sizeof(uint64_t)));
      seed->push_back(std::move(asHex));
    }
    server->setTicketSeeds(std::move(seeds));
  }

  // Set tunable server parameters
  setTunables(server.get());

  g_server = server.get();
  signal(SIGINT, sigHandler);

  cout << "Serving requests on port " << FLAGS_port << "...\n";
  // Optionally run under ServiceFramework to enable testing stats handlers
  // Once ServiceFramework is running, it will hook any new thrift services that
  // get created, and attach a collect/report their fb303 stats.
  std::unique_ptr<facebook::services::ServiceFramework> fwk;
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
