/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <folly/portability/GTest.h>
#include <folly/synchronization/Baton.h>

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>

#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/AsyncTransport.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/util/ScopedServerThread.h>

#include <thrift/lib/cpp2/test/util/TestInterface.h>

#include <thrift/lib/cpp2/test/util/TestProxygenThriftServerFactory.h>
#include <thrift/lib/cpp2/test/util/TestThriftServerFactory.h>

#include <thrift/lib/cpp2/test/util/TestHTTPClientChannelFactory.h>
#include <thrift/lib/cpp2/test/util/TestHeaderClientChannelFactory.h>

#include <folly/CancellationToken.h>
#include <folly/Optional.h>
#include <folly/Synchronized.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/EventBase.h>

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

using namespace apache::thrift;
using namespace apache::thrift::test;
using namespace apache::thrift::util;
using namespace apache::thrift::transport;
using apache::thrift::protocol::PROTOCOL_TYPES;
using namespace std::literals;

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

class SharedServerTestInterface;

class NotCalledBackHandler {
 public:
  explicit NotCalledBackHandler(std::unique_ptr<HandlerCallback<void>> callback)
      : thriftCallback_{std::move(callback)},
        cancelCallback_(
            thriftCallback_->getConnectionContext()
                ->getConnectionContext()
                ->getCancellationToken(),
            [this]() { requestCancelled(); }) {}

  folly::Baton<> cancelBaton;

 private:
  void requestCancelled() {
    // Invoke the thrift callback once the request has canceled.
    // Even after the request has been canceled handlers still should eventually
    // invoke the request callback.
    std::exchange(thriftCallback_, nullptr)
        ->exception(std::runtime_error("request cancelled"));
    cancelBaton.post();
  }

  std::unique_ptr<HandlerCallback<void>> thriftCallback_;
  folly::CancellationCallback cancelCallback_;
};

class SharedServerTestInterface : public TestInterface {
 public:
  using NotCalledBackHandlers =
      std::vector<std::shared_ptr<NotCalledBackHandler>>;

  void async_tm_notCalledBack(
      std::unique_ptr<HandlerCallback<void>> cb) override {
    auto handler = std::make_shared<NotCalledBackHandler>(std::move(cb));
    notCalledBackHandlers_.lock()->push_back(std::move(handler));
    handlersCV_.notify_one();
  }

  /**
   * Get all handlers for currently pending notCalledBack() thrift calls.
   *
   * If there is no call currently pending this function will wait for up to the
   * specified timeout for one to arrive.  If the timeout expires before a
   * notCalledBack() call is received an empty result set will be returned.
   */
  NotCalledBackHandlers getNotCalledBackHandlers(
      std::chrono::milliseconds timeout) {
    auto end_time = std::chrono::steady_clock::now() + timeout;

    NotCalledBackHandlers results;
    auto handlers = notCalledBackHandlers_.lock();
    if (!handlersCV_.wait_until(handlers.getUniqueLock(), end_time, [&] {
          return !handlers->empty();
        })) {
      // If we get here we timed out.
      // Just return an empty result set in this case.
      return results;
    }
    results.swap(*handlers);
    return results;
  }

 private:
  folly::Synchronized<NotCalledBackHandlers, std::mutex> notCalledBackHandlers_;
  std::condition_variable handlersCV_;
};

class SharedServerTests
    : public testing::TestWithParam<
          std::tuple<ThriftServerTypes, ClientChannelTypes, PROTOCOL_TYPES>> {
 protected:
  void SetUp() override {
    base.reset(new folly::EventBase);

    auto protocolId = std::get<2>(GetParam());

    switch (std::get<0>(GetParam())) {
      case THRIFT_SERVER: {
        auto f = std::make_unique<
            TestThriftServerFactory<SharedServerTestInterface>>();
        serverFactory = std::move(f);
        break;
      }
      case PROXYGEN: {
        serverFactory = std::make_unique<
            TestProxygenThriftServerFactory<SharedServerTestInterface>>();
        break;
      }
      default:
        FAIL();
    }

    switch (std::get<1>(GetParam())) {
      case HEADER: {
        auto c = std::make_unique<TestHeaderClientChannelFactory>();
        c->setProtocolId(protocolId);
        channelFactory = std::move(c);
        break;
      }
      case HTTP2: {
        auto c = std::make_unique<TestHTTPClientChannelFactory>();
        c->setProtocolId(protocolId);
        channelFactory = std::move(c);
        break;
      }
      default:
        FAIL();
    }
  }

  void createServer() {
    server = serverFactory->create();
  }

  void startServer() {
    if (!server) {
      createServer();
    }
    sst = std::make_unique<ScopedServerThread>(server);
  }

  void createSocket() {
    if (!sst) {
      startServer();
    }
    socket = folly::AsyncTransport::UniquePtr(
        new folly::AsyncSocket(base.get(), *sst->getAddress()));
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
    client = std::make_unique<TestServiceAsyncClient>(std::move(channel));
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

  folly::AsyncTransport::UniquePtr socket{nullptr};
  apache::thrift::ClientChannel::Ptr channel{nullptr};
  std::unique_ptr<TestServiceAsyncClient> client{nullptr};
};
} // namespace

