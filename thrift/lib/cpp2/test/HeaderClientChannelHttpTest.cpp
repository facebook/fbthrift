/*
 * Copyright 2014-present Facebook, Inc.
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

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp2/server/proxygen/ProxygenThriftServer.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <boost/lexical_cast.hpp>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;
using std::string;

class TestServiceHandler : public TestServiceSvIf {
 public:
  void sendResponse(string& _return, int64_t size) override {
    _return = "test" + boost::lexical_cast<std::string>(size);
  }

  void noResponse(int64_t size) override { usleep(size); }

  void echoRequest(string& _return, std::unique_ptr<string> req) override {
    _return = *req + "ccccccccccccccccccccccccccccccccccccccccccccc";
  }

  void serializationTest(string& _return, bool /* inEventBase */) override {
    _return = string(4096, 'a');
  }

  void eventBaseAsync(string& _return) override { _return = "hello world"; }

  void notCalledBack() override {}
  void voidResponse() override {}
  int32_t processHeader() override { return 1; }
  void echoIOBuf(
      std::unique_ptr<folly::IOBuf>& /*_return*/,
      std::unique_ptr<folly::IOBuf> /*buf*/) override {}
};

std::shared_ptr<BaseThriftServer> createHttpServer() {
  auto handler = std::make_shared<TestServiceHandler>();
  auto tm = ThreadManager::newSimpleThreadManager(1, 5, false, 50);
  tm->threadFactory(std::make_shared<PosixThreadFactory>());
  tm->start();
  auto server = std::make_shared<ProxygenThriftServer>();
  server->setAddress({"::1", 0});
  server->setHTTPServerProtocol(proxygen::HTTPServer::Protocol::HTTP);
  server->setInterface(handler);
  server->setNumIOWorkerThreads(1);
  server->setThreadManager(tm);
  return server;
}

TEST(HeaderClientChannelHttpTest, SimpleTest) {
  ScopedServerInterfaceThread runner(createHttpServer());
  auto const addr = runner.getAddress();

  folly::EventBase eb;
  std::shared_ptr<TAsyncSocket> socket = TAsyncSocket::newSocket(&eb, addr);
  auto channel = HeaderClientChannel::newChannel(socket);
  channel->useAsHttpClient("127.0.0.1", "meh");
  channel->setProtocolId(T_BINARY_PROTOCOL);
  TestServiceAsyncClient client(std::move(channel));
  client.sendResponse(
      [](apache::thrift::ClientReceiveState&& state) {
        EXPECT_FALSE(state.exception()) << state.exception();
        std::string res;
        TestServiceAsyncClient::recv_sendResponse(res, state);
        EXPECT_EQ(res, "test24");
      },
      24);
  eb.loop();

  client.eventBaseAsync([](apache::thrift::ClientReceiveState&& state) {
    EXPECT_FALSE(state.exception()) << state.exception();
    std::string res;
    TestServiceAsyncClient::recv_eventBaseAsync(res, state);
    EXPECT_EQ(res, "hello world");
  });
  eb.loop();
}

TEST(HeaderClientChannel, LongResponse) {
  ScopedServerInterfaceThread runner(createHttpServer());
  auto const addr = runner.getAddress();

  folly::EventBase eb;
  std::shared_ptr<TAsyncSocket> socket = TAsyncSocket::newSocket(&eb, addr);
  auto channel = HeaderClientChannel::newChannel(socket);
  channel->useAsHttpClient("127.0.0.1", "meh");
  channel->setProtocolId(T_BINARY_PROTOCOL);
  TestServiceAsyncClient client(std::move(channel));

  client.serializationTest(
      [](apache::thrift::ClientReceiveState&& state) {
        EXPECT_FALSE(state.exception()) << state.exception();
        std::string res;
        TestServiceAsyncClient::recv_serializationTest(res, state);
        EXPECT_EQ(res, string(4096, 'a'));
      },
      true);
  eb.loop();
}
