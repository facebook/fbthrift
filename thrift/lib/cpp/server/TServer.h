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

#ifndef THRIFT_SERVER_TSERVER_H
#define THRIFT_SERVER_TSERVER_H 1

#include <thrift/lib/cpp/TProcessor.h>
#include <thrift/lib/cpp/transport/TServerTransport.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/util/shared_ptr_util.h>

namespace folly {
class SocketAddress;
}

namespace apache { namespace thrift {

namespace server {

using apache::thrift::TProcessor;
using apache::thrift::protocol::TBinaryProtocolFactory;
using apache::thrift::protocol::TProtocol;
using apache::thrift::protocol::TProtocolFactory;
using apache::thrift::protocol::TDuplexProtocolFactory;
using apache::thrift::protocol::TDualProtocolFactory;
using apache::thrift::protocol::TSingleProtocolFactory;
using apache::thrift::transport::TServerTransport;
using apache::thrift::transport::TTransport;
using apache::thrift::transport::TTransportFactory;
using apache::thrift::transport::TDuplexTransportFactory;
using apache::thrift::transport::TDualTransportFactory;
using apache::thrift::transport::TSingleTransportFactory;

class TConnectionContext;

/**
 * Virtual interface class that can handle events from the server core. To
 * use this you should subclass it and implement the methods that you care
 * about. Your subclass can also store local data that you may care about,
 * such as additional "arguments" to these methods (stored in the object
 * instance's state).
 */
class TServerEventHandler {
 public:

  virtual ~TServerEventHandler() {}

  /**
   * Called before the server begins.
   *
   * @param address The address on which the server is listening.
   */
  virtual void preServe(const folly::SocketAddress* address) {}

  /**
   * Called when a new client has connected and is about to being processing.
   *
   * @param ctx A pointer to the connection context.  The context will remain
   *            valid until the corresponding connectionDestroyed() call.
   */
  virtual void newConnection(TConnectionContext* ctx) {
    (void)ctx;
  }

  /**
   * Called when a client has finished request-handling to delete server
   * context.
   *
   * @param ctx A pointer to the connection context.  The context will be
   *            destroyed after connectionDestroyed() returns.
   */
  virtual void connectionDestroyed(TConnectionContext* ctx) {
    (void)ctx;
  }

 protected:

  /**
   * Prevent direct instantiation.
   */
  TServerEventHandler() {}

};

/**
 * Thrift server.
 *
 */
class TServer : public concurrency::Runnable {
 public:

  struct FailureInjection {
    FailureInjection()
        : errorFraction(0),
          dropFraction(0),
          disconnectFraction(0) {
    }

    // Cause a fraction of requests to fail
    float errorFraction;

    // Cause a fraction of requests to be dropped (and presumably time out
    // on the client)
    float dropFraction;

    // Cause a fraction of requests to cause the channel to be disconnected,
    // possibly failing other requests as well.
    float disconnectFraction;

    bool operator==(const FailureInjection& other) const {
      return errorFraction == other.errorFraction &&
        dropFraction == other.dropFraction &&
        disconnectFraction == other.disconnectFraction;
    }

    bool operator!=(const FailureInjection& other) const {
      return !(*this == other);
    }
  };

  virtual ~TServer() {}

  virtual void serve() = 0;

  virtual void stop() {}

  // This API is intended to stop listening on the server
  // socket and stop accepting new connection first while
  // still letting the established connections to be
  // processed on the server.
  virtual void stopListening() {}

  // Allows running the server as a Runnable thread
  virtual void run() {
    serve();
  }

  std::shared_ptr<TProcessorFactory> getProcessorFactory() {
    return processorFactory_;
  }

  std::shared_ptr<TServerTransport> getServerTransport() {
    return serverTransport_;
  }

  std::shared_ptr<TDuplexTransportFactory> getDuplexTransportFactory() {
    return duplexTransportFactory_;
  }

