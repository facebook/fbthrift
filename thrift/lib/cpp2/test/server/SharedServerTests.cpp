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

#include <thrift/lib/cpp2/test/gen-cpp/TestService.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <thrift/lib/cpp/transport/THeader.h>

#include <thrift/lib/cpp2/async/StubSaslClient.h>
#include <thrift/lib/cpp2/async/StubSaslServer.h>
#include <thrift/lib/cpp2/test/util/TestInterface.h>

#include <thrift/lib/cpp2/test/util/TestThriftServerFactory.h>
#include <thrift/lib/cpp2/test/util/TestProxygenThriftServerFactory.h>

#include <thrift/lib/cpp2/test/util/TestHeaderClientChannelFactory.h>
#include <thrift/lib/cpp2/test/util/TestHTTPClientChannelFactory.h>

#include <folly/experimental/fibers/FiberManagerMap.h>
#include <wangle/concurrent/GlobalExecutor.h>

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using apache::thrift::protocol::PROTOCOL_TYPES;
using apache::thrift::test::TestServiceClient;

DECLARE_int32(thrift_cpp2_protocol_reader_string_limit);

namespace {

enum ThriftServerTypes {
  THRIFT_SERVER,
  PROXYGEN,
};

enum ClientChannelTypes {
  HEADER,
  HTTP2,
};

class SharedServerTests
    : public testing::TestWithParam<std::tuple<ThriftServerTypes,
                                               ClientChannelTypes,
                                               PROTOCOL_TYPES,
                                               THRIFT_SECURITY_POLICY>> {
 protected:
  void SetUp() override {
    base.reset(new folly::EventBase);

    auto protocolId = std::get<2>(GetParam());
    auto securityPolicy = std::get<3>(GetParam());

    switch (std::get<0>(GetParam())) {
      case THRIFT_SERVER: {
        auto f = folly::make_unique<TestThriftServerFactory<TestInterface>>();
        if (securityPolicy != THRIFT_SECURITY_DISABLED) {
          f->useStubSaslServer(true);
        }
        serverFactory = std::move(f);
        break;
      }
      case PROXYGEN: {
        ASSERT_EQ(THRIFT_SECURITY_DISABLED, securityPolicy);
        serverFactory = folly::make_unique<
            TestProxygenThriftServerFactory<TestInterface>>();
        break;
      }
      default:
        FAIL();
        break;
    }

    switch (std::get<1>(GetParam())) {
      case HEADER: {
        auto c = folly::make_unique<TestHeaderClientChannelFactory>();
        c->setProtocolId(protocolId);
        c->setSecurityPolicy(securityPolicy);
        channelFactory = std::move(c);
        break;
      }
      case HTTP2: {
        ASSERT_EQ(THRIFT_SECURITY_DISABLED, securityPolicy);
        auto c = folly::make_unique<TestHTTPClientChannelFactory>();
        c->setCodec(TestHTTPClientChannelFactory::Codec::HTTP2);
        c->setProtocolId(protocolId);
        channelFactory = std::move(c);
        break;
      }
      default:
        FAIL();
        break;
    }
  }

  void createServer() { server = serverFactory->create(); }

  void startServer() {
    if (!server) {
      createServer();
    }
    sst = folly::make_unique<ScopedServerThread>(server);
  }

  void createSocket() {
    if (!sst) {
      startServer();
    }
    socket = TAsyncTransport::UniquePtr(
        new TAsyncSocket(base.get(), *sst->getAddress()));
  }

  void createChannel() {
    if (!socket) {
      createSocket();
    }
    channel = channelFactory->create(std::move(socket));
  }

  void createClient() {
    if (!channel) {
      createChannel();
    }
    client = folly::make_unique<TestServiceAsyncClient>(std::move(channel));
  }

  void init() {
    createServer();
    startServer();
    createSocket();
    createChannel();
    createClient();
  }

  void TearDown() override {
    client.reset();
    channel.reset();
    socket.reset();
    sst.reset();
    server.reset();
    channelFactory.reset();
    serverFactory.reset();
    base.reset();
  }

 protected:
  std::unique_ptr<folly::EventBase> base;
  std::unique_ptr<TestServerFactory> serverFactory{nullptr};
  std::shared_ptr<TestClientChannelFactory> channelFactory{nullptr};

  std::shared_ptr<BaseThriftServer> server{nullptr};
  std::unique_ptr<ScopedServerThread> sst{nullptr};

  TAsyncTransport::UniquePtr socket{nullptr};
  apache::thrift::ClientChannel::Ptr channel{nullptr};
  std::unique_ptr<TestServiceAsyncClient> client{nullptr};
};
}

