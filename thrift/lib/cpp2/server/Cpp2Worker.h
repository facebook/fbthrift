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

#ifndef CPP2_WORKER_H_
#define CPP2_WORKER_H_ 1

#include <thrift/lib/cpp/async/TAsyncServerSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/HHWheelTimer.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TEventHandler.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <unordered_set>

#include <folly/experimental/wangle/ConnectionManager.h>

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
   * @param serverChannel existing server channel to use, only for duplex server
   */
  Cpp2Worker(ThriftServer* server,
             uint32_t workerID,
             const std::shared_ptr<HeaderServerChannel>&
                 serverChannel = nullptr) :
    TServer(std::shared_ptr<apache::thrift::server::TProcessor>()),
    server_(server),
    eventBase_(),
    workerID_(workerID),
    activeRequests_(0),
    pendingCount_(0),
    pendingTime_(std::chrono::steady_clock::now()) {
    auto observer =
      std::dynamic_pointer_cast<apache::thrift::async::EventBaseObserver>(
      server_->getObserver());
    if (serverChannel) {
      // duplex
      useExistingChannel(serverChannel);
    } else {
      eventBase_.reset(new async::TEventBase);
    }
    if (observer) {
      eventBase_->setObserver(observer);
    }
    manager_ = folly::wangle::ConnectionManager::makeUnique(
      eventBase_.get(), server->getIdleTimeout());
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
    return eventBase_.get();
  }

  /**
   * Get underlying server.
   *
   * @returns pointer to ThriftServer
   */
   ThriftServer* getServer() const {
    return server_;
  }

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
   * Count the number of pending fds. Used for overload detection.
   * Not thread-safe.
   */
  int pendingCount();

  /**
   * Cached pending count. Thread-safe.
   */
  int getPendingCount() const;

 private:
  /// The mother ship.
  ThriftServer* server_;

  /// An instance's TEventBase for I/O.
  std::shared_ptr<apache::thrift::async::TEventBase> eventBase_;

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
   * For a duplex Thrift server, use an existing channel
   */
  void useExistingChannel(
      const std::shared_ptr<HeaderServerChannel>& serverChannel);

  uint32_t activeRequests_;

  int pendingCount_;
  std::chrono::steady_clock::time_point pendingTime_;

  friend class Cpp2Connection;
  friend class ThriftServer;

  folly::wangle::ConnectionManager::UniquePtr manager_;
};

}} // apache::thrift

#endif // #ifndef CPP2_WORKER_H_
