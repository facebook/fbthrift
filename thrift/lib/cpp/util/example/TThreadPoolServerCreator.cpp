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
#include "thrift/lib/cpp/util/example/TThreadPoolServerCreator.h"

#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/server/example/TThreadPoolServer.h"
#include "thrift/lib/cpp/transport/TServerSocket.h"

using std::shared_ptr;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;

namespace apache { namespace thrift { namespace util {

const TThreadPoolServerCreator::OverloadAction
  TThreadPoolServerCreator::DEFAULT_OVERLOAD_ACTION;
const size_t TThreadPoolServerCreator::DEFAULT_NUM_THREADS;
const size_t TThreadPoolServerCreator::DEFAULT_MAX_PENDING_TASKS;

TThreadPoolServerCreator::TThreadPoolServerCreator(
    const shared_ptr<TProcessor>& processor,
    uint16_t port,
    bool framed)
  : SyncServerCreator(processor, port, framed)
  , overloadAction_(DEFAULT_OVERLOAD_ACTION)
  , numThreads_(DEFAULT_NUM_THREADS)
  , maxPendingTasks_(0) {
}

TThreadPoolServerCreator::TThreadPoolServerCreator(
    const shared_ptr<TProcessor>& processor,
    uint16_t port,
    int numThreads,
    bool framed)
  : SyncServerCreator(processor, port, framed)
  , overloadAction_(DEFAULT_OVERLOAD_ACTION)
  , numThreads_(numThreads)
  , maxPendingTasks_(0) {
}

TThreadPoolServerCreator::TThreadPoolServerCreator(
    const shared_ptr<TProcessor>& processor,
    uint16_t port,
    shared_ptr<TTransportFactory>& transportFactory,
    shared_ptr<TProtocolFactory>& protocolFactory)
  : SyncServerCreator(processor, port, transportFactory, protocolFactory)
  , overloadAction_(DEFAULT_OVERLOAD_ACTION)
  , numThreads_(DEFAULT_NUM_THREADS)
  , maxPendingTasks_(0) {
}

void TThreadPoolServerCreator::setThreadFactory(
    const shared_ptr<ThreadFactory>& threadFactory) {
  threadFactory_ = threadFactory;
}

shared_ptr<TServer> TThreadPoolServerCreator::createServer() {
  return createThreadPoolServer();
}

// "Deprecated factory for TThreadPoolServer"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
shared_ptr<TThreadPoolServer>
TThreadPoolServerCreator::createThreadPoolServer() {
  // Set up the thread factory and thread manager
  shared_ptr<ThreadManager> threadManager =
    ThreadManager::newSimpleThreadManager(numThreads_, maxPendingTasks_);

  shared_ptr<ThreadFactory> threadFactory;
  if (threadFactory_) {
    threadFactory = threadFactory_;
  } else {
    threadFactory.reset(new PosixThreadFactory);
  }

  threadManager->threadFactory(threadFactory);
  threadManager->start();

  shared_ptr<TThreadPoolServer> server;
  if (duplexProtocolFactory_.get() != nullptr) {
    server.reset(new TThreadPoolServer(
                   processor_, createServerSocket(),
                   getDuplexTransportFactory(),
                   duplexProtocolFactory_, threadManager));
  } else {
    server.reset(new TThreadPoolServer(
                   processor_, createServerSocket(), transportFactory_,
                   getProtocolFactory(), threadManager));
  }

  // The TThreadPoolServer timeout is not a very good knob to use for
  // controlling overload behavior.  I probably should fix it, but for now I'm
  // just wrapping it to make TThreadPoolServerCreator expose a slightly nicer
  // interface.
  //
  // If the ThreadManager has a pendingTaskCountMax() value set, the timeout
  // affects how the accept thread behaves when a new connection is accepted
  // and the pending task queue is full.
  //
  // - If it is negative or 0, the connection will be closed immediately after
  //   being accepted()
  //
  // - If it is a positive number, the accept thread will block for up to this
  //   many milliseconds after accept() returns.
  //
  // Using small positive numbers doesn't make much sense, since the timeouts
  // are potentially cumulative.  If 3 connections come in together, the main
  // accept thread will accept() and get the first one, wait for a timeout
  // interval, then accept() the second one and wait for the timeout interval,
  // then accept the third.  The end result is that the first connection gets
  // closed after 1 timeout interval, the second after 2 timeout intervals, the
  // third after 3 timeout intervals, etc.
  //
  // We allow the timeout to either be set to 0, which means close immediately,
  // or a very large value, which essentially means stop calling accept() and
  // let the kernel's TCP backlog deal with it.
  if (overloadAction_ == OVERLOAD_ACCEPT_AND_CLOSE) {
    server->setTimeout(0);
  } else {
    // Set the timeout to a very large value.
    // Don't use too big a value, though, because the ThreadManager code just
    // adds it to the current time and doesn't check for overflow, so very
    // large values are the same as a 0 timeout.
    server->setTimeout(24*60*60*1000);
  }

  SyncServerCreator::configureServer(server);
  return server;
}
#pragma GCC diagnostic pop

}}} // apache::thrift::util
