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

#ifndef CPP2_WORKER_H_
#define CPP2_WORKER_H_ 1

#include "thrift/lib/cpp/async/TAsyncServerSocket.h"
#include "thrift/lib/cpp/async/TAsyncSSLSocket.h"
#include "thrift/lib/cpp/async/HHWheelTimer.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TEventHandler.h"
#include "thrift/lib/cpp/server/TServer.h"
#include <unordered_set>

namespace apache { namespace thrift {

// Forward declaration of classes
class Cpp2Connection;
class ThriftServer;

/**
 * Cpp2Worker drives the actual I/O for ThriftServer connections.
 *
 * The ThriftServer itself accepts incoming connections, then hands off each
 * connection to a Cpp2Worker running in another thread.  There should
 * typically be around one Cpp2Worker thread per core.
 */
class Cpp2Worker :
      public apache::thrift::server::TServer,
      public apache::thrift::async::TAsyncServerSocket::AcceptCallback,
      public apache::thrift::async::TAsyncSSLSocket::HandshakeCallback {
 public:

  /**
   * Cpp2Worker is the actual server object for existing connections.
   * One or more of these should be created by ThriftServer (one per
   * CPU core is recommended).
   *
   * @param processorFactory a TAsyncProcessorFactory object as
   *        obtained from the generated Thrift code (the user service
   *        is integrated through this).
   * @param inputProtocolFactory the TProtocolFactory class supporting
   *        inbound Thrift requests.
   * @param outputProtocolFactory the TProtocolFactory class supporting
   *        responses (if any) after processing completes.
   * @param server the ThriftServer which created us.
   * @param workerID the ID assigned to this worker
   */
  Cpp2Worker(ThriftServer* server,
               uint32_t workerID) :
    TServer(std::shared_ptr<apache::thrift::server::TProcessor>()),
    server_(server),
    eventBase_(),
    workerID_(workerID),
    activeRequests_(0),
    pendingCount_(0) {
    eventBase_.setObserver(server_->getObserver());
  }

  /**
   * Destroy a Cpp2Worker. Clean up connection list.
   */
  virtual ~Cpp2Worker();

  /**
   * Get my TEventBase object.
   *
   * @returns pointer to my TEventBase object.
   */
  apache::thrift::async::TEventBase* getEventBase() {
    return &eventBase_;
  }

  /**
   * Get underlying server.
   *
   * @returns pointer to ThriftServer
   */
   ThriftServer* getServer() const {
    return server_;
  }

  void scheduleIdleConnectionTimeout(Cpp2Connection* con);
  void scheduleTimeout(apache::thrift::async::HHWheelTimer::Callback* callback,
                       std::chrono::milliseconds timeout);

  /**
   * Close a specific channel.
   */
  void closeConnection(std::shared_ptr<Cpp2Connection>);

  /**
   * Close all channels.
   */
  void closeConnections();

  /**
   * Get my numeric worker ID (for diagnostics).
   *
   * @return integer ID of this worker
   */
  int32_t getID() {
    return workerID_;
  }

  void connectionAccepted(
    int fd,
    const apache::thrift::transport::TSocketAddress& clientAddr) noexcept;
  void acceptError(const std::exception& ex) noexcept;
  void acceptStopped() noexcept;
  void stopEventBase() noexcept;

  /**
   * TAsyncSSLSocket::HandshakeCallback interface
   */
  void handshakeSuccess(apache::thrift::async::TAsyncSSLSocket *sock) noexcept;
  void handshakeError(
    apache::thrift::async::TAsyncSSLSocket *sock,
    const apache::thrift::transport::TTransportException& ex) noexcept;

  /**
   * Enter event loop and serve.
   */
  void serve();

  /**
   * Count the number of pending fds.  Used for overload detection
   * Not thread-safe
   */
  void pendingCount();

  /**
   * Cached pending count.  Thread-safe.
   */
  int getPendingCount();

 protected:
    apache::thrift::async::HHWheelTimer::UniquePtr timer_;

 private:
  /// The mother ship.
  ThriftServer* server_;

  /// An instance's TEventBase for I/O.
  apache::thrift::async::TEventBase eventBase_;

  /// Our ID in [0:nWorkers).
  uint32_t workerID_;

  /**
   * Called when the connection is fully accepted (after SSL accept if needed)
   */
  void finishConnectionAccepted(apache::thrift::async::TAsyncSocket *asyncSock);

  /**
   * Create or reuse a Cpp2Connection initialized for the given  socket FD.
   *
   * @param socket the FD of a freshly-connected socket.
   * @param address the peer address of the socket.
   * @return pointer to a TConenction object for that socket.
   */
  std::shared_ptr<Cpp2Connection> createConnection(
    std::shared_ptr<apache::thrift::async::TAsyncSocket> asyncSocket,
      const apache::thrift::transport::TSocketAddress* address);

  /**
   * Handler called when a new connection may be available.
   */
  void acceptConnections();

  /**
   * The list of active connections
   */
  std::unordered_set<std::shared_ptr<Cpp2Connection>> activeConnections_;

  uint32_t activeRequests_;

  int pendingCount_;

  friend class Cpp2Connection;
};

}} // apache::thrift

#endif // #ifndef CPP2_WORKER_H_
