/*
 * Copyright 2015-present Facebook, Inc.
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

#include <thrift/lib/cpp2/async/HTTPClientChannel.h>
#include <thrift/lib/cpp2/test/gen-cpp/TestService.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <wangle/bootstrap/ServerBootstrap.h>
#include <wangle/channel/AsyncSocketHandler.h>
#include <wangle/channel/Handler.h>
#include <wangle/channel/Pipeline.h>

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

  void serializationTest(string& _return, bool /* inEventBase */) override {
    _return = string(4096, 'a');
  }

  void eventBaseAsync(string& _return) override { _return = "hello world"; }

  void notCalledBack() override {}
  void voidResponse() override {}
  int32_t processHeader() override { return 1; }
  void echoIOBuf(string& /*_return*/, const string& /*buf*/) override {}
};

/**
 * This class implements a scoped server that immediately responds with
 * specified response upon receiving the first byte, then closes the
 * connection. It can be used for crafting invalid responses and testing
 * behavior.
 **/
class ScopedPresetResponseServer {
 public:
  explicit ScopedPresetResponseServer(std::unique_ptr<folly::IOBuf> resp)
      : resp_(std::move(resp)) {
    server_.childPipeline(std::make_shared<PipelineFactory>(resp_.get()));
    server_.bind(0);
    serverThread_ = std::thread([this]() { server_.waitForStop(); });
  }

  ~ScopedPresetResponseServer() {
    server_.stop();
    server_.join();
    serverThread_.join();
  }

  void getAddress(folly::SocketAddress* addr) {
    return server_.getSockets().at(0)->getAddress(addr);
  }

 private:
  using BytesPipeline =
      wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>;

  class Handler : public wangle::BytesToBytesHandler {
   public:
    explicit Handler(folly::IOBuf* resp)
        : wangle::BytesToBytesHandler(), resp_(resp) {}

    void read(Context* ctx, folly::IOBufQueue& /* msg */) override {
      write(ctx, resp_->clone()).then([=] { close(ctx); });
    }

   private:
    folly::IOBuf* resp_;
  };

  class PipelineFactory : public wangle::PipelineFactory<BytesPipeline> {
   public:
    explicit PipelineFactory(folly::IOBuf* resp)
        : wangle::PipelineFactory<BytesPipeline>(), resp_(resp) {}

    BytesPipeline::Ptr newPipeline(
        std::shared_ptr<folly::AsyncTransportWrapper> sock) override {
      auto pipeline = BytesPipeline::create();
      pipeline->addBack(wangle::AsyncSocketHandler(sock));
      pipeline->addBack(Handler(resp_));
      pipeline->finalize();
      return pipeline;
    }

   private:
    folly::IOBuf* resp_;
  };

  wangle::ServerBootstrap<BytesPipeline> server_;
  std::thread serverThread_;
  std::unique_ptr<folly::IOBuf> resp_;
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
  return std::make_unique<ScopedServerThread>(&serverCreator);
}

TEST(HTTPClientChannelTest, SimpleTestAsync) {
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  folly::EventBase eb;
  const folly::SocketAddress* addr = serverThread->getAddress();
  TAsyncTransport::UniquePtr socket(new TAsyncSocket(&eb, *addr));
  auto channel = HTTPClientChannel::newHTTP1xChannel(
      std::move(socket), "127.0.0.1", "/foobar");
  TestServiceAsyncClient client(std::move(channel));
  client.sendResponse(
      [&eb](apache::thrift::ClientReceiveState&& state) {
        EXPECT_FALSE(state.exception()) << state.exception();
        std::string res;
        TestServiceAsyncClient::recv_sendResponse(res, state);
        EXPECT_EQ(res, "test24");
        eb.terminateLoopSoon();
      },
      24);
  eb.loop();

  client.eventBaseAsync([&eb](apache::thrift::ClientReceiveState&& state) {
    EXPECT_FALSE(state.exception()) << state.exception();
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

  client.serializationTest(
      [&eb](apache::thrift::ClientReceiveState&& state) {
        EXPECT_FALSE(state.exception()) << state.exception();
        std::string res;
        TestServiceAsyncClient::recv_serializationTest(res, state);
        EXPECT_EQ(res, string(4096, 'a'));
        eb.terminateLoopSoon();
      },
      true);
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
  channel->setProtocolId(apache::thrift::protocol::T_BINARY_PROTOCOL);
  TestServiceAsyncClient client(std::move(channel));
  client.sendResponse(
      [&](apache::thrift::ClientReceiveState&& state) {
        EXPECT_TRUE(state.exception());
        auto ex = state.exception().get_exception();
        auto& e = dynamic_cast<TTransportException const&>(*ex);
        EXPECT_EQ(TTransportException::TIMED_OUT, e.getType());
        eb.terminateLoopSoon();
      },
      99999);
  eb.loop();
}

TEST(HTTPClientChannelTest, NoBodyResponse) {
  std::string resp = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
  ScopedPresetResponseServer server(folly::IOBuf::copyBuffer(resp));

  folly::EventBase eb;
  folly::SocketAddress addr;
  server.getAddress(&addr);
  TAsyncTransport::UniquePtr socket(new TAsyncSocket(&eb, addr));
  auto channel = HTTPClientChannel::newHTTP1xChannel(
      std::move(socket), "127.0.0.1", "/foobar");
  TestServiceAsyncClient client(std::move(channel));
  auto result = client.future_sendResponse(99999).waitVia(&eb).getTry();
  ASSERT_TRUE(result.hasException());
  folly::exception_wrapper ex = std::move(result.exception());
  EXPECT_TRUE(ex.is_compatible_with<TTransportException>());
  ex.with_exception([&](TTransportException const& e) {
    EXPECT_STREQ("Empty HTTP response, 400, Bad Request", e.what());
  });
}

TEST(HTTPClientChannelTest, NoGoodChannel) {
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  folly::EventBase eb;
  const folly::SocketAddress* addr = serverThread->getAddress();
  TAsyncTransport::UniquePtr socket(new TAsyncSocket(&eb, *addr));

  auto channel = HTTPClientChannel::newHTTP2Channel(std::move(socket));

  EXPECT_TRUE(channel->good());

  channel->getTransport()->close();

  EXPECT_FALSE(channel->good());
}

TEST(HTTPClientChannelTest, NoGoodChannel2) {
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  folly::EventBase eb;
  const folly::SocketAddress* addr = serverThread->getAddress();
  TAsyncTransport::UniquePtr socket(new TAsyncSocket(&eb, *addr));

  auto channel = HTTPClientChannel::newHTTP2Channel(std::move(socket));

  EXPECT_TRUE(channel->good());

  channel->closeNow();

  EXPECT_FALSE(channel->good());
}

TEST(HTTPClientChannelTest, EarlyShutdown) {
  std::unique_ptr<ScopedServerThread> serverThread = createHttpServer();

  // send 2 requests on the channel, both will hang for a little while,
  // then we destruct the client / channel object, we are expecting a
  // smooth shutdown
  {
    folly::EventBase eb;
    const folly::SocketAddress* addr = serverThread->getAddress();
    TAsyncTransport::UniquePtr socket(new TAsyncSocket(&eb, *addr));
    auto channel = HTTPClientChannel::newHTTP1xChannel(
        std::move(socket), "127.0.0.1", "/foobar");
    TestServiceAsyncClient client(std::move(channel));

    auto f = client.future_noResponse(1000);
    auto f2 = client.future_sendResponse(1000);
  }
}
