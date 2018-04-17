/*
 * Copyright 2018-present Facebook, Inc.
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

#include <gtest/gtest.h>

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <proxygen/httpserver/ScopedHTTPServer.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h>

namespace apache {
namespace thrift {

TEST(H2ClientConnectionTest, ServerCloseSocketImmediate) {
  folly::SocketAddress saddr;
  saddr.setFromLocalPort(static_cast<uint16_t>(0));
  proxygen::HTTPServer::IPConfig cfg{saddr,
                                     proxygen::HTTPServer::Protocol::HTTP2};

  auto server =
      proxygen::ScopedHTTPServer::start(cfg, proxygen::HTTPServerOptions{});
  ASSERT_NE(nullptr, server);
  const auto port = server->getPort();
  ASSERT_NE(0, port);

  folly::EventBase evb;
  folly::SocketAddress addr;
  addr.setFromLocalPort(port);
  async::TAsyncSocket::UniquePtr sock(new async::TAsyncSocket(&evb, addr));
  auto conn = H2ClientConnection::newHTTP2Connection(std::move(sock));
  EXPECT_TRUE(conn->good());

  server.reset(); // server closes connection

  evb.loop();
  EXPECT_FALSE(conn->good());
  EXPECT_EQ(nullptr, conn->getTransport());
}

} // namespace thrift
} // namespace apache
