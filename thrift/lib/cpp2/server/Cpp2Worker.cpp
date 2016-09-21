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

#include <thrift/lib/cpp2/server/Cpp2Worker.h>

#include <thrift/lib/cpp2/server/Cpp2Connection.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/server/SSLRejectingManager.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/concurrency/Util.h>
#include <wangle/acceptor/SSLAcceptorHandshakeHelper.h>
#include <wangle/acceptor/UnencryptedAcceptorHandshakeHelper.h>


#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include <glog/logging.h>

#include <folly/String.h>
#include <folly/io/async/AsyncSSLSocket.h>
#include <folly/portability/Sockets.h>
#include <folly/portability/SysUio.h>

DEFINE_int32(pending_interval, 0, "Pending count interval in ms");

namespace apache { namespace thrift {

using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using std::shared_ptr;
using apache::thrift::concurrency::Util;

void Cpp2Worker::onNewConnection(
  folly::AsyncTransportWrapper::UniquePtr sock,
  const folly::SocketAddress* addr,
  const std::string& nextProtocolName,
  wangle::SecureTransportType secureProtocolType,
  const wangle::TransportInfo& tinfo) {

  auto observer = server_->getObserver();
  if (server_->maxConnections_ > 0 &&
      (getConnectionManager()->getNumConnections() >=
       server_->maxConnections_ / server_->nWorkers_) ) {
    if (observer) {
      observer->connDropped();
      observer->connRejected();
    }
    return;
  }

  TAsyncSocket* tsock = dynamic_cast<TAsyncSocket*>(sock.release());
  CHECK(tsock);
  auto asyncSocket = std::shared_ptr<TAsyncSocket>(tsock, TAsyncSocket::Destructor());

  VLOG(4) << "Cpp2Worker: Creating connection for socket " <<
    asyncSocket->getFd();

  asyncSocket->setIsAccepted(true);
  asyncSocket->setShutdownSocketSet(server_->shutdownSocketSet_.get());
  std::shared_ptr<Cpp2Connection> result(
    new Cpp2Connection(asyncSocket, addr, shared_from_this()));
  Acceptor::addConnection(result.get());
  result->addConnection(result);
  result->start();

  VLOG(4) << "created connection for fd " << asyncSocket->getFd();
  if (observer) {
    observer->connAccepted();
  }
}

void Cpp2Worker::plaintextConnectionReady(
    folly::AsyncTransportWrapper::UniquePtr sock,
    const folly::SocketAddress& clientAddr,
    const std::string& nextProtocolName,
    wangle::SecureTransportType secureTransportType,
    wangle::TransportInfo& tinfo) {
  auto asyncSocket =
    sock->getUnderlyingTransport<folly::AsyncSocket>();
  CHECK(asyncSocket) << "Underlying socket is not a AsyncSocket type";
  asyncSocket->setShutdownSocketSet(server_->shutdownSocketSet_.get());
  auto peekingManager = new SSLRejectingManager(
      this,
      clientAddr,
      nextProtocolName,
      secureTransportType,
      tinfo);
  peekingManager->start(std::move(sock), server_->getObserver());
}

void Cpp2Worker::useExistingChannel(
    const std::shared_ptr<HeaderServerChannel>& serverChannel) {

  folly::SocketAddress address;

  auto conn = std::make_shared<Cpp2Connection>(
      nullptr, &address, shared_from_this(), serverChannel);
  Acceptor::getConnectionManager()
    ->addConnection(conn.get(), false);
  conn->addConnection(conn);

  conn->start();
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
  std::chrono::milliseconds acceptLatency,
  wangle::SSLErrorEnum error,
  wangle::SecureTransportType type) noexcept {
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
    wangle::Acceptor* acceptor,
    const folly::SocketAddress& clientAddr,
    std::chrono::steady_clock::time_point acceptTime,
    wangle::TransportInfo& tinfo) {
  switch (getSSLPolicy()) {
  case SSLPolicy::DISABLED:
    return wangle::AcceptorHandshakeHelper::UniquePtr(
        new wangle::UnencryptedAcceptorHandshakeHelper());

  default:
  case SSLPolicy::PERMITTED:
    if (TLSHelper::looksLikeTLS(bytes)) {
      // Returning null causes the higher layer to default to SSL.
      return nullptr;
    } else {
      return wangle::AcceptorHandshakeHelper::UniquePtr(
          new wangle::UnencryptedAcceptorHandshakeHelper());
    }

  case SSLPolicy::REQUIRED:
    return nullptr;

  }
}

}} // apache::thrift