TEST_P(SharedServerTests, AsyncThrift2Test) {
  init();

  client->sendResponse([&](ClientReceiveState&& state) {
    std::string response;
    try {
      TestServiceAsyncClient::recv_sendResponse(response, state);
    } catch (const std::exception& ex) {
    }
    EXPECT_EQ(response, "test64");
    base->terminateLoopSoon();
  }, 64);
  base->loop();
}

TEST_P(SharedServerTests, GetLoadTest) {
  init();

  RpcOptions rpcOptions;
  rpcOptions.setWriteHeader("load", "thrift.active_requests");
  auto callback = std::unique_ptr<RequestCallback>(
      new FunctionReplyCallback([&](ClientReceiveState&& state) {
        std::string response;
        auto headers = state.header()->getHeaders();
        auto load = headers.find("load");
        EXPECT_NE(load, headers.end());
        EXPECT_EQ(load->second, "0");
        TestServiceAsyncClient::recv_wrapped_sendResponse(response, state);
        EXPECT_EQ(response, "test64");
        base->terminateLoopSoon();
      }));
  client->sendResponse(rpcOptions, std::move(callback), 64);
  base->loop();

  server->setGetLoad([&](std::string counter) {
    EXPECT_EQ(counter, "thrift.active_requests");
    return 1;
  });

  rpcOptions.setWriteHeader("load", "thrift.active_requests");
  callback = std::unique_ptr<RequestCallback>(
      new FunctionReplyCallback([&](ClientReceiveState&& state) {
        std::string response;
        auto headers = state.header()->getHeaders();
        auto load = headers.find("load");
        EXPECT_NE(load, headers.end());
        EXPECT_EQ(load->second, "1");
        TestServiceAsyncClient::recv_wrapped_sendResponse(response, state);
        EXPECT_EQ(response, "test64");
        base->terminateLoopSoon();
      }));
  client->sendResponse(rpcOptions, std::move(callback), 64);
  base->loop();
}

TEST_P(SharedServerTests, SerializationInEventBaseTest) {
  init();

  std::string response;
  client->sync_serializationTest(response, true);
  EXPECT_EQ("hello world", response);
}

TEST_P(SharedServerTests, HandlerInEventBaseTest) {
  init();

  std::string response;
  client->sync_eventBaseAsync(response);
  EXPECT_EQ("hello world", response);
}

bool compareIOBufChain(const folly::IOBuf* buf1, const folly::IOBuf* buf2) {
  folly::io::Cursor c1(buf1);
  folly::io::Cursor c2(buf2);
  std::pair<const uint8_t *, size_t> p1, p2;
  while (1) {
    if (p1.second == 0) {
      p1 = c1.peek();
      c1.skip(p1.second);
    }
    if (p2.second == 0) {
      p2 = c2.peek();
      c2.skip(p2.second);
    }
    if (p1.second == 0 || p2.second == 0) {
      // one is finished, the other must be finished too
      return p1.second == 0 && p2.second == 0;
    }

    size_t m = std::min(p1.second, p2.second);
    if (memcmp(p1.first, p2.first, m) != 0) {
      return false;
    }
    p1.first += m;
    p1.second -= m;
    p2.first += m;
    p2.second -= m;
  }
}

