/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <unordered_set>

#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>
#include <folly/io/async/HHWheelTimer.h>
#include <folly/net/NetworkSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp2/security/FizzPeeker.h>
#include <thrift/lib/cpp2/server/RequestsRegistry.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/server/peeking/TLSHelper.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/acceptor/ConnectionManager.h>
#include <wangle/acceptor/PeekingAcceptorHandshakeHelper.h>

namespace apache {
namespace thrift {

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
class Cpp2Worker : public wangle::Acceptor,
                   private wangle::PeekingAcceptorHandshakeHelper::PeekCallback,
                   public std::enable_shared_from_this<Cpp2Worker> {
 protected:
  enum { kPeekCount = 9 };
  struct DoNotUse {};

 public:
  /**
   * Cpp2Worker is the actual server object for existing connections.
   * One or more of these should be created by ThriftServer (one per
   * CPU core is recommended).
   *
   * @param server the ThriftServer which created us.
   * @param serverChannel existing server channel to use, only for duplex server
   */
  static std::shared_ptr<Cpp2Worker> create(
      ThriftServer* server,
      const std::shared_ptr<HeaderServerChannel>& serverChannel = nullptr,
      folly::EventBase* eventBase = nullptr,
      std::shared_ptr<fizz::server::CertManager> certManager = nullptr,
      std::shared_ptr<wangle::SSLContextManager> ctxManager = nullptr,
      std::shared_ptr<const fizz::server::FizzServerContext> fizzContext =
          nullptr) {
    std::shared_ptr<Cpp2Worker> worker(new Cpp2Worker(server, {}));
    worker->setFizzCertManager(certManager);
    worker->setSSLContextManager(ctxManager);
    worker->construct(server, serverChannel, eventBase, fizzContext);
    return worker;
  }

  void init(
      folly::AsyncServerSocket* serverSocket,
      folly::EventBase* eventBase,
      wangle::SSLStats* stats,
      std::shared_ptr<const fizz::server::FizzServerContext> fizzContext)
      override {
    if (auto thriftConfigBase =
            folly::get_ptr(accConfig_.customConfigMap, "thrift_tls_config")) {
      assert(static_cast<ThriftTlsConfig*>((*thriftConfigBase).get()));
      auto thriftConfig =
          static_cast<ThriftTlsConfig*>((*thriftConfigBase).get());
      if (thriftConfig->enableThriftParamsNegotiation) {
        auto thriftParametersContext =
            std::make_shared<ThriftParametersContext>();
        fizzPeeker_.setThriftParametersContext(
            std::move(thriftParametersContext));
      }
    }
    securityProtocolCtxManager_.addPeeker(this);
    Acceptor::init(serverSocket, eventBase, stats, fizzContext);
  }

  /*
   * This is called from ThriftServer::stopDuplex
   * Necessary for keeping the ThriftServer alive until this Worker dies
   */
  void stopDuplex(std::shared_ptr<ThriftServer> ts);

  /**
   * Get underlying server.
   *
   * @returns pointer to ThriftServer
   */
  ThriftServer* getServer() const {
    return server_;
  }

  /**
   * SSL stats hook
   */
  void updateSSLStats(
      const folly::AsyncTransport* sock,
      std::chrono::milliseconds acceptLatency,
      wangle::SSLErrorEnum error,
      const folly::exception_wrapper& ex) noexcept override;

  void handleHeader(
      folly::AsyncTransport::UniquePtr sock,
      const folly::SocketAddress* addr);

  RequestsRegistry* getRequestsRegistry() const {
    return requestsRegistry_;
  }

  bool isStopping() {
    return stopping_;
  }

  struct ActiveRequestsDecrement {
    void operator()(Cpp2Worker* worker) {
      if (--worker->activeRequests_ == 0 && worker->stopping_) {
        worker->stopBaton_.post();
      }
    }
  };
  using ActiveRequestsGuard =
      std::unique_ptr<Cpp2Worker, ActiveRequestsDecrement>;
  ActiveRequestsGuard getActiveRequestsGuard();

 protected:
  Cpp2Worker(
      ThriftServer* server,
      DoNotUse /* ignored, never call constructor directly */)
      : Acceptor(server->getServerSocketConfig()),
        wangle::PeekingAcceptorHandshakeHelper::PeekCallback(kPeekCount),
        server_(server),
        activeRequests_(0) {
    setGracefulShutdownTimeout(server->workersJoinTimeout_);
  }

  void construct(
      ThriftServer*,
      const std::shared_ptr<HeaderServerChannel>& serverChannel,
      folly::EventBase* eventBase,
      std::shared_ptr<const fizz::server::FizzServerContext> fizzContext) {
    auto observer = std::dynamic_pointer_cast<folly::EventBaseObserver>(
        server_->getObserverShared());
    if (serverChannel) {
      eventBase = serverChannel->getEventBase();
    } else if (!eventBase) {
      eventBase = folly::EventBaseManager::get()->getEventBase();
    }
    init(nullptr, eventBase, nullptr, fizzContext);
    initRequestsRegistry();

    if (serverChannel) {
      // duplex
      useExistingChannel(serverChannel);
    }

    if (observer) {
      eventBase->add([eventBase, observer = std::move(observer)] {
        eventBase->setObserver(observer);
      });
    }
  }

  void onNewConnection(
      folly::AsyncTransport::UniquePtr,
      const folly::SocketAddress*,
      const std::string&,
      wangle::SecureTransportType,
      const wangle::TransportInfo&) override;

  virtual std::shared_ptr<folly::AsyncTransport> createThriftTransport(
      folly::AsyncTransport::UniquePtr);

  void markSocketAccepted(folly::AsyncSocket* sock);

  void plaintextConnectionReady(
      folly::AsyncTransport::UniquePtr sock,
      const folly::SocketAddress& clientAddr,
      const std::string& nextProtocolName,
      wangle::SecureTransportType secureTransportType,
      wangle::TransportInfo& tinfo) override;

  void requestStop();

  // returns false if timed out due to deadline
  bool waitForStop(std::chrono::system_clock::time_point deadline);

  virtual wangle::AcceptorHandshakeHelper::UniquePtr createSSLHelper(
      const std::vector<uint8_t>& bytes,
      const folly::SocketAddress& clientAddr,
      std::chrono::steady_clock::time_point acceptTime,
      wangle::TransportInfo& tinfo);

  wangle::DefaultToFizzPeekingCallback* getFizzPeeker() override {
    return &fizzPeeker_;
  }

 private:
  /// The mother ship.
  ThriftServer* server_;

  FizzPeeker fizzPeeker_;

  // For DuplexChannel case, set only during shutdown so that we can extend the
  // lifetime of the ThriftServer if the Worker is kept alive by some
  // Connections which are kept alive by in-flight requests
  std::shared_ptr<ThriftServer> duplexServer_;

  folly::AsyncSocket::UniquePtr makeNewAsyncSocket(
      folly::EventBase* base,
      int fd) override {
    return folly::AsyncSocket::UniquePtr(
        new folly::AsyncSocket(base, folly::NetworkSocket::fromFd(fd)));
  }

  folly::AsyncSSLSocket::UniquePtr makeNewAsyncSSLSocket(
      const std::shared_ptr<folly::SSLContext>& ctx,
      folly::EventBase* base,
      int fd) override {
    return folly::AsyncSSLSocket::UniquePtr(
        new apache::thrift::async::TAsyncSSLSocket(
            ctx,
            base,
            folly::NetworkSocket::fromFd(fd),
            true, /* set server */
            true /* defer the security negotiation until sslAccept. */));
  }

  /**
   * For a duplex Thrift server, use an existing channel
   */
  void useExistingChannel(
      const std::shared_ptr<HeaderServerChannel>& serverChannel);

  uint32_t activeRequests_;
  RequestsRegistry* requestsRegistry_;
  bool stopping_{false};
  folly::Baton<> stopBaton_;

  void initRequestsRegistry();

  wangle::AcceptorHandshakeHelper::UniquePtr getHelper(
      const std::vector<uint8_t>& bytes,
      const folly::SocketAddress& clientAddr,
      std::chrono::steady_clock::time_point acceptTime,
      wangle::TransportInfo& tinfo) override;

  bool isPlaintextAllowedOnLoopback() {
    return server_->isPlaintextAllowedOnLoopback();
  }

  SSLPolicy getSSLPolicy() {
    return server_->getSSLPolicy();
  }

  bool shouldPerformSSL(
      const std::vector<uint8_t>& bytes,
      const folly::SocketAddress& clientAddr);

  friend class Cpp2Connection;
  friend class ThriftServer;
  friend class RocketRoutingHandler;
  friend class TestRoutingHandler;
};

} // namespace thrift
} // namespace apache
