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
#include <thrift/lib/cpp/util/TNonblockingServerCreator.h>

using std::shared_ptr;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::server;

namespace apache { namespace thrift { namespace util {

const size_t TNonblockingServerCreator::DEFAULT_NUM_TASK_THREADS;
const size_t TNonblockingServerCreator::DEFAULT_NUM_IO_THREADS;
const bool TNonblockingServerCreator::DEFAULT_HI_PRI_IO_THREADS;
const size_t TNonblockingServerCreator::DEFAULT_CONN_STACK_LIMIT;
const size_t TNonblockingServerCreator::DEFAULT_MAX_CONNECTIONS;
const size_t TNonblockingServerCreator::DEFAULT_MAX_ACTIVE_PROCESSORS;
const size_t TNonblockingServerCreator::DEFAULT_MAX_FRAME_SIZE;
const double TNonblockingServerCreator::DEFAULT_HYSTERESIS_FRACTION = 0.8;
const TOverloadAction TNonblockingServerCreator::DEFAULT_OVERLOAD_ACTION;
const int64_t TNonblockingServerCreator::DEFAULT_TASK_EXPIRE_TIME;
const size_t TNonblockingServerCreator::DEFAULT_WRITE_BUFFER_SIZE;
const size_t TNonblockingServerCreator::DEFAULT_IDLE_READ_BUF_LIMIT;
const size_t TNonblockingServerCreator::DEFAULT_IDLE_WRITE_BUF_LIMIT;
const int TNonblockingServerCreator::DEFAULT_RESIZE_EVERY_N;

TNonblockingServerCreator::TNonblockingServerCreator(
    const shared_ptr<TProcessor>& processor,
    uint16_t port,
    size_t numTaskThreads)
  : processor_(processor)
  , port_(port)
  , numTaskThreads_(numTaskThreads)
  , numIOThreads_(DEFAULT_NUM_IO_THREADS)
  , useHiPriIOThreads_(DEFAULT_HI_PRI_IO_THREADS)
  , connectionStackLimit_(DEFAULT_CONN_STACK_LIMIT)
  , maxConnections_(DEFAULT_MAX_CONNECTIONS)
  , maxActiveProcessors_(DEFAULT_MAX_ACTIVE_PROCESSORS)
  , maxFrameSize_(DEFAULT_MAX_FRAME_SIZE)
  , hysteresisFraction_(DEFAULT_HYSTERESIS_FRACTION)
  , overloadAction_(DEFAULT_OVERLOAD_ACTION)
  , taskExpireTime_(DEFAULT_TASK_EXPIRE_TIME)
  , defaultWriteBufferSize_(DEFAULT_WRITE_BUFFER_SIZE)
  , idleReadBufferLimit_(DEFAULT_IDLE_READ_BUF_LIMIT)
  , idleWriteBufferLimit_(DEFAULT_IDLE_WRITE_BUF_LIMIT)
  , resizeBufferEveryN_(DEFAULT_RESIZE_EVERY_N) {
}

shared_ptr<TServer> TNonblockingServerCreator::createServer() {
  return createNonblockingServer();
}

shared_ptr<TNonblockingServer>
TNonblockingServerCreator::createNonblockingServer() {
  // Set up the thread manager
  shared_ptr<ThreadManager> threadManager;
  if (numTaskThreads_ > 0) {
    // We don't set a limit on the number of pending tasks.  TNonblockingServer
    // isn't prepared to deal with failures adding new tasks, and it also
    // provides its own separate mechanism to deal with overload.
    threadManager = ThreadManager::newSimpleThreadManager(numTaskThreads_);

    shared_ptr<ThreadFactory> threadFactory;
    if (taskThreadFactory_) {
      threadFactory = taskThreadFactory_;
    } else {
      threadFactory.reset(new PosixThreadFactory);
    }

    threadManager->threadFactory(threadFactory);
    threadManager->start();
  }

  // Create the server
  shared_ptr<TNonblockingServer> server;
  if (duplexProtocolFactory_.get() != nullptr) {
    server.reset(new TNonblockingServer(
                   processor_, duplexProtocolFactory_, port_, threadManager));
  } else {
    server.reset(new TNonblockingServer(
                   processor_, getProtocolFactory(), port_, threadManager));
  }

  // Set tunable parameters
  if (numIOThreads_ > 0) {
    server->setNumIOThreads(numIOThreads_);
  }
  server->setUseHighPriorityIOThreads(useHiPriIOThreads_);
  server->setConnectionStackLimit(connectionStackLimit_);
  server->setMaxConnections(maxConnections_);
  server->setMaxActiveProcessors(maxActiveProcessors_);
  server->setMaxFrameSize(maxFrameSize_);
  server->setOverloadHysteresis(hysteresisFraction_);
  server->setOverloadAction(overloadAction_);
  server->setTaskExpireTime(taskExpireTime_);
  server->setWriteBufferDefaultSize(defaultWriteBufferSize_);
  server->setIdleReadBufferLimit(idleReadBufferLimit_);
  server->setIdleWriteBufferLimit(idleWriteBufferLimit_);
  server->setResizeBufferEveryN(resizeBufferEveryN_);
  server->setSocketOptions(socketOptions_);

  // Let ServerCreatorBase set its parameters
  ServerCreatorBase::configureServer(server);

  return server;
}

}}} // apache::thrift::util
