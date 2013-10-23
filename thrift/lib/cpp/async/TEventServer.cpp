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
#include "thrift/lib/cpp/async/TEventServer.h"

#include "folly/ScopeGuard.h"
#include "thrift/lib/cpp/async/TEventConnection.h"
#include "thrift/lib/cpp/async/TEventWorker.h"
#include "thrift/lib/cpp/async/TEventTask.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"

#include <boost/thread/barrier.hpp>

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

namespace apache { namespace thrift { namespace async {

using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using namespace std;
using std::shared_ptr;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadFactory;

void TEventServer::useExistingSocket(TAsyncServerSocket::UniquePtr socket) {
  if (!socket_) {
    socket_ = std::move(socket);
  }
}

void TEventServer::useExistingSocket(int socket) {
  if (!socket_) {
    socket_.reset(new TAsyncServerSocket());
    socket_->useExistingSocket(socket);
  }
}

std::vector<int> TEventServer::getListenSockets() const {
  return (socket_) ? socket_->getSockets() : std::vector<int>{};
}

int TEventServer::getListenSocket() const {
  if (socket_) {
    std::vector<int> sockets = socket_->getSockets();
    CHECK(sockets.size() == 1);
    return sockets[0];
  }

  return -1;
}

void TEventServer::setupWorkers() {
  uint32_t threadsStarted = 0;

  try {
    // set up a callback for expired tasks
    if (threadManager_ != nullptr) {
      threadManager_->setExpireCallback(std::bind(&TEventServer::expireClose,
                                                 this,
                                                 std::placeholders::_1));
    }

    auto b = std::make_shared<boost::barrier>(nWorkers_ + 1);

    // Create the worker threads.
    workers_.reserve(nWorkers_);
    for (uint32_t n = 0; n < nWorkers_; ++n) {
      addWorker();
      workers_[n].worker->getEventBase()->runInLoop([b](){
        b->wait();
      });
    }

    for (auto& worker: workers_) {
      worker.thread->start();
      ++threadsStarted;
    }

    // Wait for all workers to start
    b->wait();
  } catch (...) {
    // XXX: TEventWorker::acceptStopped() calls stopConsuming() and
    //      eventBase_.terminateLoopSoon().  Normally this stops the worker from
    //      processing more work and stops the event loop.  However, if
    //      startConsuming() and eventBase_.loop() haven't run yet this won't
    //      do the right thing. The worker thread will still continue to
    //      call startConsuming() later, and will then start the event loop.
    for (uint32_t i = 0; i < threadsStarted; ++i) {
      workers_[i].worker->acceptStopped();
      workers_[i].thread->join();
    }
    workers_.clear();

    throw;
  }
}

void TEventServer::setupServerSocket() {
  bool eventBaseAttached = false;

  try {
    // We check for write success so we don't need or want SIGPIPEs.
    signal(SIGPIPE, sigNoOp);

    // bind to the socket
    if (!socket_) {
      socket_.reset(new TAsyncServerSocket());
      socket_->bind(port_);
    }

    socket_->listen(listenBacklog_);
    socket_->setMaxNumMessagesInQueue(maxNumMsgsInPipe_);
    socket_->setAcceptRateAdjustSpeed(acceptRateAdjustSpeed_);

    // Notify handler of the preServe event
    if (eventHandler_ != nullptr) {
      TSocketAddress address;
      socket_->getAddress(&address);
      eventHandler_->preServe(&address);
    }

    // Add workers as an accept callback
    for (auto& info: workers_) {
      socket_->addAcceptCallback(info.worker.get(),
          info.worker->getEventBase());
    }

    socket_->attachEventBase(getEventBase());
    eventBaseAttached = true;
    socket_->startAccepting();
  } catch (...) {
    if (socket_) {
      if (eventBaseAttached) {
        socket_->detachEventBase();
      }

      socket_.reset();
    }

    throw;
  }
}

void TEventServer::setup() {
  setupWorkers();
  setupServerSocket();
}

/**
 * Loop and accept incoming connections.
 */
void TEventServer::serve() {
  setup();
  SCOPE_EXIT { this->cleanUp(); };
  serveEventBase_ = getEventBase();
  serveEventBase_->loop();
}

void TEventServer::cleanUp() {
  // It is users duty to make sure that setup() call
  // should have returned before doing this cleanup
  serveEventBase_ = nullptr;
  stopListening();

  // wait on the worker threads to actually stop
  for (auto& worker: workers_) {
    worker.thread->join();
  }
  workers_.clear();
}

uint64_t TEventServer::getNumDroppedConnections() const {
  return socket_->getNumDroppedConnections();
}

void TEventServer::expireClose(
               std::shared_ptr<apache::thrift::concurrency::Runnable> task) {
  TEventTask *taskp = static_cast<TEventTask*>(task.get());
  TEventConnection* connection = taskp->getConnection();
  assert(connection && connection->getWorker());
  // We're only going to be called while TEventConnection is awaiting task
  // completion; the cleanup() call will just set a flag that will cause
  // the connection to self-destruct after task completion notification is
  // received and the processor exits.
  connection->cleanup();
  connection->notifyCompletion(TaskCompletionMessage(connection));
}

void TEventServer::stop() {
  // TODO: We really need a memory fence or some locking here to ensure that
  // the compiler doesn't optimize out eventBase.  In practice, most users will
  // only call stop() when the server is actually serving, so this shouldn't be
  // much of an issue.
  TEventBase* eventBase = serveEventBase_;
  if (eventBase) {
    eventBase->terminateLoopSoon();
  }
}

void TEventServer::stopListening() {
  if (socket_) {
    // Stop accepting new connections
    socket_->pauseAccepting();
    socket_->detachEventBase();

    // Wait for any tasks currently running on the task queue workers to finish,
    // then stop the task queue workers. Have to do this now, so there aren't
    // tasks completeing and trying to write to i/o thread workers after we've
    // stopped the i/o workers.
    if (threadManager_) {
      threadManager_->join();
    }

    // Free the listening socket. This will close it, which will in turn cause
    // the i/o workers to stop.
    socket_.reset();
  }
}

void TEventServer::addWorker() {
  // Create the TEventWorker
  WorkerInfo info;
  uint32_t workerID = workers_.size();
  info.worker.reset(new TEventWorker(asyncProcessorFactory_,
                                     duplexProtocolFactory_,
                                     this,
                                     workerID));
  info.worker->setMaxNumActiveConnections(maxNumActiveConnectionsPerWorker_);

  // Create the thread
  info.thread = threadFactory_->newThread(info.worker, ThreadFactory::ATTACHED);

  workers_.push_back(info);
}

TConnectionContext* TEventServer::getConnectionContext() const {
  TEventConnection *currConn = currentConnection_.get();
  return currConn == nullptr
      ? nullptr
      : currConn->getConnectionContext();
}

}}} // apache::thrift::async
