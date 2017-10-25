/*
 * Copyright 2017-present Facebook, Inc.
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

#include <folly/SocketAddress.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSClientConnection.h>

using apache::thrift::ClientConnectionIf;
using apache::thrift::H2ClientConnection;
using apache::thrift::HeaderClientChannel;
using apache::thrift::RSClientConnection;
using apache::thrift::ThriftClient;
using apache::thrift::async::TAsyncSSLSocket;
using apache::thrift::async::TAsyncSocket;

template <typename AsyncClient>
static std::unique_ptr<AsyncClient> newHeaderClient(
    folly::EventBase* evb,
    folly::SocketAddress const& addr) {
  auto sock = TAsyncSocket::newSocket(evb, addr);
  auto chan = HeaderClientChannel::newChannel(std::move(sock));
  chan->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);
  return std::make_unique<AsyncClient>(std::move(chan));
}

template <typename AsyncClient>
static std::unique_ptr<AsyncClient> newHTTP2Client(
    folly::EventBase* evb,
    folly::SocketAddress const& addr,
    bool encrypted) {
  TAsyncSocket::UniquePtr sock(new TAsyncSocket(evb, addr));
  if (encrypted) {
    auto sslContext = std::make_shared<folly::SSLContext>();
    sslContext->setAdvertisedNextProtocols({"h2"});
    auto sslSock =
        new TAsyncSSLSocket(sslContext, evb, sock->detachFd(), false);
    sslSock->sslConn(nullptr);
    sock.reset(sslSock);
  }
  auto sslContext = std::make_shared<folly::SSLContext>();
  std::shared_ptr<ClientConnectionIf> conn =
      H2ClientConnection::newHTTP2Connection(std::move(sock));
  auto client = ThriftClient::Ptr(new ThriftClient(conn, evb));
  client->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);
  client->setTimeout(500);
  return std::make_unique<AsyncClient>(std::move(client));
}

template <typename AsyncClient>
static std::unique_ptr<AsyncClient> newRSocketClient(
    folly::EventBase* evb,
    folly::SocketAddress const& addr,
    bool encrypted) {
  TAsyncSocket::UniquePtr sock(new TAsyncSocket(evb, addr));
  if (encrypted) {
    auto sslContext = std::make_shared<folly::SSLContext>();
    sslContext->setAdvertisedNextProtocols({"rs"});
    auto sslSock =
        new TAsyncSSLSocket(sslContext, evb, sock->detachFd(), false);
    sslSock->sslConn(nullptr);
    sock.reset(sslSock);
  }
  std::shared_ptr<ClientConnectionIf> conn =
      std::make_shared<RSClientConnection>(std::move(sock), evb);
  auto client = ThriftClient::Ptr(new ThriftClient(conn, evb));
  client->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);
  return std::make_unique<AsyncClient>(std::move(client));
}

template <typename AsyncClient>
static std::unique_ptr<AsyncClient> newClient(
    folly::EventBase* evb,
    folly::SocketAddress const& addr,
    folly::StringPiece transport,
    bool encrypted = false) {
  if (transport == "header") {
    return newHeaderClient<AsyncClient>(evb, addr);
  }
  if (transport == "rsocket") {
    return newRSocketClient<AsyncClient>(evb, addr, encrypted);
  }
  if (transport == "http2") {
    return newHTTP2Client<AsyncClient>(evb, addr, encrypted);
  }
  return nullptr;
}

template <typename AsyncClient>
class ConnectionThread : public folly::ScopedEventBaseThread {
 public:
  ~ConnectionThread() {
    getEventBase()->runInEventBaseThreadAndWait([&] { connection_.reset(); });
  }

  std::shared_ptr<AsyncClient> newSyncClient(
      folly::SocketAddress const& addr,
      folly::StringPiece transport,
      bool encrypted = false) {
    DCHECK(connection_ == nullptr);
    getEventBase()->runInEventBaseThreadAndWait([&]() {
      connection_ =
          newClient<AsyncClient>(getEventBase(), addr, transport, encrypted);
    });
    return connection_;
  }

 private:
  std::shared_ptr<AsyncClient> connection_;
};
