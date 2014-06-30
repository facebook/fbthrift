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
#include "thrift/perf/cpp/AsyncLoadHandler.h"
#include "thrift/perf/cpp/LoadHandler.h"

#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/async/TSyncToAsyncProcessor.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/transport/TSSLSocket.h>

#include "common/config/Flags.h"
#include "common/services/cpp/ServiceFramework.h"

#include <iostream>
#include <signal.h>

using std::cout;
using namespace boost;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;

DEFINE_bool(enable_service_framework, false,
            "Run ServiceFramework to track service stats");
DEFINE_int32(port, 1234, "server port");
DEFINE_bool(header, false, "use THeaderProtocol");
DEFINE_int64(num_threads, 4, "number of worker threads");
DEFINE_int64(num_queue_threads, 4, "number of task queue threads");
DEFINE_int32(max_conn_pool_size, 0, "maximum size of idle connection pool");
DEFINE_int32(recv_timeout, 0, "receive timeout (in milliseconds)");
DEFINE_int32(call_timeout, 0, "call timeout (in milliseconds)");
DEFINE_int32(max_frame_size, 0, "maximum frame size allowed");
DEFINE_int64(read_bufsize_default, 0, "default read buffer size");
DEFINE_int64(write_bufsize_default, 0, "default write buffer size");
DEFINE_int64(idle_read_buf_limit, 0, "idle read buffer limit");
DEFINE_int64(idle_write_buf_limit, 0, "idle write buffer limit");
DEFINE_int32(resize_buf_every_n, 0, "resize buffer every N requests");
DEFINE_bool(framed_transport, true, "use framed transport");
DEFINE_bool(task_queue_mode, false, "use task queue mode");
DEFINE_bool(sync_to_async_mode, false, "use a sync to async processor");
DEFINE_string(cert, "", "SSL certificate file");
DEFINE_string(key, "", "SSL private key file");

void setTunables(TEventServer* server) {
  if (FLAGS_max_conn_pool_size > 0) {
    server->setMaxConnectionPoolSize(FLAGS_max_conn_pool_size);
  }

  if (FLAGS_recv_timeout > 0) {
    server->setRecvTimeout(FLAGS_recv_timeout);
  }
  if (FLAGS_call_timeout > 0) {
    server->setCallTimeout(FLAGS_call_timeout);
  }
  if (FLAGS_max_frame_size > 0) {
    server->setMaxFrameSize(FLAGS_max_frame_size);
  }

  if (FLAGS_read_bufsize_default > 0) {
    server->setReadBufferDefaultSize(FLAGS_read_bufsize_default);
  }
  if (FLAGS_write_bufsize_default > 0) {
    server->setWriteBufferDefaultSize(FLAGS_write_bufsize_default);
  }
  if (FLAGS_idle_read_buf_limit > 0) {
    server->setIdleReadBufferLimit(FLAGS_idle_read_buf_limit);
  }
  if (FLAGS_idle_write_buf_limit > 0) {
    server->setIdleWriteBufferLimit(FLAGS_idle_write_buf_limit);
  }
  if (FLAGS_resize_buf_every_n) {
    server->setResizeBufferEveryN(FLAGS_resize_buf_every_n);
  }
}

TEventServer *g_server = nullptr;

void sigHandler(int signo) {
  // graceful stop which allows the profiler to work
  g_server->stop();
}

int main(int argc, char* argv[]) {
  facebook::config::Flags::initFlags(&argc, &argv, true);

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
        new facebook::services::ServiceFramework("EventServer Load Tester"));
    fwk->go(false /* waitUntilStop */);
  }

  std::shared_ptr<AsyncLoadHandler> handler(new AsyncLoadHandler);
  std::shared_ptr<LoadHandler> queueHandler(new LoadHandler);

  typedef TBinaryProtocolT<TBufferBase> ProtocolType;
  typedef LoadTestAsyncProcessorT<ProtocolType> LoadProcessor;
  typedef LoadTestProcessorT<ProtocolType> LoadQueueProcessor;
  typedef LoadTestAsyncProcessorT<THeaderProtocol> HeaderLoadProcessor;
  typedef LoadTestProcessorT<THeaderProtocol> HeaderLoadQueueProcessor;
  std::shared_ptr<TAsyncProcessor> processor;
  std::shared_ptr<LoadQueueProcessor> queueProcessor(
    new LoadQueueProcessor(queueHandler));
  std::shared_ptr<HeaderLoadQueueProcessor> headerQueueProcessor(
    new HeaderLoadQueueProcessor(queueHandler));

  std::shared_ptr<TProtocolFactory> protocolFactory;
  std::shared_ptr<TDuplexProtocolFactory> duplexProtocolFactory;

  if (FLAGS_header) {
    std::bitset<CLIENT_TYPES_LEN> clientTypes;
    clientTypes[THRIFT_UNFRAMED_DEPRECATED] = 1;
    clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
    clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;
    THeaderProtocolFactory *factory = new THeaderProtocolFactory();
    factory->setClientTypes(clientTypes);
    duplexProtocolFactory = std::shared_ptr<TDuplexProtocolFactory>(factory);
    if (FLAGS_sync_to_async_mode) {
      processor.reset(new TSyncToAsyncProcessor(headerQueueProcessor));
    } else {
      processor.reset(new HeaderLoadProcessor(handler));
    }
  } else {
    protocolFactory.reset(new TBinaryProtocolFactoryT<TBufferBase>);
    if (FLAGS_sync_to_async_mode) {
      processor.reset(new TSyncToAsyncProcessor(queueProcessor));
    } else {
      processor.reset(new LoadProcessor(handler));
    }
  }

  scoped_ptr<TEventServer> server;
  if (FLAGS_task_queue_mode) {
    std::shared_ptr<ThreadManager> threadManager(
      ThreadManager::newSimpleThreadManager(
        FLAGS_num_queue_threads));
    std::shared_ptr<PosixThreadFactory> threadFactory(new PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();
    if (FLAGS_header) {
      server.reset(new TEventServer(headerQueueProcessor, duplexProtocolFactory,
                                    FLAGS_port,
                                    threadManager, FLAGS_num_threads));
    } else {
      server.reset(new TEventServer(queueProcessor, protocolFactory, FLAGS_port,
                                    threadManager, FLAGS_num_threads));
    }
  } else {
    if (FLAGS_header) {
      server.reset(new TEventServer(processor, duplexProtocolFactory,
                                    FLAGS_port,
                                    FLAGS_num_threads));
    } else {
      server.reset(new TEventServer(processor, protocolFactory, FLAGS_port,
                                    FLAGS_num_threads));
    }
  }

  if (!FLAGS_framed_transport) {
    server->setTransportType(TEventServer::TransportType::UNFRAMED_BINARY);
  }

  if (FLAGS_cert.length() > 0 && FLAGS_key.length() > 0) {
    std::shared_ptr<SSLContext> sslContext(new SSLContext());
    sslContext->loadCertificate(FLAGS_cert.c_str());
    sslContext->loadPrivateKey(FLAGS_key.c_str());
    server->setSSLContext(sslContext);
  }

  handler->setServer(server.get());

  // Set tunable server parameters
  setTunables(server.get());

  g_server = server.get();
  signal(SIGINT, sigHandler);

  cout << "Serving requests on port " << FLAGS_port << "...\n";
  server->serve();

  cout << "Exiting normally" << std::endl;
  fwk->stop();

  return 0;
}
