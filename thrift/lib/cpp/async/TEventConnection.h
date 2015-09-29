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

#ifndef THRIFT_ASYNC_TEVENTCONNECTION_H_
#define THRIFT_ASYNC_TEVENTCONNECTION_H_ 1

#include <thrift/lib/cpp/server/TConnectionContext.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/async/TEventServer.h>
#include <memory>
#include <boost/noncopyable.hpp>

namespace apache { namespace thrift {

class TProcessor;

namespace protocol {
class TProtocol;
}

namespace server {
class TServerEventHandler;
}

namespace transport {
class TMemoryBuffer;
}

namespace async {

class TAsyncEventChannel;
class TAsyncProcessor;
class TEventWorker;
class TAsyncSocket;
class TaskCompletionMessage;

/**
 * Represents a connection that is handled via libevent. This connection
 * essentially encapsulates a socket that has some associated libevent state.
 */
class TEventConnection : private boost::noncopyable,
                         public folly::EventBase::LoopCallback {
 public:

  /**
   * Constructor for TEventConnection.
   *
   * @param asyncSocket shared pointer to the async socket
   * @param address the peer address of this connection
   * @param worker the worker instance that is handling this connection
   */
  TEventConnection(std::shared_ptr<TAsyncSocket> asyncSocket,
                   const folly::SocketAddress* address,
                   TEventWorker* worker, TEventServer::TransportType transport);

  /**
   * (Re-)Initialize a TEventConnection.  We break this out from the
   * constructor to allow for pooling.
   *
   * @param asyncSocket shared pointer to the async socket
   * @param address the peer address of this connection
   * @param worker the worker instance that is handling this connection
   */
  void init(std::shared_ptr<TAsyncSocket> asyncSocket,
            const folly::SocketAddress* address,
            TEventWorker* worker, TEventServer::TransportType transport);

  /// First cause -- starts i/o on connection
  void start();

  /// Shut down the connection even if it's OK; used for load reduction.
  void stop() {
    shutdown_ = true;
  }

  /// Return a pointer to the worker that owns us
  TEventWorker* getWorker() const {
    return worker_;
  }

  /// cause the notification callback to occur within the appropriate context
  bool notifyCompletion(TaskCompletionMessage &&msg);

  /// Run scheduled read when there are too many reads on the stack
  void runLoopCallback() noexcept override;

  std::shared_ptr<apache::thrift::TProcessor> getProcessor() const {
    return processor_;
  }

  std::shared_ptr<apache::thrift::protocol::TProtocol>
   getInputProtocol() const {
    return inputProtocol_;
  }

  std::shared_ptr<apache::thrift::protocol::TProtocol>
   getOutputProtocol() const {
    return outputProtocol_;
  }

  /// Get the per-server event handler set for this server, if any
  std::shared_ptr<apache::thrift::server::TServerEventHandler>
   getServerEventHandler() const {
    return serverEventHandler_;
  }

  /// Get the TConnectionContext for this connection
  server::TConnectionContext* getConnectionContext() {
    return &context_;
  }

  /// Destructor -- close down the connection.
  ~TEventConnection() override;

  /**
   * Check the size of our memory buffers and resize if needed.  Do not call
   * when a call is in progress.
   */
  void checkBufferMemoryUsage();

 private:
  class ConnContext : public server::TConnectionContext {
   public:
    void init(const folly::SocketAddress* address,
              std::shared_ptr<protocol::TProtocol> inputProtocol,
              std::shared_ptr<protocol::TProtocol> outputProtocol) {
      address_ = *address;
      inputProtocol_ = inputProtocol;
      outputProtocol_ = outputProtocol;
    }

    const folly::SocketAddress* getPeerAddress() const override {
      return &address_;
    }

    void reset() {
      address_.reset();
      cleanupUserData();
    }

    // TODO(dsanduleac): implement the virtual getInputProtocol() & such

    std::shared_ptr<protocol::TProtocol> getInputProtocol() const override {
      // from TEventConnection
      return inputProtocol_;
    }

    std::shared_ptr<protocol::TProtocol> getOutputProtocol() const override {
      return outputProtocol_;
    }

   private:
    folly::SocketAddress address_;
    std::shared_ptr<protocol::TProtocol> inputProtocol_;
    std::shared_ptr<protocol::TProtocol> outputProtocol_;
  };

  void readNextRequest();
  void handleReadSuccess();
  void handleReadFailure();
  void handleAsyncTaskComplete(bool success);
  void handleSendSuccess();
  void handleSendFailure();

  void handleEOF();
  void handleFailure(const char* msg);
  void cleanup();

  //! The worker instance handling this connection.
  TEventWorker* worker_;

  //! This connection's socket.
  std::shared_ptr<TAsyncSocket> asyncSocket_;

  //! Transport that the processor reads from.
  std::shared_ptr<apache::thrift::transport::TMemoryBuffer> inputTransport_;

  //! Transport that the processor writes to.
  std::shared_ptr<apache::thrift::transport::TMemoryBuffer> outputTransport_;

  /// Largest size of read buffer seen since buffer was constructed
  size_t largestReadBufferSize_;

  /// Largest size of write buffer seen since buffer was constructed
  size_t largestWriteBufferSize_;

  /// Count of the number of calls for use with getResizeBufferEveryN().
  int32_t callsForResize_;

  //! Protocol decoder.
  std::shared_ptr<apache::thrift::protocol::TProtocol> inputProtocol_;

  //! Protocol encoder.
  std::shared_ptr<apache::thrift::protocol::TProtocol> outputProtocol_;

  //! Channel that actually performs the socket I/O and callbacks.
  TAsyncEventChannel* asyncChannel_;

  /// Count of outstanding processor callbacks (generally 0 or 1).
  int32_t processorActive_;

  //! Count of the number of handleReadSuccess frames on the stack
  int32_t readersActive_;

  /// Sync processor if we're in queuing mode
  std::shared_ptr<apache::thrift::TProcessor> processor_;

  /// Flag used to shut down connection (used for load-shedding mechanism).
  bool shutdown_;

  /// Flag indicating that we have deferred closing down (processor was active)
  bool closing_;

  /// The per-server event handler set for this erver, if any
  std::shared_ptr<apache::thrift::server::TServerEventHandler>
   serverEventHandler_;

  /// per-connection context
  ConnContext context_;

  /// Our processor
  std::shared_ptr<TAsyncProcessor> asyncProcessor_;

  /// So that TEventWorker can call handleAsyncTaskComplete();
  friend class TEventWorker;

  /// Make the server a friend so it can manage tasks when overloaded
  friend class TEventServer;

  /// Make an async task a friend so it can communicate a cleanup() to us.
  friend class TEventTask;
};

}}} // apache::thrift::async

#endif // #ifndef THRIFT_ASYNC_TEVENTCONNECTION_H_
