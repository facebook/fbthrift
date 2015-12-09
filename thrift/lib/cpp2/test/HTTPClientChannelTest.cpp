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

#include <gtest/gtest.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/transport/THttpServer.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/util/TThreadedServerCreator.h>

#include <thrift/lib/cpp2/test/gen-cpp/TestService.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/async/HTTPClientChannel.h>

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
    if (size >= 0) {
      usleep(size);
    }

    _return = "test" + boost::lexical_cast<std::string>(size);
  }

  void noResponse(int64_t size) override { usleep(size); }

  void echoRequest(string& _return, const string& req) override {
    _return = req + "ccccccccccccccccccccccccccccccccccccccccccccc";
  }

  void serializationTest(string& _return, bool inEventBase) override {
    _return = string(4096, 'a');
  }

  void eventBaseAsync(string& _return) override { _return = "hello world"; }

  void notCalledBack() override {}
  void voidResponse() override {}
  int32_t processHeader() override { return 1; }
  void echoIOBuf(string& /*_return*/, const string& /*buf*/) override {}
};

std::unique_ptr<ScopedServerThread> createHttpServer() {
  auto handler = std::make_shared<TestServiceHandler>();
  auto processor = std::make_shared<TestServiceProcessor>(handler);
  std::shared_ptr<TTransportFactory> transportFactory =
      std::make_shared<THttpServerTransportFactory>();
  std::shared_ptr<TProtocolFactory> protocolFactory =
      std::make_shared<TBinaryProtocolFactoryT<THttpServer>>();
  TThreadedServerCreator serverCreator(
      processor, 0, transportFactory, protocolFactory);
  return folly::make_unique<ScopedServerThread>(&serverCreator);
}

TEST(HTTPClientChannelTest, SimpleTestAsync) {
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  folly::EventBase eb;
  const folly::SocketAddress* addr = serverThread->getAddress();
  TAsyncTransport::UniquePtr socket(new TAsyncSocket(&eb, *addr));
  auto channel = HTTPClientChannel::newHTTP1xChannel(
      std::move(socket), "127.0.0.1", "/foobar");
  TestServiceAsyncClient client(std::move(channel));
  client.sendResponse([&eb](apache::thrift::ClientReceiveState&& state) {
    if (state.exception()) {
      try {
        std::rethrow_exception(state.exception());
      } catch (const std::exception& e) {
        LOG(INFO) << e.what();
      }
    }
    EXPECT_TRUE(state.exception() == nullptr);
    std::string res;
    TestServiceAsyncClient::recv_sendResponse(res, state);
    EXPECT_EQ(res, "test24");
    eb.terminateLoopSoon();
  }, 24);
  eb.loop();

  client.eventBaseAsync([&eb](apache::thrift::ClientReceiveState&& state) {
    EXPECT_TRUE(state.exception() == nullptr);
    std::string res;
    TestServiceAsyncClient::recv_eventBaseAsync(res, state);
    EXPECT_EQ(res, "hello world");
    eb.terminateLoopSoon();
  });
  eb.loop();
}

TEST(HTTPClientChannelTest, SimpleTestSync) {
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  folly::EventBase eb;
  const folly::SocketAddress* addr = serverThread->getAddress();
  TAsyncTransport::UniquePtr socket(new TAsyncSocket(&eb, *addr));
  auto channel = HTTPClientChannel::newHTTP1xChannel(
      std::move(socket), "127.0.0.1", "/foobar");
  TestServiceAsyncClient client(std::move(channel));
  std::string res;
  client.sync_sendResponse(res, 24);
  EXPECT_EQ(res, "test24");

  client.sync_eventBaseAsync(res);
  EXPECT_EQ(res, "hello world");
}

TEST(HTTPClientChannelTest, LongResponse) {
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  folly::EventBase eb;
  const folly::SocketAddress* addr = serverThread->getAddress();
  TAsyncTransport::UniquePtr socket(new TAsyncSocket(&eb, *addr));
  auto channel = HTTPClientChannel::newHTTP1xChannel(
      std::move(socket), "127.0.0.1", "/foobar");
  TestServiceAsyncClient client(std::move(channel));

  client.serializationTest([&eb](apache::thrift::ClientReceiveState&& state) {
    EXPECT_TRUE(state.exception() == nullptr);
    std::string res;
    TestServiceAsyncClient::recv_serializationTest(res, state);
    EXPECT_EQ(res, string(4096, 'a'));
    eb.terminateLoopSoon();
  }, true);
  eb.loop();
}

TEST(HTTPClientChannelTest, ClientTimeout) {
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  folly::EventBase eb;
  const folly::SocketAddress* addr = serverThread->getAddress();
  TAsyncTransport::UniquePtr socket(new TAsyncSocket(&eb, *addr));
  auto channel = HTTPClientChannel::newHTTP1xChannel(
      std::move(socket), "127.0.0.1", "/foobar");
  channel->setTimeout(1);
  TestServiceAsyncClient client(std::move(channel));
  bool threw = false;
  client.sendResponse([&](apache::thrift::ClientReceiveState&& state) {
    if (state.exception()) {
      try {
        std::rethrow_exception(state.exception());
      } catch (const TTransportException& e) {
        auto expected = TTransportException::TIMED_OUT;
        EXPECT_EQ(expected, e.getType());
        threw = true;
      }
    } else {
      std::string res;
      TestServiceAsyncClient::recv_sendResponse(res, state);
      LOG(WARNING) << res;
      EXPECT_EQ(res, "test99999");
    }
    eb.terminateLoopSoon();
  }, 99999);
  eb.loop();

  EXPECT_TRUE(threw);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