  std::shared_ptr<TDuplexProtocolFactory> getDuplexProtocolFactory() {
    return duplexProtocolFactory_;
  }

  std::shared_ptr<TServerEventHandler> getEventHandler() {
    return eventHandler_;
  }

  /**
   * Get the TConnectionContext for the connection currently being processed.
   *
   * This is intended to be invoked from within the TProcessor (or the handler
   * used by the TProcessor).
   *
   * Note: Not all server types currently support getConnectionContext().  Some
   * servers may always return nullptr.
   *
   * TODO: Eventually this method should be supported by all server types, and
   * made into a pure virtual method.
   *
   * @return Return a pointer to the TConnectionContext for the current
   *         connection, or nullptr if invoked outside of a call to
   *         TProcessor::process().  The returned object is only guaranteed to
   *         be valid until process() returns.
   */
  virtual TConnectionContext* getConnectionContext() const {
    return nullptr;
  }

  virtual void setFailureInjection(FailureInjection fi) {
    throw std::runtime_error(
      "setFailureInjection not supported by this implementation.");
  }

protected:
  template<typename ProcessorFactory>
  TServer(const std::shared_ptr<ProcessorFactory>& processorFactory,
          THRIFT_OVERLOAD_IF(ProcessorFactory, TProcessorFactory)):
    processorFactory_(processorFactory) {
    setTransportFactory(std::shared_ptr<TTransportFactory>(
                          new TTransportFactory()));
    setProtocolFactory(std::shared_ptr<TBinaryProtocolFactory>(
                         new TBinaryProtocolFactory()));
  }

  template<typename Processor>
  TServer(const std::shared_ptr<Processor>& processor,
          THRIFT_OVERLOAD_IF(Processor, TProcessor)):
    processorFactory_(new TSingletonProcessorFactory(processor)) {
    setTransportFactory(std::shared_ptr<TTransportFactory>(
                          new TTransportFactory()));
    setProtocolFactory(std::shared_ptr<TBinaryProtocolFactory>(
                         new TBinaryProtocolFactory()));
  }

  template<typename ProcessorFactory>
  TServer(const std::shared_ptr<ProcessorFactory>& processorFactory,
          const std::shared_ptr<TServerTransport>& serverTransport,
          THRIFT_OVERLOAD_IF(ProcessorFactory, TProcessorFactory)):
    processorFactory_(processorFactory),
    serverTransport_(serverTransport) {
    setTransportFactory(std::shared_ptr<TTransportFactory>(
                          new TTransportFactory()));
    setProtocolFactory(std::shared_ptr<TBinaryProtocolFactory>(
                         new TBinaryProtocolFactory()));
  }

  template<typename Processor>
  TServer(const std::shared_ptr<Processor>& processor,
          const std::shared_ptr<TServerTransport>& serverTransport,
          THRIFT_OVERLOAD_IF(Processor, TProcessor)):
    processorFactory_(new TSingletonProcessorFactory(processor)),
    serverTransport_(serverTransport) {
    setTransportFactory(std::shared_ptr<TTransportFactory>(
                          new TTransportFactory()));
    setProtocolFactory(std::shared_ptr<TBinaryProtocolFactory>(
                         new TBinaryProtocolFactory()));
  }

  template<typename ProcessorFactory>
  TServer(const std::shared_ptr<ProcessorFactory>& processorFactory,
          const std::shared_ptr<TServerTransport>& serverTransport,
          const std::shared_ptr<TTransportFactory>& transportFactory,
          const std::shared_ptr<TProtocolFactory>& protocolFactory,
          THRIFT_OVERLOAD_IF(ProcessorFactory, TProcessorFactory)):
    processorFactory_(processorFactory),
    serverTransport_(serverTransport) {
    setTransportFactory(transportFactory);
    setProtocolFactory(protocolFactory);
  }

