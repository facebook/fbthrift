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

#ifndef THRIFT_SERVER_TEVENTWORKER_H_
#define THRIFT_SERVER_TEVENTWORKER_H_ 1

#include <thrift/lib/cpp/async/TAsyncServerSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TEventTask.h>
#include <thrift/lib/cpp/async/TEventHandler.h>
#include <folly/io/async/NotificationQueue.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <ext/hash_map>
#include <list>
#include <stack>

namespace apache { namespace thrift { namespace async {

// Forward declaration of classes
class TAsyncProcessorFactory;
class TEventConnection;
class TEventServer;
/**
 * TEventWorker drives the actual I/O for TEventServer connections.
 *
 * The TEventServer itself accepts incoming connections, then hands off each
 * connection to a TEventWorker running in another thread.  There should
 * typically be around one TEventWorker thread per core.
 */
class TEventWorker :
      public apache::thrift::server::TServer,
      public TAsyncServerSocket::AcceptCallback,
      public TAsyncSSLSocket::HandshakeCallback,
      public folly::NotificationQueue<TaskCompletionMessage>::Consumer {
 private:
  /// Object that processes requests.
  std::shared_ptr<TAsyncProcessorFactory> asyncProcessorFactory_;

  /// The mother ship.
  TEventServer* server_;

  /// An instance's TEventBase for I/O.
  TEventBase eventBase_;

  /// Our ID in [0:nWorkers).
  uint32_t workerID_;

  /// Pipe that task completion notifications are sent over
  folly::NotificationQueue<TaskCompletionMessage> notificationQueue_;

  /**
   * A stack of idle TEventConnection objects for reuse.
   * When we close a connection, we place it on this stack so that the
   * object can be reused later, rather than freeing the memory and
   * reallocating a new object later.
   */
  std::stack<TEventConnection*> connectionStack_;

  /// Transport type to use
  TEventServer::TransportType transportType_;

  /**
   * Called when the connection is fully accepted (after SSL accept if needed)
   */
  void finishConnectionAccepted(TAsyncSocket *asyncSock);

  /**
   * Create or reuse a TEventConnection initialized for the given  socket FD.
   *
   * @param socket the FD of a freshly-connected socket.
   * @param address the peer address of the socket.
   * @return pointer to a TConenction object for that socket.
   */
  TEventConnection* createConnection(
      std::shared_ptr<TAsyncSocket> asyncSocket,
      const folly::SocketAddress* address);

  /**
   * Handler called when a new connection may be available.
   */
  void acceptConnections();

  void makeCompletionCallback();

  /**
   * Initialize our TEventBase to generate incoming connection events.
   * Note that this is called once before the main loop is executed and
   * sets up a READ event on the output of the listener's socktpair.
   */
  void registerEvents();

  /**
   * Callback used when loop latency exceeds the requested threshold.
   */
  void maxLatencyCob();

  typedef std::list<TEventConnection*> ConnectionList;

  // pointer hash functor
  static const uint64_t kPtrHashMult = 5700357409661599291LL;
  static const uint64_t kPtrHashShift = 3;
  template<typename T>
    struct hash { };
  template<typename T>
    struct hash<T*> {
      size_t operator()(T* p) const {
        return ((size_t)p ^ ((size_t)p >> kPtrHashShift)) * kPtrHashMult;
      }
      size_t operator()(const T* p) const {
        return ((size_t)p ^ ((size_t)p >> kPtrHashShift)) * kPtrHashMult;
      }
    };
  typedef __gnu_cxx::hash_map<const TEventConnection*,
                              ConnectionList::iterator,
                              hash<TEventConnection*> > ConnectionMap;

  /**
   * The list of active connections (used to allow the oldest connections
   * to be shed during overload).
   */
  ConnectionList activeConnectionList_;
  /**
   * A hash map used to map connections to their place in the connection list
   */
  ConnectionMap activeConnectionMap_;

  // Max number of active connections
  int32_t maxNumActiveConnections_;

 public:

  /**
   * TEventWorker is the actual server object for existing connections.
   * One or more of these should be created by TEventServer (one per
   * CPU core is recommended).
   *
   * @param processorFactory a TAsyncProcessorFactory object as
   *        obtained from the generated Thrift code (the user service
   *        is integrated through this).
   * @param inputProtocolFactory the TProtocolFactory class supporting
   *        inbound Thrift requests.
   * @param outputProtocolFactory the TProtocolFactory class supporting
   *        responses (if any) after processing completes.
   * @param server the TEventServer which created us.
   * @param workerID the ID assigned to this worker
   */
  TEventWorker(std::shared_ptr<TAsyncProcessorFactory> processorFactory,
               std::shared_ptr<apache::thrift::server::TDuplexProtocolFactory>
                protocolFactory,
               TEventServer* server,
               uint32_t workerID) :
    TServer(std::shared_ptr<apache::thrift::server::TProcessor>()),
    asyncProcessorFactory_(processorFactory),
    server_(server),
    eventBase_(),
    workerID_(workerID),
    transportType_(TEventServer::FRAMED),
    maxNumActiveConnections_(0) {

    setDuplexProtocolFactory(protocolFactory);
    transportType_ = server->getTransportType();
    if (transportType_ == TEventServer::FRAMED) {
      // do the dynamic cast once rather than per connection
      if (dynamic_cast<apache::thrift::protocol::THeaderProtocolFactory*>(
            protocolFactory.get())) {
        transportType_ = TEventServer::HEADER;
      }
    }
  }

  /**
   * Destroy a TEventWorker. We've use boost::scoped_ptr<> to take care
   * of freeing up memory, so nothing to be done here but release the
   * connection stack.
   */
  ~TEventWorker() override;

  /**
   * Get my TAsyncProcessorFactory object.
   *
   * @returns pointer to my TAsyncProcessorFactory object.
   */
  std::shared_ptr<TAsyncProcessorFactory> getAsyncProcessorFactory() {
    return asyncProcessorFactory_;
  }

  /**
   * Get my TEventBase object.
   *
   * @returns pointer to my TEventBase object.
   */
  TEventBase* getEventBase() {
    return &eventBase_;
  }

  /**
   * Get underlying server.
   *
   * @returns pointer to TEventServer
   */
   TEventServer* getServer() const {
    return server_;
  }

  /**
   * Get my numeric worker ID (for diagnostics).
   *
   * @return integer ID of this worker
   */
  int32_t getID() {
    return workerID_;
  }

  void setMaxNumActiveConnections(int32_t numActiveConnections) {
    maxNumActiveConnections_ = numActiveConnections;
  }

  int32_t getMaxNumActiveConnections() const {
    return maxNumActiveConnections_;
  }

  /**
   * Dispose of a TEventConnection object.
   * Will add to a pool of these objects or destroy as necessary.
   *
   * @param connection a now-idle connection.
   */
  void returnConnection(TEventConnection* connection);

  /**
   * Cause a completion callback for the requested connection to occur
   * within that connection's context.
   *
   * @param msg task completion message
   * @return true if notification was sent, false if it failed.
   */
  bool notifyCompletion(TaskCompletionMessage &&msg);

  /**
   * Task completed (called in this worker's thread)
   */
  void messageAvailable(TaskCompletionMessage&& msg) override;

  void connectionAccepted(
      int fd, const folly::SocketAddress& clientAddr) noexcept override;
  void acceptError(const std::exception& ex) noexcept override;
  void acceptStopped() noexcept override;

  /**
   * TAsyncSSLSocket::HandshakeCallback interface
   */
  void handshakeSuccess(TAsyncSSLSocket* sock) noexcept override;
  void handshakeError(
      TAsyncSSLSocket* sock,
      const transport::TTransportException& ex) noexcept override;

  /**
   * Enter event loop and serve.
   */
  void serve() override;

  /**
   * Exit event loop.
   */
  void shutdown();
};

}}} // apache::thrift::async

#endif // #ifndef THRIFT_SERVER_TEVENTWORKER_H_