TEST_P(SharedServerTests, AsyncThrift2Test) {
  init();

  client->sendResponse(
      [&](ClientReceiveState&& state) {
        std::string response;
        try {
          TestServiceAsyncClient::recv_sendResponse(response, state);
        } catch (const std::exception&) {
        }
        EXPECT_EQ(response, "test64");
        base->terminateLoopSoon();
      },
      64);
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
        EXPECT_NE(load->second, "");
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
        EXPECT_NE(load->second, "");
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
  folly::ByteRange b1;
  folly::ByteRange b2;
  while (1) {
    if (b1.empty()) {
      b1 = c1.peekBytes();
      c1.skip(b1.size());
    }
    if (b2.empty()) {
      b2 = c2.peekBytes();
      c2.skip(b2.size());
    }
    if (b1.empty() || b2.empty()) {
      // one is finished, the other must be finished too
      return b1.empty() && b2.empty();
    }

    size_t m = std::min(b1.size(), b2.size());
    if (memcmp(b1.data(), b2.data(), m) != 0) {
      return false;
    }
    b1.advance(m);
    b2.advance(m);
  }
}

TEST_P(SharedServerTests, LargeSendTest) {
  channelFactory->setTimeout(45000);
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
  ASSERT_EQ(
      request->computeChainDataLength() + kEchoSuffix.size(),
      response->computeChainDataLength());

  // response = request + kEchoSuffix. Make sure it's so
  request->prependChain(
      folly::IOBuf::wrapBuffer(kEchoSuffix.data(), kEchoSuffix.size()));
  // Not EXPECT_EQ; do you want to print two >1GiB strings on error?
  EXPECT_TRUE(compareIOBufChain(request.get(), response.get()));
}

TEST_P(SharedServerTests, OnewaySyncClientTest) {
  init();

  client->sync_noResponse(0);
}

TEST_P(SharedServerTests, ThriftServerSizeLimits) {
  init();

  gflags::FlagSaver flagSaver;
  FLAGS_thrift_cpp2_protocol_reader_string_limit = 1024 * 1024;

  std::string response;

  // make a largest possible input which should not throw an exception
  std::string smallInput(1 << 19, '1');
  client->sync_echoRequest(response, smallInput);

  // make an input that is too large by 1 byte
  std::string largeInput(1 << 21, '1');
  EXPECT_THROW(client->sync_echoRequest(response, largeInput), std::exception);
}

namespace {
class MyExecutor : public folly::Executor {
 public:
  void add(folly::Func f) override {
    calls++;
    f();
  }

  std::atomic<int> calls{0};
};
} // namespace

TEST_P(SharedServerTests, PoolExecutorTest) {
  auto exe = std::make_shared<MyExecutor>();
  serverFactory->useSimpleThreadManager(false).useThreadManager(
      std::make_shared<
          apache::thrift::concurrency::ThreadManagerExecutorAdapter>(exe));

  init();

  std::string response;

  client->sync_echoRequest(response, "test");
  EXPECT_EQ(1, exe->calls);
}

