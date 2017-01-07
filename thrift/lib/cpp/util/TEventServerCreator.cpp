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
#include <thrift/lib/cpp/util/TEventServerCreator.h>

#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>

using std::shared_ptr;
using apache::thrift::async::TAsyncProcessor;
using apache::thrift::async::TEventServer;
using apache::thrift::server::TServer;

namespace apache { namespace thrift { namespace util {

const size_t TEventServerCreator::DEFAULT_NUM_IO_THREADS;
const size_t TEventServerCreator::DEFAULT_NUM_TASK_THREADS;
const uint32_t TEventServerCreator::DEFAULT_CONN_POOL_SIZE;
constexpr std::chrono::milliseconds TEventServerCreator::DEFAULT_RECV_TIMEOUT;
const size_t TEventServerCreator::DEFAULT_MAX_FRAME_SIZE;
constexpr std::chrono::milliseconds TEventServerCreator::DEFAULT_WORKER_LATENCY;
const size_t TEventServerCreator::DEFAULT_WRITE_BUFFER_SIZE;
const size_t TEventServerCreator::DEFAULT_READ_BUFFER_SIZE;
const size_t TEventServerCreator::DEFAULT_IDLE_READ_BUF_LIMIT;
const size_t TEventServerCreator::DEFAULT_IDLE_WRITE_BUF_LIMIT;
const int TEventServerCreator::DEFAULT_RESIZE_EVERY_N;

TEventServerCreator::TEventServerCreator(
    const shared_ptr<TAsyncProcessor>& asyncProcessor,
    uint16_t port,
    size_t numIoThreads)
  : asyncProcessor_(asyncProcessor)
  , port_(port)
  , numIoThreads_(numIoThreads)
  , maxConnPoolSize_(DEFAULT_CONN_POOL_SIZE)
  , recvTimeout_(DEFAULT_RECV_TIMEOUT)
  , maxFrameSize_(DEFAULT_MAX_FRAME_SIZE)
  , defaultReadBufferSize_(DEFAULT_READ_BUFFER_SIZE)
  , defaultWriteBufferSize_(DEFAULT_WRITE_BUFFER_SIZE)
  , idleReadBufferLimit_(DEFAULT_IDLE_READ_BUF_LIMIT)
  , idleWriteBufferLimit_(DEFAULT_IDLE_WRITE_BUF_LIMIT)
  , resizeBufferEveryN_(DEFAULT_RESIZE_EVERY_N)
  , workerLatencyLimit_(DEFAULT_WORKER_LATENCY)
  , listenBacklog_(DEFAULT_LISTEN_BACKLOG) {
}

TEventServerCreator::TEventServerCreator(
    const shared_ptr<TProcessor>& syncProcessor,
    uint16_t port,
    size_t numIoThreads,
    size_t numTaskThreads)
  : syncProcessor_(syncProcessor)
  , port_(port)
  , numIoThreads_(numIoThreads)
  , numTaskThreads_(numTaskThreads)
  , maxConnPoolSize_(DEFAULT_CONN_POOL_SIZE)
  , recvTimeout_(DEFAULT_RECV_TIMEOUT)
  , maxFrameSize_(DEFAULT_MAX_FRAME_SIZE)
  , defaultReadBufferSize_(DEFAULT_READ_BUFFER_SIZE)
  , defaultWriteBufferSize_(DEFAULT_WRITE_BUFFER_SIZE)
  , idleReadBufferLimit_(DEFAULT_IDLE_READ_BUF_LIMIT)
  , idleWriteBufferLimit_(DEFAULT_IDLE_WRITE_BUF_LIMIT)
  , resizeBufferEveryN_(DEFAULT_RESIZE_EVERY_N)
  , workerLatencyLimit_(DEFAULT_WORKER_LATENCY)
  , listenBacklog_(DEFAULT_LISTEN_BACKLOG) {
}

shared_ptr<TServer> TEventServerCreator::createServer() {
  return createEventServer();
}

shared_ptr<TEventServer>
TEventServerCreator::createEventServer() {
  // Create the server
  shared_ptr<TEventServer> server;

  if (asyncProcessor_) {
    server.reset(new TEventServer(asyncProcessor_,
                                  getDuplexProtocolFactory(),
                                  port_,
                                  numIoThreads_));
  }
  else {
    if (!taskQueueThreadManager_) {
      shared_ptr<concurrency::PosixThreadFactory> threadFactory(
        new concurrency::PosixThreadFactory());
      taskQueueThreadManager_ =
        concurrency::ThreadManager::newSimpleThreadManager(numTaskThreads_);

      taskQueueThreadManager_->threadFactory(threadFactory);
      taskQueueThreadManager_->start();
    }

    server.reset(new TEventServer(syncProcessor_,
                                  getDuplexProtocolFactory(),
                                  port_,
                                  taskQueueThreadManager_,
                                  numIoThreads_));
  }


  // Set tunable parameters
  server->setMaxConnectionPoolSize(maxConnPoolSize_);
  server->setRecvTimeout(recvTimeout_.count());
  server->setMaxFrameSize(maxFrameSize_);
  server->setReadBufferDefaultSize(defaultReadBufferSize_);
  server->setWriteBufferDefaultSize(defaultWriteBufferSize_);
  server->setIdleReadBufferLimit(idleReadBufferLimit_);
  server->setIdleWriteBufferLimit(idleWriteBufferLimit_);
  server->setResizeBufferEveryN(resizeBufferEveryN_);
  server->setCallTimeout(workerLatencyLimit_.count());
  server->setListenBacklog(listenBacklog_);

  // Let ServerCreatorBase set its parameters
  ServerCreatorBase::configureServer(server);

  return server;
}

}}} // apache::thrift::util