  template<typename Processor>
  TServer(const std::shared_ptr<Processor>& processor,
          const std::shared_ptr<TServerTransport>& serverTransport,
          const std::shared_ptr<TTransportFactory>& transportFactory,
          const std::shared_ptr<TProtocolFactory>& protocolFactory,
          THRIFT_OVERLOAD_IF(Processor, TProcessor)):
    processorFactory_(new TSingletonProcessorFactory(processor)),
    serverTransport_(serverTransport) {
    setTransportFactory(transportFactory);
    setProtocolFactory(protocolFactory);
  }

  template<typename ProcessorFactory>
  TServer(
    const std::shared_ptr<ProcessorFactory>& processorFactory,
    const std::shared_ptr<TServerTransport>& serverTransport,
    const std::shared_ptr<TDuplexTransportFactory>& duplexTransportFactory,
    const std::shared_ptr<TDuplexProtocolFactory>& duplexProtocolFactory,
    THRIFT_OVERLOAD_IF(ProcessorFactory, TProcessorFactory)) :
    processorFactory_(processorFactory),
    serverTransport_(serverTransport),
    duplexTransportFactory_(duplexTransportFactory),
    duplexProtocolFactory_(duplexProtocolFactory) {}

  template<typename Processor>
  TServer(
    const std::shared_ptr<Processor>& processor,
    const std::shared_ptr<TServerTransport>& serverTransport,
    const std::shared_ptr<TDuplexTransportFactory>& duplexTransportFactory,
    const std::shared_ptr<TDuplexProtocolFactory>& duplexProtocolFactory,
    THRIFT_OVERLOAD_IF(Processor, TProcessor)) :
    processorFactory_(new TSingletonProcessorFactory(processor)),
    serverTransport_(serverTransport),
    duplexTransportFactory_(duplexTransportFactory),
    duplexProtocolFactory_(duplexProtocolFactory) {}

  /**
   * Get a TProcessor to handle calls on a particular connection.
   *
   * This method should only be called once per connection (never once per
   * call).  This allows the TProcessorFactory to return a different processor
   * for each connection if it desires.
   */
  std::shared_ptr<TProcessor> getProcessor(TConnectionContext* ctx) {
    return processorFactory_->getProcessor(ctx);
  }

  // Class variables
  std::shared_ptr<TProcessorFactory> processorFactory_;
  std::shared_ptr<TServerTransport> serverTransport_;

  std::shared_ptr<TDuplexTransportFactory> duplexTransportFactory_;
  std::shared_ptr<TDuplexProtocolFactory> duplexProtocolFactory_;

  std::shared_ptr<TServerEventHandler> eventHandler_;

public:
  void setProcessorFactory(
    std::shared_ptr<TProcessorFactory> processorFactory) {
    processorFactory_ = processorFactory;
  }

  void setTransportFactory(
    std::shared_ptr<TTransportFactory> transportFactory) {
    duplexTransportFactory_.reset(
      new TSingleTransportFactory<TTransportFactory>(transportFactory));
  }

  void setDuplexTransportFactory(
    std::shared_ptr<TDuplexTransportFactory> duplexTransportFactory) {
    duplexTransportFactory_ = duplexTransportFactory;
  }

  void setProtocolFactory(std::shared_ptr<TProtocolFactory> protocolFactory) {
    duplexProtocolFactory_.reset(
      new TSingleProtocolFactory<TProtocolFactory>(protocolFactory));
  }

  void setDuplexProtocolFactory(
    std::shared_ptr<TDuplexProtocolFactory> duplexProtocolFactory) {
    duplexProtocolFactory_ = duplexProtocolFactory;
  }

  void setServerEventHandler(
    std::shared_ptr<TServerEventHandler> eventHandler) {
    eventHandler_ = eventHandler;
  }

};

/**
 * Helper function to increase the max file descriptors limit
 * for the current process and all of its children.
 * By default, tries to increase it to as much as 2^24.
 */
 int increase_max_fds(int max_fds=(1<<24));


}}} // apache::thrift::server

#endif // #ifndef THRIFT_SERVER_TSERVER_H
