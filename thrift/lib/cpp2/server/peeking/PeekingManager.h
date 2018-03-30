/*
 * Copyright 2004-present Facebook, Inc.
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

#pragma once

#include <folly/io/Cursor.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp2/server/peeking/TLSHelper.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/acceptor/ManagedConnection.h>
#include <wangle/acceptor/SocketPeeker.h>

namespace apache {
namespace thrift {

/**
 * The number of bytes that will be read from the socket.
 * TLSHelper currently needs the most bytes. Thus, it's cap
 * it up at the amount that TLSHelper needs.
 */
constexpr uint8_t kPeekBytes = 13;

/**
 * A manager that rejects or accepts connections based on critera
 * added by helper functions. This is useful for cases where
 * clients might be sending different types of protocols
 * over plaintext and it's up to the Acceptor to determine
 * what kind of protocol they are talking to route to the
 * appropriate handlers.
 */
class PeekingManager : public wangle::ManagedConnection,
                       public wangle::SocketPeeker::Callback {
 public:
  PeekingManager(
      std::shared_ptr<apache::thrift::Cpp2Worker> acceptor,
      const folly::SocketAddress& clientAddr,
      const std::string& nextProtocolName,
      wangle::SecureTransportType secureTransportType,
      wangle::TransportInfo tinfo,
      apache::thrift::ThriftServer* server,
      bool checkTLS = false)
      : acceptor_(acceptor),
        clientAddr_(clientAddr),
        nextProtocolName_(nextProtocolName),
        secureTransportType_(secureTransportType),
        tinfo_(std::move(tinfo)),
        server_(server),
        checkTLS_(checkTLS) {}

  ~PeekingManager() override = default;

  void start(
      folly::AsyncTransportWrapper::UniquePtr socket,
      std::shared_ptr<apache::thrift::server::TServerObserver> obs) noexcept {
    socket_ = std::move(socket);
    observer_ = std::move(obs);
    auto underlyingSocket =
        socket_->getUnderlyingTransport<folly::AsyncSocket>();
    CHECK(underlyingSocket) << "Underlying socket is not a AsyncSocket type";
    acceptor_->getConnectionManager()->addConnection(this, true);
    peeker_.reset(
        new wangle::SocketPeeker(*underlyingSocket, this, kPeekBytes));
    peeker_->start();
  }

  void peekSuccess(std::vector<uint8_t> peekBytes) noexcept override {
    folly::DelayedDestruction::DestructorGuard dg(this);
    peeker_ = nullptr;
    acceptor_->getConnectionManager()->removeConnection(this);

    if (checkTLS_) {
      checkTLSBytes(peekBytes);
    } else {
      checkConnectionBytes(peekBytes);
    }
    destroy();
  }

  /**
   * This rejects SSL connections with an alert. It is
   * useful for cases where clients might send SSL connections on
   * a plaintext port and you need to fail fast to tell clients to
   * go away.
   */
  void checkTLSBytes(std::vector<uint8_t>& peekBytes) {
    if (TLSHelper::looksLikeTLS(peekBytes)) {
      LOG(ERROR) << "Received SSL connection on non SSL port";
      sendPlaintextTLSAlert(peekBytes);
      if (observer_) {
        observer_->protocolError();
      }
      dropConnection();
      return;
    }
    acceptor_->connectionReady(
        std::move(socket_),
        std::move(clientAddr_),
        std::move(nextProtocolName_),
        secureTransportType_,
        tinfo_);
  }

  /**
   * Route the socket to a handler if the handler determines that it
   * is able to handle the connection by peeking in the first few bytes.
   */
  void checkConnectionBytes(std::vector<uint8_t>& peekBytes) {
    // Check for new transports
    bool acceptedHandler = false;
    for (auto const& handler : *server_->getRoutingHandlers()) {
      if (handler->canAcceptConnection(peekBytes)) {
        handler->handleConnection(
            acceptor_->getConnectionManager(),
            std::move(socket_),
            &clientAddr_,
            tinfo_);
        acceptedHandler = true;
        break;
      }
    }

    // Default to Header Transport
    if (!acceptedHandler) {
      acceptor_->handleHeader(std::move(socket_), &clientAddr_);
      return;
    }
  }

  void sendPlaintextTLSAlert(const std::vector<uint8_t>& peekBytes) {
    uint8_t major = peekBytes[1];
    uint8_t minor = peekBytes[2];
    auto alert = TLSHelper::getPlaintextAlert(
        major, minor, TLSHelper::Alert::UNEXPECTED_MESSAGE);
    socket_->writeChain(nullptr, std::move(alert));
  }

  void peekError(const folly::AsyncSocketException&) noexcept override {
    dropConnection();
  }

  void timeoutExpired() noexcept override {
    dropConnection();
  }

  void dropConnection() override {
    peeker_ = nullptr;
    acceptor_->getConnectionManager()->removeConnection(this);
    socket_->closeNow();
    destroy();
  }

  void describe(std::ostream& os) const override {
    os << "Peeking the socket " << clientAddr_;
  }

  bool isBusy() const override {
    return true;
  }

  void notifyPendingShutdown() override {}

  void closeWhenIdle() override {}

  void dumpConnectionState(uint8_t /* loglevel */) override {}

 private:
  folly::AsyncTransportWrapper::UniquePtr socket_;
  std::shared_ptr<apache::thrift::server::TServerObserver> observer_;
  typename wangle::SocketPeeker::UniquePtr peeker_;

  std::shared_ptr<apache::thrift::Cpp2Worker> acceptor_;
  folly::SocketAddress clientAddr_;
  std::string nextProtocolName_;
  wangle::SecureTransportType secureTransportType_;
  wangle::TransportInfo tinfo_;
  ThriftServer* server_;
  bool checkTLS_;
};
} // namespace thrift
} // namespace apache