TEST_P(SharedServerTests, LargeSendTest) {
  channelFactory->setTimeout(20000);
  init();

  std::unique_ptr<folly::IOBuf> response;
  std::unique_ptr<folly::IOBuf> request;

  constexpr size_t oneMB = 1 << 20;
  std::string oneMBBuf;
  oneMBBuf.reserve(oneMB);
  for (uint32_t i = 0; i < (1 << 20) / 32; i++) {
    oneMBBuf.append(32, char(i & 0xff));
  }
  ASSERT_EQ(oneMBBuf.size(), oneMB);

  // A bit more than 1GiB, which used to be the max frame size
  constexpr size_t hugeSize = (size_t(1) << 30) + (1 << 10);
  request = folly::IOBuf::wrapBuffer(oneMBBuf.data(), oneMB);
  for (uint32_t i = 0; i < hugeSize / oneMB - 1; i++) {
    request->prependChain(folly::IOBuf::wrapBuffer(oneMBBuf.data(), oneMB));
  }
  request->prependChain(
      folly::IOBuf::wrapBuffer(oneMBBuf.data(), hugeSize % oneMB));
  ASSERT_EQ(request->computeChainDataLength(), hugeSize);

  client->sync_echoIOBuf(response, *request);
  ASSERT_EQ(request->computeChainDataLength() + kEchoSuffix.size(),
            response->computeChainDataLength());

  // response = request + kEchoSuffix. Make sure it's so
  request->prependChain(
      folly::IOBuf::wrapBuffer(kEchoSuffix.data(), kEchoSuffix.size()));
  // Not EXPECT_EQ; do you want to print two >1GiB strings on error?
  EXPECT_TRUE(compareIOBufChain(request.get(), response.get()));
}

TEST_P(SharedServerTests, OverloadTest) {
  const int numThreads = 1;
  const int queueSize = 10;
  auto tm = concurrency::ThreadManager::newSimpleThreadManager(
      numThreads, 0, false, queueSize);
  tm->threadFactory(std::make_shared<concurrency::PosixThreadFactory>());
  tm->start();
  serverFactory->useSimpleThreadManager(false);
  serverFactory->useThreadManager(tm);

  init();

  std::string response;

  auto tval = 10000;
  int too_full = 0;
  int exception_headers = 0;
  int completed = 0;
  auto lambda = [&](ClientReceiveState&& state) {
    std::string response;
    auto headers = state.header()->getHeaders();
    auto exHeader = headers.find("ex");
    if (exHeader != headers.end()) {
      EXPECT_EQ(kQueueOverloadedErrorCode, exHeader->second);
      exception_headers++;
    }
    auto ew =
        TestServiceAsyncClient::recv_wrapped_sendResponse(response, state);
    if (ew) {
      usleep(tval); // Wait for large task to finish
      too_full++;
    }

    completed++;

    if (completed == (numThreads + queueSize + 1)) {
      base->terminateLoopSoon();
    }
  };

  // Fill up the server's request buffer
  client->sendResponse(lambda, tval);
  for (int i = 0; i < numThreads + queueSize; i++) {
    client->sendResponse(lambda, 0);
  }
  base->loop();

  // We expect one 'too full' exception (queue size is 2, one
  // being worked on)
  // And three timeouts
  EXPECT_EQ(1, too_full);
  EXPECT_EQ(1, exception_headers);
}

TEST_P(SharedServerTests, OnewaySyncClientTest) {
  init();

  client->sync_noResponse(0);
}

TEST_P(SharedServerTests, ThriftServerSizeLimits) {
  init();

  google::FlagSaver flagSaver;
  FLAGS_thrift_cpp2_protocol_reader_string_limit = 1024 * 1024;

  std::string response;

  try {
    // make a largest possible input which should not throw an exception
    std::string smallInput(1 << 19, '1');
    client->sync_echoRequest(response, smallInput);
    SUCCEED();
  } catch (const std::exception& ex) {
    ADD_FAILURE();
  }

  // make an input that is too large by 1 byte
  std::string largeInput(1 << 21, '1');
  EXPECT_THROW(client->sync_echoRequest(response, largeInput), std::exception);
}

namespace {
class MyExecutor : public folly::Executor {
 public:
  void add(std::function<void()> f) override {
    calls++;
    f();
  }

  std::atomic<int> calls{0};
};
}

TEST_P(SharedServerTests, PoolExecutorTest) {
  auto exe = std::make_shared<MyExecutor>();
  serverFactory->useSimpleThreadManager(false)
      .useThreadManager(std::make_shared<
          apache::thrift::concurrency::ThreadManagerExecutorAdapter>(exe));

  init();

  std::string response;

  client->sync_echoRequest(response, "test");
  EXPECT_EQ(1, exe->calls);
}

