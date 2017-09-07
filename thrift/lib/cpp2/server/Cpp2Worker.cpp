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

#include <thrift/lib/cpp2/server/Cpp2Worker.h>

#include <folly/String.h>
#include <folly/io/async/AsyncSSLSocket.h>
#include <folly/portability/Sockets.h>
#include <glog/logging.h>

#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/concurrency/Util.h>
#include <thrift/lib/cpp2/server/Cpp2Connection.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/server/peeking/PeekingManager.h>
#include <wangle/acceptor/EvbHandshakeHelper.h>
#include <wangle/acceptor/SSLAcceptorHandshakeHelper.h>
#include <wangle/acceptor/UnencryptedAcceptorHandshakeHelper.h>

DEFINE_int32(pending_interval, 0, "Pending count interval in ms");

namespace apache {
namespace thrift {

using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using std::shared_ptr;
using apache::thrift::concurrency::Util;

void Cpp2Worker::onNewConnection(
    folly::AsyncTransportWrapper::UniquePtr sock,
    const folly::SocketAddress* addr,
    const std::string& nextProtocolName,
    wangle::SecureTransportType,
    const wangle::TransportInfo& tinfo) {
  auto observer = server_->getObserver();
  if (server_->maxConnections_ > 0 &&
      (getConnectionManager()->getNumConnections() >=
       server_->maxConnections_ / server_->nWorkers_)) {
    if (observer) {
      observer->connDropped();
      observer->connRejected();
    }
    return;
  }

  // Check if this is an encypted connection to perform required transport
  // routing based on the application protocol.
  // TODO: (karthiksk) T21334731 We ideally should make connectionReady method
  // of Acceptor virtual to make it the single place for enforcing routing
  // decisions.
  if (!nextProtocolName.empty()) {
    for (auto& routingHandler : *server_->getRoutingHandlers()) {
      if (routingHandler->canAcceptEncryptedConnection(nextProtocolName)) {
        VLOG(4) << "Cpp2Worker: Routing encrypted connection for protocol "
                << nextProtocolName;
        // TODO: (karthiksk) T21334789 Eliminate const_cast by making
        // handleConnection take const folly::SocketAddress* as parameter.
        routingHandler->handleConnection(
            getConnectionManager(),
            std::move(sock),
            const_cast<folly::SocketAddress*>(addr),
            tinfo);
        return;
      }
    }
  }

  auto fd = sock->getUnderlyingTransport<folly::AsyncSocket>()->getFd();
  VLOG(4) << "Cpp2Worker: Creating connection for socket " << fd;

  auto thriftTransport = createThriftTransport(std::move(sock));
  auto connection = std::make_shared<Cpp2Connection>(
      std::move(thriftTransport), addr, shared_from_this());
  Acceptor::addConnection(connection.get());
  connection->addConnection(connection);
  connection->start();

  VLOG(4) << "Cpp2Worker: created connection for socket " << fd;

  if (observer) {
    observer->connAccepted();
    observer->activeConnections(
        getConnectionManager()->getNumConnections() * server_->nWorkers_);
  }
}

std::shared_ptr<async::TAsyncTransport> Cpp2Worker::createThriftTransport(
    folly::AsyncTransportWrapper::UniquePtr sock) {
  TAsyncSocket* tsock = dynamic_cast<TAsyncSocket*>(sock.release());
  CHECK(tsock);
  auto asyncSocket =
      std::shared_ptr<TAsyncSocket>(tsock, TAsyncSocket::Destructor());
  asyncSocket->setIsAccepted(true);
  asyncSocket->setShutdownSocketSet(server_->shutdownSocketSet_.get());
  return asyncSocket;
}

void Cpp2Worker::plaintextConnectionReady(
    folly::AsyncTransportWrapper::UniquePtr sock,
    const folly::SocketAddress& clientAddr,
    const std::string& nextProtocolName,
    wangle::SecureTransportType secureTransportType,
    wangle::TransportInfo& tinfo) {
  auto asyncSocket = sock->getUnderlyingTransport<folly::AsyncSocket>();
  CHECK(asyncSocket) << "Underlying socket is not a AsyncSocket type";
  asyncSocket->setShutdownSocketSet(server_->shutdownSocketSet_.get());
  auto peekingManager = new PeekingManager(
      this,
      clientAddr,
      nextProtocolName,
      secureTransportType,
      tinfo,
      server_->getRoutingHandlers(),
      server_->getNumIOWorkerThreads());
  peekingManager->start(std::move(sock), server_->getObserver());
}

void Cpp2Worker::useExistingChannel(
    const std::shared_ptr<HeaderServerChannel>& serverChannel) {
  folly::SocketAddress address;

  auto conn = std::make_shared<Cpp2Connection>(
      nullptr, &address, shared_from_this(), serverChannel);
  Acceptor::getConnectionManager()->addConnection(conn.get(), false);
  conn->addConnection(conn);

  conn->start();
}

void Cpp2Worker::stopDuplex(std::shared_ptr<ThriftServer> myServer) {
  // They better have given us the correct ThriftServer
  DCHECK(server_ == myServer.get());

  // This does not really fully drain everything but at least
  // prevents the connections from accepting new requests
  wangle::Acceptor::drainAllConnections();

  // Capture a shared_ptr to our ThriftServer making sure it will outlive us
  // Otherwise our raw pointer to it (server_) will be jeopardized.
  duplexServer_ = myServer;
}

int Cpp2Worker::computePendingCount() {
  // Only recalculate once every pending_interval
  if (FLAGS_pending_interval > 0) {
    auto now = std::chrono::steady_clock::now();
    if (pendingTime_ < now) {
      pendingTime_ = now + std::chrono::milliseconds(FLAGS_pending_interval);
      pendingCount_ = 0;
      Acceptor::getConnectionManager()->iterateConns(
          [&](wangle::ManagedConnection* connection) {
            if ((static_cast<Cpp2Connection*>(connection))->pending()) {
              pendingCount_++;
            }
          });
    }
  }

  return pendingCount_;
}

int Cpp2Worker::getPendingCount() const {
  return pendingCount_;
}

void Cpp2Worker::updateSSLStats(
    const folly::AsyncTransportWrapper* sock,
    std::chrono::milliseconds /* acceptLatency */,
    wangle::SSLErrorEnum error) noexcept {
  if (!sock) {
    return;
  }

  auto socket = sock->getUnderlyingTransport<folly::AsyncSSLSocket>();
  if (!socket) {
    return;
  }
  auto observer = server_->getObserver();
  if (!observer) {
    return;
  }
  if (socket->good() && error == wangle::SSLErrorEnum::NO_ERROR) {
    observer->tlsComplete();
    if (socket->getSSLSessionReused()) {
      observer->tlsResumption();
    }
  } else {
    observer->tlsError();
  }
}

wangle::AcceptorHandshakeHelper::UniquePtr Cpp2Worker::getHelper(
    const std::vector<uint8_t>& bytes,
    const folly::SocketAddress& clientAddr,
    std::chrono::steady_clock::time_point acceptTime,
    wangle::TransportInfo& ti) {
  auto sslPolicy = getSSLPolicy();
  auto performSSL = (sslPolicy == SSLPolicy::REQUIRED) ||
      (sslPolicy != SSLPolicy::DISABLED && TLSHelper::looksLikeTLS(bytes));

  if (!performSSL) {
    return wangle::AcceptorHandshakeHelper::UniquePtr(
        new wangle::UnencryptedAcceptorHandshakeHelper());
  }

  wangle::AcceptorHandshakeHelper::UniquePtr sslAcceptor(
      new wangle::SSLAcceptorHandshakeHelper(clientAddr, acceptTime, ti));

  // If we have a nonzero dedicated ssl handshake pool, offload the SSL
  // handshakes with EvbHandshakeHelper.
  if (server_->sslHandshakePool_->numThreads() > 0) {
    return wangle::EvbHandshakeHelper::UniquePtr(new wangle::EvbHandshakeHelper(
        std::move(sslAcceptor), server_->sslHandshakePool_->getEventBase()));
  } else {
    return sslAcceptor;
  }
}

} // namespace thrift
} // namespace apache
