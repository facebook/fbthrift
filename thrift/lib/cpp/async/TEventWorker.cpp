/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp/async/TEventWorker.h>

#include <folly/portability/Sockets.h>
#include <thrift/lib/cpp/async/TEventConnection.h>
#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TEventTask.h>
#include <thrift/lib/cpp/concurrency/Util.h>

#include <folly/portability/SysUio.h>

#include <iostream>
#include <fcntl.h>

namespace apache { namespace thrift { namespace async {

using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using std::shared_ptr;
using apache::thrift::concurrency::Util;

/**
 * Creates a new connection either by reusing an object off the stack or
 * by allocating a new one entirely
 */
TEventConnection* TEventWorker::createConnection(
    shared_ptr<TAsyncSocket> asyncSocket,
    const folly::SocketAddress* addr) {
  T_DEBUG_T("TEventWorker: Creating connection for socket %d",
            asyncSocket->getFd());

  TEventConnection* result;
  // Check the stack
  if (connectionStack_.empty()) {
    result = new TEventConnection(asyncSocket, addr, this, transportType_);
  } else {
    result = connectionStack_.top();
    connectionStack_.pop();
    result->init(asyncSocket, addr, this, transportType_);
  }
  activeConnectionList_.push_front(result);
  activeConnectionMap_.insert(make_pair(result,
                                        activeConnectionList_.begin()));

  T_DEBUG_T("created connection %p for fd %d\n", result, asyncSocket->getFd());
  return result;
}

/**
 * Returns a connection to the stack
 */
void TEventWorker::returnConnection(TEventConnection* connection) {
  T_DEBUG_T("return %p TEventConection on clientSocket\n", connection);

  ConnectionMap::iterator it = activeConnectionMap_.find(connection);
  if (it == activeConnectionMap_.end()) {
    // we already deleted it (load shedding)
    T_DEBUG_T("connection %p not found in ConnectionMap", connection);
  } else {
    activeConnectionList_.erase(it->second);
    activeConnectionMap_.erase(it);
  }

  if (connectionStack_.size() < server_->getMaxConnectionPoolSize()) {
    connection->checkBufferMemoryUsage();
    connectionStack_.push(connection);
  }
  else {
    delete connection;
  }
}

void TEventWorker::connectionAccepted(int fd, const folly::SocketAddress& clientAddr)
  noexcept {
  TAsyncSocket *asyncSock = nullptr;
  TAsyncSSLSocket *sslSock = nullptr;
  if (server_->getSSLContext()) {
    sslSock = new TAsyncSSLSocket(server_->getSSLContext(), &eventBase_, fd,
                                  true);
    asyncSock = sslSock;
  } else {
    asyncSock = new TAsyncSocket(&eventBase_, fd);
  }

  if (maxNumActiveConnections_ > 0 &&
      maxNumActiveConnections_ <= activeConnectionList_.size()) {
    T_ERROR("Too many active connections (%ld), max allowed is (%d)",
            activeConnectionList_.size(), maxNumActiveConnections_);
    asyncSock->destroy();
    return;
  }

  if (sslSock != nullptr) {
    // The connection may be deleted in sslAccept().
    sslSock->sslAccept(this, server_->getRecvTimeout());
  } else {
    finishConnectionAccepted(asyncSock);
  }
}

void TEventWorker::handshakeSuccess(TAsyncSSLSocket *sock)
  noexcept {
  T_DEBUG_T("Handshake succeeded");
  finishConnectionAccepted(sock);
}

void TEventWorker::handshakeError(TAsyncSSLSocket *sock,
                                  const TTransportException& ex)
  noexcept {
  T_ERROR("TEventWorker: SSL handshake failed: %s", ex.what());
  sock->destroy();
}


void TEventWorker::finishConnectionAccepted(TAsyncSocket *asyncSocket) {
  // Create a new TEventConnection for this client socket.
  TEventConnection* clientConnection;

  shared_ptr<TAsyncSocket> asyncSocketPtr(asyncSocket,
                                          folly::DelayedDestruction::Destructor());
  try {
    folly::SocketAddress clientAddr;
    asyncSocketPtr->getPeerAddress(&clientAddr);
    clientConnection = createConnection(asyncSocketPtr, &clientAddr);
  } catch (...) {
    // Fail fast if we could not create a TEventConnection object
    T_ERROR("TEventWorker: failed to create a new TEventConnection");
    return;
  }

  // begin i/o on connection
  clientConnection->start();
}

void TEventWorker::acceptError(const std::exception& ex) noexcept {
  // We just log an error message if an accept error occurs.
  // Most accept errors are transient (e.g., out of file descriptors), so we
  // will continue trying to accept new connections.
  T_ERROR("TEventWorker: error accepting connection: %s", ex.what());
}

void TEventWorker::acceptStopped() noexcept {
  // Stop the worker main loop
  stopConsuming();
  eventBase_.terminateLoopSoon();
}

/**
 * Register the core libevent events onto the proper base.
 */
void TEventWorker::registerEvents() {
  if (server_->getCallTimeout() > 0) {
    eventBase_.setMaxLatency(server_->getCallTimeout() * Util::US_PER_MS,
                           std::bind(&TEventWorker::maxLatencyCob, this));
  }

  startConsuming(&eventBase_, &notificationQueue_);

   // Print some libevent stats
  T_DEBUG_T("libevent %s method %s\n",
                  folly::EventBase::getLibeventVersion(),
                  folly::EventBase::getLibeventMethod());

}

void TEventWorker::maxLatencyCob() {
  if (!activeConnectionList_.empty()) {
    TEventConnection* connection = activeConnectionList_.back();
    ConnectionMap::iterator it = activeConnectionMap_.find(connection);
    if (it == activeConnectionMap_.end()) {
      T_ERROR("connection %p not found in ConnectionMap", connection);
    } else {
      // This happens when iterations of the loop in EventBase::loop() are
      // taking too long. Likely causes:
      // - Server uses async handlers, but makes some blocking call or otherwise
      // ties up the async I/O thread for an extended time time.
      // - Too many connections, so that processing all the I/O events that fire
      // in one loop iteration takes too long.
      T_ERROR("worker %d: latency overload: nuking connection %p\n",
              workerID_, connection);
      // remove now since we might get called again before it shuts down.
      activeConnectionList_.erase(it->second);
      activeConnectionMap_.erase(it);

      connection->stop();
    }
  }
}

/// cause the notification callback to occur within the appropriate context;
/// note that we're likely called from a different thread than the worker
bool TEventWorker::notifyCompletion(TaskCompletionMessage &&msg) {
  T_DEBUG_T("worker %d: notifying connection %p ", workerID_,
            msg.connection);
  try {
    notificationQueue_.putMessage(std::move(msg));
  } catch (const std::exception &ex) {
    T_ERROR("worker %d: Failed to send notification: %s", workerID_, ex.what());
    return false;
  }
  return true;
}

/**
  * A task called notifyCompletion(); make the appropriate callback
  */
void TEventWorker::messageAvailable(TaskCompletionMessage &&msg) {
  // for now, just invoke task complete.  Next diff will also handle
  // input/output memory buffers.
  msg.connection->handleAsyncTaskComplete(true);
}

/**
 * All the work gets done here via callbacks to acceptConnections() and
 * to the handler functions in TAsyncSocket
 */
void TEventWorker::serve() {
  try {
    // Inform the EventBaseManager that our EventBase will be used
    // for this thread.  This relies on the fact that TEventWorker always
    // starts in a brand new thread, so nothing else has tried to use the
    // EventBaseManager to get an event base for this thread yet.
    server_->getEventBaseManager()->setEventBase(&eventBase_, false);

    registerEvents();
    eventBase_.loop();
  }
  catch (TException& tx) {
    std::string err("TEventServer: Caught TException: ");
    err += tx.what();
    GlobalOutput(err.c_str());
  }
}

void TEventWorker::shutdown() {
  eventBase_.terminateLoopSoon();
}

TEventWorker::~TEventWorker() {
  // Cleanup any connection that were active when serving was shutdown.
  while (!activeConnectionList_.empty()) {
    auto connection = activeConnectionList_.front();
    connection->cleanup();
  }
  activeConnectionList_.clear();
  activeConnectionMap_.clear();

  while (!connectionStack_.empty()) {
    delete connectionStack_.top();
    connectionStack_.pop();
  }
}

}}} // apache::thrift::async
