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
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/concurrency/Util.h>


#include <iostream>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include <glog/logging.h>

#include <folly/String.h>

DEFINE_int32(pending_interval, 0, "Pending count interval in ms");

namespace apache { namespace thrift {

using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using std::shared_ptr;
using apache::thrift::concurrency::Util;

void Cpp2Worker::onNewConnection(
  folly::AsyncSocket::UniquePtr sock,
  const folly::SocketAddress* addr,
  const std::string& nextProtocolName,
  SecureTransportType secureProtocolType,
  const wangle::TransportInfo& tinfo) {

  auto observer = server_->getObserver();
  if (server_->maxConnections_ > 0 &&
      (getConnectionManager()->getNumConnections() >=
       server_->maxConnections_ / server_->nWorkers_) ) {
    if (observer) {
      observer->connDropped();
    }
    return;
  }

  TAsyncSocket* tsock = dynamic_cast<TAsyncSocket*>(sock.release());
  CHECK(tsock);
  auto asyncSocket = std::shared_ptr<TAsyncSocket>(tsock, TAsyncSocket::Destructor());

  VLOG(4) << "Cpp2Worker: Creating connection for socket " <<
    asyncSocket->getFd();

  asyncSocket->setShutdownSocketSet(server_->shutdownSocketSet_.get());
  std::shared_ptr<Cpp2Connection> result(
    new Cpp2Connection(asyncSocket, addr, this));
  Acceptor::addConnection(result.get());
  result->addConnection(result);
  result->start();

  VLOG(4) << "created connection for fd " << asyncSocket->getFd();
  if (observer) {
    observer->connAccepted();
  }
}

void Cpp2Worker::useExistingChannel(
    const std::shared_ptr<HeaderServerChannel>& serverChannel) {

  folly::SocketAddress address;

  auto conn = std::make_shared<Cpp2Connection>(
      nullptr, &address, this, serverChannel);
  Acceptor::getConnectionManager()->addConnection(conn.get(), false);
  conn->addConnection(conn);

  DCHECK(!eventBase_);
  // Use supplied event base and don't delete it when finished
  eventBase_ = serverChannel->getEventBase();

  conn->start();
}

int Cpp2Worker::pendingCount() {
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

folly::Optional<SecureTransportType>
Cpp2Worker::PeekingCallback::getSecureTransportType(
    std::array<uint8_t, kPeekCount> bytes) {

  // TLS starts as
  // 0: 0x16 - handshake protocol magic
  // 1: 0x03 - SSL version major
  // 2: 0x00 to 0x03 - SSL version minor (SSLv3 or TLS1.0 through TLS1.2)
  // 3-4: length (2 bytes)
  // 5: 0x01 - handshake type (ClientHello)
  // 6-8: handshake len (3 bytes), equals value from offset 3-4 minus 4

  // Framed binary starts as
  // 0-3: frame len
  // 4: 0x80 - binary magic
  // 5: 0x01 - protocol version
  // 6-7: various
  // 8-11: method name len

  // Other Thrift transports/protocols can't conflict because they don't have
  // 16-03-01 at offsets 0-1-5.

  // Definitely not TLS
  if (bytes[0] != 0x16 || bytes[1] != 0x03 || bytes[5] != 0x01) {
    return SecureTransportType::NONE;
  }

  // This is most likely TLS, but could be framed binary, which has 80-01
  // at offsets 4-5.
  if (bytes[4] == 0x80 && bytes[8] != 0x7c) {
    // Binary will have the method name length at offsets 8-11, which must be
    // smaller than the frame length at 0-3, so byte 8 is <=  byte 0,
    // which is 0x16.
    // However, for TLS, bytes 6-8 (24 bits) are the length of the
    // handshake protocol and this value is 4 less than the record-layer
    // length at offset 3-4 (16 bits), so byte 8 equals 0x7c (0x80 - 4),
    // which is not smaller than 0x16
    return SecureTransportType::NONE;
  }

  return SecureTransportType::TLS;
}

}} // apache::thrift
