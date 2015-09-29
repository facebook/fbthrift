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

#include <folly/io/async/AsyncServerSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <folly/io/async/HHWheelTimer.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <unordered_set>

#include <wangle/acceptor/ConnectionManager.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/acceptor/PeekingAcceptorHandshakeHelper.h>

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
class Cpp2Worker
    : public wangle::Acceptor {
 public:

  /**
   * Cpp2Worker is the actual server object for existing connections.
   * One or more of these should be created by ThriftServer (one per
   * CPU core is recommended).
   *
   * @param server the ThriftServer which created us.
   * @param serverChannel existing server channel to use, only for duplex server
   */
  explicit Cpp2Worker(ThriftServer* server,
             const std::shared_ptr<HeaderServerChannel>&
             serverChannel = nullptr,
             folly::EventBase* eventBase = nullptr) :
    Acceptor(server->getServerSocketConfig()),
    server_(server),
    eventBase_(eventBase),
    activeRequests_(0),
    pendingCount_(0),
    pendingTime_(std::chrono::steady_clock::now()) {
    auto observer =
      std::dynamic_pointer_cast<folly::EventBaseObserver>(
      server_->getObserver());
    if (serverChannel) {
      // duplex
      useExistingChannel(serverChannel);
    } else if (!eventBase_) {
      eventBase_ = folly::EventBaseManager::get()->getEventBase();
    }
    if (observer) {
      eventBase_->setObserver(observer);
    }

    Acceptor::init(nullptr, eventBase_);
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

  /// An instance's folly::EventBase for I/O.
  folly::EventBase* eventBase_;

  void onNewConnection(folly::AsyncSocket::UniquePtr,
                       const folly::SocketAddress*,
                       const std::string&,
                       SecureTransportType,
                       const wangle::TransportInfo&) override;

  folly::AsyncSocket::UniquePtr makeNewAsyncSocket(folly::EventBase* base,
                                                   int fd) override {
    return folly::AsyncSocket::UniquePtr(new apache::thrift::async::TAsyncSocket(base, fd));
  }

  folly::AsyncSSLSocket::UniquePtr makeNewAsyncSSLSocket(
      const std::shared_ptr<folly::SSLContext>& ctx,
      folly::EventBase* base,
      int fd) override {
    return folly::AsyncSSLSocket::UniquePtr(
        new apache::thrift::async::TAsyncSSLSocket(
          ctx,
          base,
          fd,
          true, /* set server */
          true /* defer the security negotiation until sslAccept. */));
  }

  /**
   * For a duplex Thrift server, use an existing channel
   */
  void useExistingChannel(
      const std::shared_ptr<HeaderServerChannel>& serverChannel);

  uint32_t activeRequests_;

  int pendingCount_;
  std::chrono::steady_clock::time_point pendingTime_;

  enum { kPeekCount = 9 };
  using PeekingHelper = wangle::PeekingAcceptorHandshakeHelper<kPeekCount>;

  class PeekingCallback : public PeekingHelper::Callback {
   public:
    folly::Optional<SecureTransportType> getSecureTransportType(
        std::array<uint8_t, kPeekCount> peekedBytes) override;
  };
  PeekingCallback peekingCallback_;

  void startHandshakeHelper(
      folly::AsyncSSLSocket::UniquePtr sslSock,
      wangle::Acceptor* acceptor,
      const folly::SocketAddress& clientAddr,
      std::chrono::steady_clock::time_point acceptTime,
      wangle::TransportInfo& tinfo) noexcept override {

    auto helper = new PeekingHelper(
        std::move(sslSock),
        this,
        clientAddr,
        acceptTime,
        tinfo,
        &peekingCallback_
      );
      helper->start();
  }

  friend class Cpp2Connection;
  friend class ThriftServer;
};

}} // apache::thrift

#endif // #ifndef CPP2_WORKER_H_
