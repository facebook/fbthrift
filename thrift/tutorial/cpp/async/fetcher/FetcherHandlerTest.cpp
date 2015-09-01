/*
 * Copyright 2015 Facebook, Inc.
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

#include <map>
#include <thrift/lib/cpp/transport/TSocket.h>
#include <thrift/lib/cpp/transport/TServerSocket.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <thrift/tutorial/cpp/async/fetcher/gen-cpp2/Fetcher.h>
#include <thrift/tutorial/cpp/async/fetcher/FetcherHandler.h>

#include <gtest/gtest.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::tutorial::fetcher;

class FetcherHandlerTest : public testing::Test {
protected:
  EventBase eb;

  template <typename T>
  T sync(Future<T> f) { return f.waitVia(&eb).get(); }

  FetchHttpRequest makeRequest(SocketAddress addr, string path) {
    FetchHttpRequest request;
    request.addr = addr.getAddressStr();
    request.port = addr.getPort();
    request.path = path;
    return request;
  }
};

TEST_F(FetcherHandlerTest, example_pass) {
  const auto data = map<string, string>{
    {"/foo", "<html></html>"},
    {"/bar", "<html><head></head><body></body></html>"},
  };
  auto sock = make_shared<transport::TServerSocket>(0);
  sock->setAcceptTimeout(10);
  sock->listen();
  auto listen = thread([=] {
      while (true) {
        shared_ptr<transport::TRpcTransport> conn;
        try {
          conn = sock->accept();
        } catch (transport::TTransportException&) {
          break;  // closed
        }
        string raw;
        uint8_t buf[4096];
        while (auto k = conn->read(buf, sizeof(buf))) {
          raw.append(reinterpret_cast<const char*>(buf), k);
          if (StringPiece(raw).endsWith("\r\n\r\n")) {
            break;
          }
        }
        vector<StringPiece> lines;
        split("\r\n", raw, lines);
        auto line = lines.at(0);
        vector<StringPiece> parts;
        split(" ", line, parts);
        EXPECT_EQ("GET", parts.at(0));
        EXPECT_EQ("HTTP/1.0", parts.at(2));
        auto path = parts.at(1);
        auto i = data.find(path.str());
        if (i != data.end()) {
          const auto& content = i->second;
          conn->write(
              reinterpret_cast<const uint8_t*>(content.data()), content.size());
        }
      }
  });
  SCOPE_EXIT {
    sock->close();
    listen.join();
  };

  auto handler = make_shared<FetcherHandler>();
  ScopedServerInterfaceThread runner(handler);

  auto client = runner.newClient<FetcherAsyncClient>(eb);
  SocketAddress http;
  sock->getAddress(&http);

  EXPECT_EQ(
      data.at("/foo"),
      sync(client->future_fetchHttp(makeRequest(http, "/foo"))));
}
