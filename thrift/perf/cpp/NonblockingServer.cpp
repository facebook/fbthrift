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
#include "thrift/perf/cpp/LoadHandler.h"

#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/protocol/THeaderProtocol.h"
#include "thrift/lib/cpp/server/TNonblockingServer.h"

#include "common/config/Flags.h"
#include <iostream>
#include <signal.h>

using namespace boost;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;

DEFINE_int32(port, 1234, "server port");
DEFINE_bool(header, false, "use THeaderProtocol");

DEFINE_int64(num_task_threads, 50,
             "number of task threads (0 causes tasks to be run in the "
             "I/O threads)");
DEFINE_int64(num_io_threads, 0, "number of I/O threads");
DEFINE_int64(max_pending_tasks, 0, "max pending tasks");
DEFINE_bool(hi_pri_io, false, "use high priority I/O threads");
DEFINE_int64(connection_stack_limit, 0,
             "maximum size of idle TConnection stack");
DEFINE_int64(max_connections, 0, "maximum number of connections");
DEFINE_int64(max_active_processors, 0, "maximum number of active processors");
DEFINE_int64(max_active_requests, 0, "maximum number of active requests");
DEFINE_double(overload_hysteresis, 0.0, "overload hysteresis fraction");
DEFINE_string(overload_action, "", "overload hysteresis action");
DEFINE_int64(task_expire_time, 0, "task expire time");
DEFINE_int64(write_bufsize_default, 0, "default write buffer size");
DEFINE_int64(idle_read_buf_limit, 0, "idle read buffer limit");
DEFINE_int64(idle_write_buf_limit, 0, "idle write buffer limit");
DEFINE_int64(idle_buf_mem_limit, 0, "idle buffer memory limit");
DEFINE_int32(resize_buf_every_n, 0, "resize buffer every N requests");

bool setTunables(TNonblockingServer* server) {
  if (FLAGS_num_io_threads > 0) {
    server->setNumIOThreads(FLAGS_num_io_threads);
  }
  if (FLAGS_hi_pri_io) {
    server->setUseHighPriorityIOThreads(true);
  }

  if (FLAGS_connection_stack_limit > 0) {
    server->setConnectionStackLimit(FLAGS_connection_stack_limit);
  }
  if (FLAGS_max_connections > 0) {
    server->setMaxConnections(FLAGS_max_connections);
  }
  if (FLAGS_max_active_processors > 0) {
    server->setMaxActiveProcessors(FLAGS_max_active_processors);
  }
  if (FLAGS_max_active_requests > 0) {
    server->setMaxActiveRequests(FLAGS_max_active_requests);
  }

  if (FLAGS_overload_hysteresis > 0) {
    server->setOverloadHysteresis(FLAGS_overload_hysteresis);
  }
  if (FLAGS_overload_action == "none" || FLAGS_overload_action == "") {
    server->setOverloadAction(T_OVERLOAD_NO_ACTION);
  } else if (FLAGS_overload_action == "close") {
    server->setOverloadAction(T_OVERLOAD_CLOSE_ON_ACCEPT);
  } else if (FLAGS_overload_action == "drain") {
    server->setOverloadAction(T_OVERLOAD_DRAIN_TASK_QUEUE);
  } else if (FLAGS_overload_action != "") {
    std::cerr << "unknown overload action \"" << FLAGS_overload_action <<
      "\"\n";
    std::cerr << "pick one of: \"none\", \"close\", \"drain\"\n";
    return false;
  }

  if (FLAGS_task_expire_time > 0) {
    server->setTaskExpireTime(FLAGS_task_expire_time);
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
  if (FLAGS_idle_buf_mem_limit > 0) {
    server->setIdleBufferMemLimit(FLAGS_idle_buf_mem_limit);
  }
  if (FLAGS_resize_buf_every_n > 0) {
    server->setResizeBufferEveryN(FLAGS_resize_buf_every_n);
  }

  return true;
}

int main(int argc, char* argv[]) {
  facebook::config::Flags::initFlags(&argc, &argv, true);

  signal(SIGINT, exit);

  if (argc != 1) {
    fprintf(stderr, "error: unhandled arguments:");
    for (int n = 1; n < argc; ++n) {
      fprintf(stderr, " %s", argv[n]);
    }
    fprintf(stderr, "\n");
    return 1;
  }

  std::shared_ptr<LoadHandler> handler(new LoadHandler);

  typedef TBinaryProtocolT<TBufferBase> ProtocolType;
  typedef LoadTestProcessorT<ProtocolType> LoadProcessor;
  std::shared_ptr<LoadProcessor> processor(new LoadProcessor(handler));

  std::shared_ptr<TDuplexProtocolFactory> duplexProtocolFactory;
  std::shared_ptr<TProtocolFactory> protocolFactory;
  if (FLAGS_header) {
    std::bitset<CLIENT_TYPES_LEN> clientTypes;
    clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
    clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;
    THeaderProtocolFactory *factory = new THeaderProtocolFactory();
    factory->setClientTypes(clientTypes);
    duplexProtocolFactory = std::shared_ptr<TDuplexProtocolFactory>(factory);
  } else {
    protocolFactory.reset(new TBinaryProtocolFactoryT<TBufferBase>);
  }


  std::shared_ptr<ThreadManager> threadManager;

  if (FLAGS_num_task_threads > 0) {
    threadManager =
      ThreadManager::newSimpleThreadManager(FLAGS_num_task_threads,
                                            FLAGS_max_pending_tasks);
    std::shared_ptr<PosixThreadFactory> threadFactory(new PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();
  }

  scoped_ptr<TNonblockingServer> server;
  if (FLAGS_header) {
    server.reset(new TNonblockingServer(processor, duplexProtocolFactory,
                                        FLAGS_port, threadManager));
  } else {
    server.reset(new TNonblockingServer(processor, protocolFactory, FLAGS_port,
                                        threadManager));
  }

  // Set tunable server parameters
  if (!setTunables(server.get())) {
    return 1;
  }

  std::cout << "Serving requests on port " << FLAGS_port << "...\n";
  server->serve();

  return 0;
}
