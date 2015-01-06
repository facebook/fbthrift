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
  const apache::thrift::transport::TSocketAddress* addr,
  const std::string& nextProtocolName,
  const folly::TransportInfo& tinfo) {

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
          [&](folly::wangle::ManagedConnection* connection) {
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

}} // apache::thrift