namespace {
class FiberExecutor : public folly::Executor {
 public:
  void add(std::function<void()> f) override {
    folly::fibers::getFiberManager(*wangle::getEventBase()).add(f);
  }
};
}

TEST_P(SharedServerTests, FiberExecutorTest) {
  auto exe = std::make_shared<
      apache::thrift::concurrency::ThreadManagerExecutorAdapter>(
      std::make_shared<FiberExecutor>());
  serverFactory->useSimpleThreadManager(false);
  serverFactory->useThreadManager(exe);

  init();

  std::string response;

  client->sync_sendResponse(response, 1);
  EXPECT_EQ("test1", response);
}

TEST_P(SharedServerTests, FreeCallbackTest) {
  init();

  RpcOptions options;
  options.setTimeout(std::chrono::milliseconds(1));

  try {
    client->sync_notCalledBack(options);
  } catch (...) {
    // Expect timeout
    return;
  }
  ADD_FAILURE();
}

namespace {
class TestServerEventHandler
    : public server::TServerEventHandler,
      public TProcessorEventHandler,
      public TProcessorEventHandlerFactory,
      public std::enable_shared_from_this<TestServerEventHandler> {
 public:
  std::shared_ptr<TProcessorEventHandler> getEventHandler() override {
    return shared_from_this();
  }

  void check() { EXPECT_EQ(8, count); }
  void preServe(const folly::SocketAddress*) override { EXPECT_EQ(0, count++); }
  void newConnection(TConnectionContext*) override { EXPECT_EQ(1, count++); }
  void connectionDestroyed(TConnectionContext*) override {
    EXPECT_EQ(7, count++);
  }

  void* getContext(const char*, TConnectionContext*) override {
    EXPECT_EQ(2, count++);
    return nullptr;
  }
  void freeContext(void*, const char*) override { EXPECT_EQ(6, count++); }
  void preRead(void*, const char*) override { EXPECT_EQ(3, count++); }
  void onReadData(void*, const char*, const SerializedMessage&) override {
    EXPECT_EQ(4, count++);
  }

  void postRead(void*, const char*, THeader*, uint32_t) override {
    EXPECT_EQ(5, count++);
  }

 private:
  std::atomic<int> count{0};
};
}

TEST_P(SharedServerTests, CallbackOrderingTest) {
  auto serverHandler = std::make_shared<TestServerEventHandler>();
  TProcessorBase::addProcessorEventHandlerFactory(serverHandler);
  serverFactory->setServerEventHandler(serverHandler);

  init();

  auto channel = static_cast<ClientChannel*>(client->getChannel());
  auto socket = channel->getTransport();
  client->noResponse([](ClientReceiveState&& state) {}, 1000);
  base->tryRunAfterDelay([&]() { socket->closeNow(); }, 100);
  base->tryRunAfterDelay([&]() { base->terminateLoopSoon(); }, 500);
  base->loopForever();
  serverHandler->check();
  TProcessorBase::removeProcessorEventHandlerFactory(serverHandler);
}

using testing::Combine;
using testing::Values;

INSTANTIATE_TEST_CASE_P(ThriftServerTests,
                        SharedServerTests,
                        Combine(Values(ThriftServerTypes::THRIFT_SERVER),
                                Values(ClientChannelTypes::HEADER),
                                Values(protocol::T_BINARY_PROTOCOL,
                                       protocol::T_COMPACT_PROTOCOL),
                                Values(THRIFT_SECURITY_DISABLED,
                                       THRIFT_SECURITY_PERMITTED,
                                       THRIFT_SECURITY_REQUIRED)));

INSTANTIATE_TEST_CASE_P(ProxygenThriftServerTests,
                        SharedServerTests,
                        Combine(Values(ThriftServerTypes::PROXYGEN),
                                Values(ClientChannelTypes::HTTP2),
                                Values(protocol::T_BINARY_PROTOCOL,
                                       protocol::T_COMPACT_PROTOCOL),
                                Values(THRIFT_SECURITY_DISABLED)));
