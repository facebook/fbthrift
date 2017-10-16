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

#include <thrift/lib/cpp2/transport/util/ConnectionThread.h>

#include <folly/Conv.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSClientConnection.h>

DEFINE_string(
    transport,
    "http2",
    "The transport to use (http1, http2, rsocket)");
DEFINE_bool(use_ssl, false, "Create an encrypted client connection");

namespace apache {
namespace thrift {

using apache::thrift::async::TAsyncSSLSocket;
using apache::thrift::async::TAsyncSocket;
using apache::thrift::async::TAsyncTransport;

ConnectionThread::~ConnectionThread() {
  getEventBase()->runInEventBaseThreadAndWait([&]() {
    SYNCHRONIZED(connections_) {
      connections_.clear();
    }
  });
}

std::shared_ptr<ClientConnectionIf> ConnectionThread::getConnection(
    const std::string& addr,
    uint16_t port) {
  std::string serverKey = folly::to<std::string>(addr, ":", port);
  getEventBase()->runInEventBaseThreadAndWait(
      [&]() { maybeCreateConnection(serverKey, addr, port); });
  SYNCHRONIZED(connections_) {
    return connections_[serverKey];
  }
  LOG(FATAL) << "Unreachable";
}

void ConnectionThread::maybeCreateConnection(
    const std::string& serverKey,
    const std::string& addr,
    uint16_t port) {
  SYNCHRONIZED(connections_) {
    std::shared_ptr<ClientConnectionIf>& connection = connections_[serverKey];
    if (connection == nullptr || !connection->good()) {
      TAsyncSocket::UniquePtr socket(
          new TAsyncSocket(getEventBase(), addr, port));
      if (FLAGS_use_ssl) {
        auto sslContext = std::make_shared<folly::SSLContext>();
        if (FLAGS_transport == "rsocket") {
          sslContext->setAdvertisedNextProtocols({"rs"});
        } else if (FLAGS_transport == "http1") {
          sslContext->setAdvertisedNextProtocols({"http"});
        } else {
          sslContext->setAdvertisedNextProtocols({"h2", "http"});
        }
        auto sslSocket = new TAsyncSSLSocket(
            sslContext, getEventBase(), socket->detachFd(), false);
        sslSocket->sslConn(nullptr);
        socket.reset(sslSocket);
      }
      if (FLAGS_transport == "rsocket") {
        connection = std::make_shared<RSClientConnection>(
            std::move(socket), getEventBase(), FLAGS_use_ssl);
      } else if (FLAGS_transport == "http1") {
        connection = H2ClientConnection::newHTTP1xConnection(
            std::move(socket), addr, "/");
      } else {
        if (FLAGS_transport != "http2") {
          LOG(ERROR) << "Unknown transport " << FLAGS_transport
                     << ".  Will use http2.";
        }
        connection = H2ClientConnection::newHTTP2Connection(
            std::move(socket), addr, "/");
      }
    }
  }
}

} // namespace thrift
} // namespace apache
