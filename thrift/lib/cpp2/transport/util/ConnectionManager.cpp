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

#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>

#include <folly/Conv.h>
#include <folly/Singleton.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSClientConnection.h>

DEFINE_int32(
    num_client_connections,
    1,
    "Number of connections client will establish with each "
    "server (a specific address and port).");
DEFINE_string(
    transport,
    "http2",
    "The transport to use (http1, http2, rsocket)");

namespace apache {
namespace thrift {

using apache::thrift::async::TAsyncSocket;
using apache::thrift::async::TAsyncTransport;
using std::string;

namespace {
folly::Singleton<ConnectionManager> factory;
}

std::shared_ptr<ConnectionManager> ConnectionManager::getInstance() {
  return factory.try_get();
}

ConnectionManager::ConnectionManager() {
  for (int32_t i = 0; i < FLAGS_num_client_connections; ++i) {
    threads_.push_back(std::make_unique<folly::ScopedEventBaseThread>());
  }
}

ConnectionManager::Connections::Connections() {
  current = 0;
  clients.resize(FLAGS_num_client_connections);
}

std::shared_ptr<ClientConnectionIf> ConnectionManager::getConnection(
    const string& addr,
    uint16_t port) {
  string server = folly::to<string>(addr, ":", port);
  SYNCHRONIZED(clients_) {
    Connections& candidates = clients_[server];
    auto index = candidates.current;
    candidates.current =
        (candidates.current + 1) % FLAGS_num_client_connections;
    std::shared_ptr<ClientConnectionIf>& connection = candidates.clients[index];
    if (connection == nullptr) {
      if (FLAGS_transport == "rsocket") {
        // TODO Make RSClientConnection accept TAsyncSocket
        folly::SocketAddress address;
        address.setFromHostPort(addr, port);

        connection = std::make_unique<RSClientConnection>(
            *threads_[index]->getEventBase(), address);
      } else {
        TAsyncTransport::UniquePtr transport(
            new TAsyncSocket(threads_[index]->getEventBase(), addr, port));

        if (FLAGS_transport == "http1") {
          connection = H2ClientConnection::newHTTP1xConnection(
              std::move(transport), addr, "/");
        } else {
          if (FLAGS_transport != "http2") {
            LOG(ERROR) << "Unknown transport " << FLAGS_transport
                       << ".  Will use http2.";
          }
          connection =
              H2ClientConnection::newHTTP2Connection(std::move(transport));
        }
      }
    }
    return connection;
  }
  LOG(FATAL) << "Unreachable";
}

} // namespace thrift
} // namespace apache
