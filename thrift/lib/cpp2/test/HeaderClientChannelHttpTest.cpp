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
#include <thrift/lib/cpp/transport/THttpServer.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/util/TThreadedServerCreator.h>

#include <thrift/lib/cpp2/test/gen-cpp/TestService.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <boost/lexical_cast.hpp>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::test;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;
using std::string;

class TestServiceHandler : public TestServiceIf {
 public:
  void sendResponse(string& _return, int64_t size) override {
    _return = "test" + boost::lexical_cast<std::string>(size);
  }

  void noResponse(int64_t size) override { usleep(size); }

  void echoRequest(string& _return, const string& req) override {
    _return = req + "ccccccccccccccccccccccccccccccccccccccccccccc";
  }

  void serializationTest(string& _return, bool /* inEventBase */) override {
    _return = string(4096, 'a');
  }

  void eventBaseAsync(string& _return) override { _return = "hello world"; }

  void notCalledBack() override {}
  void voidResponse() override {}
  int32_t processHeader() override { return 1; }
  void echoIOBuf(string& /*_return*/, const string& /*req*/) override {}
};

std::unique_ptr<ScopedServerThread> createHttpServer() {
  auto handler = std::make_shared<TestServiceHandler>();
  auto processor = std::make_shared<TestServiceProcessor>(handler);
  std::shared_ptr<TTransportFactory> transportFactory =
      std::make_shared<THttpServerTransportFactory>();
  std::shared_ptr<TProtocolFactory> protocolFactory =
      std::make_shared<TBinaryProtocolFactoryT<THttpServer>>();
  TThreadedServerCreator serverCreator(processor,
                                       0,
                                       transportFactory,
                                       protocolFactory);
  return std::make_unique<ScopedServerThread>(&serverCreator);
}

TEST(HeaderClientChannelHttpTest, SimpleTest) {
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  folly::EventBase eb;
  const folly::SocketAddress* addr = serverThread->getAddress();
  std::shared_ptr<TAsyncSocket> socket = TAsyncSocket::newSocket(&eb, *addr);
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
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  folly::EventBase eb;
  const folly::SocketAddress* addr = serverThread->getAddress();
  std::shared_ptr<TAsyncSocket> socket = TAsyncSocket::newSocket(&eb, *addr);
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
