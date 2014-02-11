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


#include "thrift/lib/cpp2/server/Cpp2Worker.h"

#include "thrift/lib/cpp2/server/Cpp2Connection.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"
#include "thrift/lib/cpp/async/TAsyncSSLSocket.h"
#include "thrift/lib/cpp/concurrency/Util.h"


#include <iostream>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include <glog/logging.h>

#include "folly/String.h"

DEFINE_int32(pending_interval, 10, "Pending count interval in ms");

namespace apache { namespace thrift {

using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using std::shared_ptr;
using apache::thrift::concurrency::Util;

/**
 * Creates a new connection either by reusing an object off the stack or
 * by allocating a new one entirely
 */
std::shared_ptr<Cpp2Connection> Cpp2Worker::createConnection(
  std::shared_ptr<TAsyncSocket> asyncSocket,
  const TSocketAddress* addr) {
  VLOG(4) << "Cpp2Worker: Creating connection for socket " <<
    asyncSocket->getFd();

  std::shared_ptr<Cpp2Connection> result(
    new Cpp2Connection(asyncSocket, addr, this));
  activeConnections_.insert(result);

  scheduleIdleConnectionTimeout(result.get());

  VLOG(4) << "created connection for fd " << asyncSocket->getFd();
  return result;
}

void Cpp2Worker::scheduleTimeout(
    apache::thrift::async::HHWheelTimer::Callback* callback,
    std::chrono::milliseconds timeout) {
  timer_->scheduleTimeout(callback, timeout);
}

void Cpp2Worker::scheduleIdleConnectionTimeout(Cpp2Connection* con) {
  if (server_->getIdleTimeout() > std::chrono::milliseconds(0)) {
    scheduleTimeout(con, server_->getIdleTimeout());
  }
}

void Cpp2Worker::connectionAccepted(int fd, const TSocketAddress& clientAddr)
  noexcept {
  TAsyncSocket *asyncSock = nullptr;
  TAsyncSSLSocket *sslSock = nullptr;
  auto observer = server_->getObserver();

  if (server_->maxConnections_ > 0 &&
      (activeConnections_.size() >=
       server_->maxConnections_ / server_->nWorkers_) ) {
    if (observer) {
      observer->connDropped();
    }

    close(fd);
    return;
  }

  if (server_->getSSLContext()) {
    sslSock = new TAsyncSSLSocket(server_->getSSLContext(), &eventBase_, fd,
                                  true);
    asyncSock = sslSock;
  } else {
    asyncSock = new TAsyncSocket(&eventBase_, fd);
  }
  asyncSock->setShutdownSocketSet(server_->shutdownSocketSet_.get());

  if (sslSock != nullptr) {
    // The connection may be deleted in sslAccept().
    sslSock->sslAccept(this, server_->getIdleTimeout().count());
  } else {
    finishConnectionAccepted(asyncSock);
  }

  if (observer) {
    observer->connAccepted();
  }
  VLOG(4) << "accepted connection for fd " << fd;
}

void Cpp2Worker::handshakeSuccess(TAsyncSSLSocket *sock)
  noexcept {
  VLOG(4) << "Handshake succeeded";
  finishConnectionAccepted(sock);
}

void Cpp2Worker::handshakeError(TAsyncSSLSocket *sock,
                                  const TTransportException& ex)
  noexcept {
  VLOG(1) << "Cpp2Worker: SSL handshake failed: " << folly::exceptionStr(ex);
  sock->destroy();
}


void Cpp2Worker::finishConnectionAccepted(TAsyncSocket *asyncSocket) {
  // Create a new Cpp2Connection for this client socket.
  std::shared_ptr<Cpp2Connection> clientConnection;
  std::shared_ptr<TAsyncSocket> asyncSocketPtr(
    asyncSocket, TDelayedDestruction::Destructor());
  try {
    TSocketAddress clientAddr;
    asyncSocketPtr->getPeerAddress(&clientAddr);
    clientConnection = createConnection(asyncSocketPtr, &clientAddr);
  } catch (...) {
    // Fail fast if we could not create a Cpp2Connection object
    LOG(ERROR) << "Cpp2Worker: failed to create a new Cpp2Connection: "
      << folly::exceptionStr(std::current_exception());
    return;
  }

  // begin i/o on connection
  clientConnection->start();
}

void Cpp2Worker::acceptError(const std::exception& ex) noexcept {
  // We just log an error message if an accept error occurs.
  // Most accept errors are transient (e.g., out of file descriptors), so we
  // will continue trying to accept new connections.
  LOG(WARNING) << "Cpp2Worker: error accepting connection: "
    << folly::exceptionStr(ex);
}

void Cpp2Worker::acceptStopped() noexcept {
  if (server_->getStopWorkersOnStopListening()) {
    stopEventBase();
  }
}

void Cpp2Worker::stopEventBase() noexcept {
  eventBase_.terminateLoopSoon();
}

/**
 * All the work gets done here via callbacks to acceptConnections() and
 * to the handler functions in TAsyncSocket
 */
void Cpp2Worker::serve() {
  try {
    // Inform the TEventBaseManager that our TEventBase will be used
    // for this thread.  This relies on the fact that Cpp2Worker always
    // starts in a brand new thread, so nothing else has tried to use the
    // TEventBaseManager to get an event base for this thread yet.
    server_->getEventBaseManager()->setEventBase(&eventBase_, false);

    // Set up HHWheelTimer. It manages connection idle timeout as well as
    // request timeout.
    timer_.reset(new HHWheelTimer(&eventBase_));

    // No events are registered by default, loopForever.
    eventBase_.loopForever();

    // Inform the TEventBaseManager that our TEventBase is no longer valid.
    // This prevents iterations over the manager's TEventBases from
    // including this one, which will soon be destructed.
    server_->getEventBaseManager()->clearEventBase();
  } catch (TException& tx) {
    LOG(ERROR) << "Cpp2Worker::serve: " << folly::exceptionStr(tx);
  }
}

void Cpp2Worker::closeConnection(
  std::shared_ptr<Cpp2Connection> connection) {
  activeConnections_.erase(connection);
  connection->stop();
}

void Cpp2Worker::closeConnections() {
  for (auto& connection : activeConnections_) {
    connection->stop();
  }
  activeConnections_.clear();
}

Cpp2Worker::~Cpp2Worker() {
  closeConnections();
  eventBase_.terminateLoopSoon();
}

int Cpp2Worker::pendingCount() {
  auto now = std::chrono::steady_clock::now();

  // Only recalculate once every pending_interval
  if (pendingTime_ < now) {
    pendingTime_ = now + std::chrono::milliseconds(FLAGS_pending_interval);
    pendingCount_ = 0;
    for (auto& connection : activeConnections_) {
      if (connection->pending()) {
        pendingCount_++;
      }
    }
  }
  return pendingCount_;
}

int Cpp2Worker::getPendingCount() {
  return pendingCount_;
}

}} // apache::thrift