namespace {
class FiberExecutor : public folly::Executor {
 public:
  void add(folly::Func f) override {
    folly::fibers::getFiberManager(*folly::getEventBase()).add(std::move(f));
  }
};
} // namespace

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

  void check() {
    EXPECT_EQ(8, count);
  }
  void preServe(const folly::SocketAddress*) override {
    EXPECT_EQ(0, count++);
  }
  void newConnection(TConnectionContext*) override {
    EXPECT_EQ(1, count++);
  }
  void connectionDestroyed(TConnectionContext*) override {
    EXPECT_EQ(7, count++);
  }

  void* getContext(const char*, TConnectionContext*) override {
    EXPECT_EQ(2, count++);
    return nullptr;
  }
  void freeContext(void*, const char*) override {
    EXPECT_EQ(6, count++);
  }
  void preRead(void*, const char*) override {
    EXPECT_EQ(3, count++);
  }
  void onReadData(void*, const char*, const SerializedMessage&) override {
    EXPECT_EQ(4, count++);
  }

  void postRead(void*, const char*, THeader*, uint32_t) override {
    EXPECT_EQ(5, count++);
  }

 private:
  std::atomic<int> count{0};
};
} // namespace

TEST_P(SharedServerTests, CallbackOrderingTest) {
  auto serverHandler = std::make_shared<TestServerEventHandler>();
  TProcessorBase::addProcessorEventHandlerFactory(serverHandler);
  serverFactory->setServerEventHandler(serverHandler);

  init();

  auto channel = static_cast<ClientChannel*>(client->getChannel());
  auto socket = channel->getTransport();
  client->noResponse([](ClientReceiveState&&) {}, 1000);
  base->tryRunAfterDelay([&]() { socket->closeNow(); }, 100);
  base->tryRunAfterDelay([&]() { base->terminateLoopSoon(); }, 500);
  base->loopForever();
  serverHandler->check();
  TProcessorBase::removeProcessorEventHandlerFactory(serverHandler);
}

TEST_P(SharedServerTests, CancellationTest) {
  init();

  auto interface = std::dynamic_pointer_cast<SharedServerTestInterface>(
      server->getProcessorFactory());
  ASSERT_TRUE(interface);
  EXPECT_EQ(0, interface->getNotCalledBackHandlers(0s).size());

  // Make a call to notCalledBack(), which will time out since the server never
  // reponds to this API.
  try {
    RpcOptions rpcOptions;
    rpcOptions.setTimeout(std::chrono::milliseconds(10));
    client->sync_notCalledBack(rpcOptions);
    EXPECT_FALSE(true) << "request should have never returned";
  } catch (const TTransportException& ex) {
    // ThriftServer and ProxygenThriftServer unfortunately differ in the
    // exception type generated here: ThriftServer generates a
    // TTransportException with a TIMED_OUT error code.
    if (ex.getType() != TTransportException::TIMED_OUT) {
      throw;
    }
  } catch (const TApplicationException& ex) {
    // ProxygenThriftServer generates a TApplicationException with a code of
    // TIMEOUT.
    if (ex.getType() != TApplicationException::TIMEOUT) {
      throw;
    }
  }

  // Wait for the server to register the call
  auto handlers = interface->getNotCalledBackHandlers(10s);
  ASSERT_EQ(1, handlers.size()) << "expected a single notCalledBack() call";
  // We haven't closed the client yet, so for ThriftServer we don't expect the
  // cancelBaton to have been signaled yet.  However, ProxygenThriftServer
  // triggers cancellation on a per-request basis rather than a per-connection
  // basis, so the cancelBaton may have already been canceled when our request
  // timed out above.

  // Close the client.  This should trigger request cancellation on the server.
  client.reset();

  // The handler's cancellation token should be triggered when we close the
  // connection.
  auto wasCancelled = handlers[0]->cancelBaton.try_wait_for(10s);
  EXPECT_TRUE(wasCancelled);
}

using testing::Combine;
using testing::Values;

INSTANTIATE_TEST_CASE_P(
    ThriftServerTests,
    SharedServerTests,
    Combine(
        Values(ThriftServerTypes::THRIFT_SERVER),
        Values(ClientChannelTypes::HEADER),
        Values(protocol::T_BINARY_PROTOCOL, protocol::T_COMPACT_PROTOCOL)));

INSTANTIATE_TEST_CASE_P(
    ProxygenThriftServerTests,
    SharedServerTests,
    Combine(
        Values(ThriftServerTypes::PROXYGEN),
        Values(ClientChannelTypes::HTTP2),
        Values(protocol::T_BINARY_PROTOCOL, protocol::T_COMPACT_PROTOCOL)));
