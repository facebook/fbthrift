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

#include <memory>

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <fmt/core.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <folly/Conv.h>
#include <folly/Memory.h>
#include <folly/Optional.h>
#include <folly/Range.h>
#include <folly/executors/GlobalExecutor.h>
#include <folly/io/GlobalShutdownSocketSet.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/AsyncSocketException.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/test/TestSSLServer.h>
#include <wangle/acceptor/ServerSocketConfig.h>

#include <folly/io/async/AsyncSocket.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/HTTPClientChannel.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/Cpp2Connection.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/test/util/TestHeaderClientChannelFactory.h>
#include <thrift/lib/cpp2/test/util/TestInterface.h>
#include <thrift/lib/cpp2/test/util/TestThriftServerFactory.h>
#include <thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <thrift/lib/cpp2/util/ScopedServerThread.h>

using namespace apache::thrift;
using namespace apache::thrift::test;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using std::string;

DECLARE_int32(thrift_cpp2_protocol_reader_string_limit);

std::unique_ptr<HTTP2RoutingHandler> createHTTP2RoutingHandler(
    ThriftServer& server) {
  auto h2_options = std::make_unique<proxygen::HTTPServerOptions>();
  h2_options->threads = static_cast<size_t>(server.getNumIOWorkerThreads());
  h2_options->idleTimeout = server.getIdleTimeout();
  h2_options->shutdownOn = {SIGINT, SIGTERM};
  return std::make_unique<HTTP2RoutingHandler>(
      std::move(h2_options), server.getThriftProcessor(), server);
}

TEST(ThriftServer, H2ClientAddressTest) {
  class EchoClientAddrTestInterface : public TestServiceSvIf {
    void sendResponse(std::string& _return, int64_t /* size */) override {
      _return = getConnectionContext()->getPeerAddress()->describe();
    }
  };

  ScopedServerInterfaceThread runner(
      std::make_shared<EchoClientAddrTestInterface>());
  auto& thriftServer = dynamic_cast<ThriftServer&>(runner.getThriftServer());
  thriftServer.addRoutingHandler(createHTTP2RoutingHandler(thriftServer));

  folly::EventBase base;
  folly::AsyncSocket::UniquePtr socket(
      new folly::AsyncSocket(&base, runner.getAddress()));
  TestServiceAsyncClient client(
      HTTPClientChannel::newHTTP2Channel(std::move(socket)));
  auto channel =
      boost::polymorphic_downcast<HTTPClientChannel*>(client.getChannel());

  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, channel->getTransport()->getLocalAddress().describe());
}

TEST(ThriftServer, OnewayClientConnectionCloseTest) {
  static std::atomic<bool> done(false);

  class OnewayTestInterface : public TestServiceSvIf {
    void noResponse(int64_t size) override {
      usleep(size);
      done = true;
    }
  };

  TestThriftServerFactory<OnewayTestInterface> factory2;
  ScopedServerThread st(factory2.create());

  {
    folly::EventBase base;
    std::shared_ptr<folly::AsyncSocket> socket(
        folly::AsyncSocket::newSocket(&base, *st.getAddress()));
    TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

    client.sync_noResponse(10000);
  } // client out of scope

  usleep(50000);
  EXPECT_TRUE(done);
}

TEST(ThriftServer, OnewayDeferredHandlerTest) {
  class OnewayTestInterface : public TestServiceSvIf {
   public:
    folly::Baton<> done;

    folly::Future<folly::Unit> future_noResponse(int64_t size) override {
      auto tm = getThreadManager();
      auto ctx = getConnectionContext();
      return folly::futures::sleep(std::chrono::milliseconds(size))
          .via(tm)
          .thenValue(
              [ctx](auto&&) { EXPECT_EQ("noResponse", ctx->getMethodName()); })
          .thenValue([this](auto&&) { done.post(); });
    }
  };

  auto handler = std::make_shared<OnewayTestInterface>();
  ScopedServerInterfaceThread runner(handler);

  folly::EventBase eb;
  handler->done.reset();
  auto client = runner.newClient<TestServiceAsyncClient>(eb);
  client->sync_noResponse(100);
  ASSERT_TRUE(handler->done.try_wait_for(std::chrono::seconds(1)));
}

TEST(ThriftServer, CompressionClientTest) {
  TestThriftServerFactory<TestInterface> factory;
  ScopedServerThread sst(factory.create());
  folly::EventBase base;
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  auto channel =
      boost::polymorphic_downcast<HeaderClientChannel*>(client.getChannel());
  channel->setTransform(apache::thrift::transport::THeader::ZLIB_TRANSFORM);
  channel->setMinCompressBytes(1);

  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "test64");

  auto trans = channel->getWriteTransforms();
  EXPECT_EQ(trans.size(), 1);
  for (auto& tran : trans) {
    EXPECT_EQ(tran, apache::thrift::transport::THeader::ZLIB_TRANSFORM);
  }
}

TEST(ThriftServer, ResponseTooBigTest) {
  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  runner.getThriftServer().setMaxResponseSize(4096);
  folly::EventBase eb;
  auto client = runner.newClient<TestServiceAsyncClient>(eb);

  std::string request(4096, 'a');
  std::string response;
  try {
    client->sync_echoRequest(response, request);
    ADD_FAILURE() << "should throw";
  } catch (const TApplicationException& tae) {
    EXPECT_EQ(
        tae.getType(),
        TApplicationException::TApplicationExceptionType::INTERNAL_ERROR);
  } catch (...) {
    ADD_FAILURE() << "unexpected exception thrown";
  }
}

class TestConnCallback : public folly::AsyncSocket::ConnectCallback {
 public:
  void connectSuccess() noexcept override {}

  void connectErr(const folly::AsyncSocketException& ex) noexcept override {
    exception.reset(new folly::AsyncSocketException(ex));
  }

  std::unique_ptr<folly::AsyncSocketException> exception;
};

TEST(ThriftServer, SSLClientOnPlaintextServerTest) {
  TestThriftServerFactory<TestInterface> factory;
  ScopedServerThread sst(factory.create());
  folly::EventBase base;
  auto sslCtx = std::make_shared<SSLContext>();
  std::shared_ptr<folly::AsyncSocket> socket(
      TAsyncSSLSocket::newSocket(sslCtx, &base));
  TestConnCallback cb;
  socket->connect(&cb, *sst.getAddress());
  base.loop();
  ASSERT_TRUE(cb.exception);
  auto msg = cb.exception->what();
  EXPECT_NE(nullptr, strstr(msg, "unexpected message"));
}

TEST(ThriftServer, CompressionServerTest) {
  /* This tests the boundary condition of uncompressed value being larger
     than minCompressBytes and compressed value being smaller. We want to ensure
     this case does not cause corruption */
  TestThriftServerFactory<TestInterface> factory;
  factory.minCompressBytes(100);
  ScopedServerThread sst(factory.create());
  folly::EventBase base;
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  auto channel =
      boost::polymorphic_downcast<HeaderClientChannel*>(client.getChannel());
  channel->setTransform(apache::thrift::transport::THeader::ZLIB_TRANSFORM);

  std::string request(55, 'a');
  std::string response;
  // The response is slightly more than 100 bytes before compression
  // and less than 100 bytes after compression
  client.sync_echoRequest(response, request);
  EXPECT_EQ(response.size(), 100);
}

TEST(ThriftServer, DefaultCompressionTest) {
  /* Tests the functionality of default transforms, ensuring the server properly
     applies them even if the client does not apply any transforms. */
  class Callback : public RequestCallback {
   public:
    explicit Callback(bool compressionExpected, uint16_t expectedTransform)
        : compressionExpected_(compressionExpected),
          expectedTransform_(expectedTransform) {}

   private:
    void requestSent() override {}

    void replyReceived(ClientReceiveState&& state) override {
      auto trans = state.header()->getTransforms();
      if (compressionExpected_) {
        EXPECT_EQ(trans.size(), 1);
        for (auto& tran : trans) {
          EXPECT_EQ(tran, expectedTransform_);
        }
      } else {
        EXPECT_EQ(trans.size(), 0);
      }
    }
    void requestError(ClientReceiveState&& state) override {
      state.exception().throw_exception();
    }
    bool compressionExpected_;
    uint16_t expectedTransform_;
  };

  TestThriftServerFactory<TestInterface> factory;
  factory.minCompressBytes(1);
  factory.defaultWriteTransform(
      apache::thrift::transport::THeader::ZLIB_TRANSFORM);
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());
  ScopedServerThread sst(server);
  folly::EventBase base;

  // First, with minCompressBytes set low, ensure we compress even though the
  // client did not compress
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));
  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));
  client.sendResponse(
      std::make_unique<Callback>(
          true, apache::thrift::transport::THeader::ZLIB_TRANSFORM),
      64);
  base.loop();

  // Ensure that client transforms take precedence
  auto channel =
      boost::polymorphic_downcast<HeaderClientChannel*>(client.getChannel());
  channel->setTransform(apache::thrift::transport::THeader::SNAPPY_TRANSFORM);
  client.sendResponse(
      std::make_unique<Callback>(
          true, apache::thrift::transport::THeader::SNAPPY_TRANSFORM),
      64);
  base.loop();

  // Ensure that minCompressBytes still works with default transforms. We
  // Do not expect compression
  server->setMinCompressBytes(1000);
  std::shared_ptr<folly::AsyncSocket> socket2(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));
  TestServiceAsyncClient client2(HeaderClientChannel::newChannel(socket2));
  client2.sendResponse(std::make_unique<Callback>(false, 0), 64);
  base.loop();
}

TEST(ThriftServer, HeaderTest) {
  TestThriftServerFactory<TestInterface> factory;
  auto serv = factory.create();
  ScopedServerThread sst(serv);
  folly::EventBase base;
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  RpcOptions options;
  // Set it as a header directly so the client channel won't set a
  // timeout and the test won't throw TTransportException
  options.setWriteHeader(
      apache::thrift::transport::THeader::CLIENT_TIMEOUT_HEADER,
      folly::to<std::string>(10));
  try {
    client.sync_processHeader(options);
    ADD_FAILURE() << "should timeout";
  } catch (const TApplicationException& e) {
    EXPECT_EQ(
        e.getType(), TApplicationException::TApplicationExceptionType::TIMEOUT);
  }
}

namespace {
template <class MakeClientFunc>
void doLoadHeaderTest(MakeClientFunc&& makeClient) {
  static constexpr int kEmptyMetricLoad = 12345;
  class Callback : public RequestCallback {
   public:
    explicit Callback(folly::Optional<std::string> loadMetric)
        : loadMetric_(std::move(loadMetric)) {}

   private:
    void requestSent() override {}

    void replyReceived(ClientReceiveState&& state) override {
      const auto& headers = state.header()->getHeaders();
      auto loadIter = headers.find(THeader::QUERY_LOAD_HEADER);
      ASSERT_EQ(loadMetric_.hasValue(), loadIter != headers.end());
      if (!loadMetric_) {
        return;
      }
      folly::StringPiece loadMetric(*loadMetric_);
      if (loadMetric.removePrefix("custom_load_metric_")) {
        EXPECT_EQ(loadMetric, loadIter->second);
      } else if (loadMetric.empty()) {
        EXPECT_EQ(folly::to<std::string>(kEmptyMetricLoad), loadIter->second);
      } else {
        FAIL() << "Unexpected load metric";
      }
    }

    void requestError(ClientReceiveState&&) override {
      ADD_FAILURE() << "The response should not be an error";
    }

    folly::Optional<std::string> loadMetric_;
  };

  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  folly::EventBase base;
  auto client = makeClient(runner, &base);

  {
    LOG(ERROR) << "========= no load header ==========";
    client->voidResponse(std::make_unique<Callback>(folly::none));
  }

  {
    LOG(ERROR) << "========= empty load header ==========";
    RpcOptions emptyLoadOptions;
    const std::string kLoadMetric;
    emptyLoadOptions.setWriteHeader(THeader::QUERY_LOAD_HEADER, kLoadMetric);
    client->voidResponse(
        emptyLoadOptions, std::make_unique<Callback>(kLoadMetric));
  }

  {
    LOG(ERROR) << "========= custom load header ==========";
    RpcOptions customLoadOptions;
    const std::string kLoadMetric{"custom_load_metric_789"};
    customLoadOptions.setWriteHeader(THeader::QUERY_LOAD_HEADER, kLoadMetric);
    client->voidResponse(
        customLoadOptions, std::make_unique<Callback>(kLoadMetric));
  }

  {
    LOG(ERROR) << "========= server overloaded ==========";
    RpcOptions customLoadOptions;
    // force overloaded
    runner.getThriftServer().setIsOverloaded(
        [](const auto*, const string* method) {
          EXPECT_EQ("voidResponse", *method);
          return true;
        });
    runner.getThriftServer().setGetLoad([](const std::string& metric) {
      folly::StringPiece metricPiece(metric);
      if (metricPiece.removePrefix("custom_load_metric_")) {
        return folly::to<int32_t>(metricPiece.toString());
      } else if (metricPiece.empty()) {
        return kEmptyMetricLoad;
      }
      ADD_FAILURE() << "Unexpected load metric on request";
      return -42;
    });
    const std::string kLoadMetric;
    customLoadOptions.setWriteHeader(THeader::QUERY_LOAD_HEADER, kLoadMetric);
    client->voidResponse(
        customLoadOptions, std::make_unique<Callback>(kLoadMetric));
  }

  base.loop();
}
} // namespace

TEST(ThriftServer, LoadHeaderTest_HeaderClientChannel) {
  doLoadHeaderTest([](auto& runner, auto* evb) {
    return runner.template newClient<TestServiceAsyncClient>(evb);
  });
}

TEST(ThriftServer, LoadHeaderTest_RocketClientChannel) {
  doLoadHeaderTest([](auto& runner, auto* evb) {
    return runner.template newClient<TestServiceAsyncClient>(
        evb, [](auto socket) mutable {
          return RocketClientChannel::newChannel(std::move(socket));
        });
  });
}

enum LatencyHeaderStatus {
  EXPECTED,
  NOT_EXPECTED,
};

static void validateLatencyHeaders(
    std::map<std::string, std::string> headers,
    LatencyHeaderStatus status) {
  bool isHeaderExpected = (status == LatencyHeaderStatus::EXPECTED);
  auto readLatency = folly::get_optional(headers, kReadLatencyHeader.str());
  ASSERT_EQ(isHeaderExpected, readLatency.has_value());
  auto queueLatency = folly::get_optional(headers, kQueueLatencyHeader.str());
  ASSERT_EQ(isHeaderExpected, queueLatency.has_value());
  auto processLatency =
      folly::get_optional(headers, kProcessLatencyHeader.str());
  ASSERT_EQ(isHeaderExpected, processLatency.has_value());
  if (isHeaderExpected) {
    EXPECT_GE(folly::to<int64_t>(readLatency.value()), 0);
    EXPECT_GE(folly::to<int64_t>(queueLatency.value()), 0);
    EXPECT_GE(folly::to<int64_t>(processLatency.value()), 0);
  }
}

TEST(ThriftServer, LatencyHeader_LoggingDisabled) {
  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  folly::EventBase base;
  auto client = runner.newClient<TestServiceAsyncClient>(&base);

  RpcOptions rpcOptions;
  client->sync_voidResponse(rpcOptions);
  validateLatencyHeaders(
      rpcOptions.getReadHeaders(), LatencyHeaderStatus::NOT_EXPECTED);
}

namespace {
template <class MakeClientFunc>
void doServerOverloadedTest(MakeClientFunc&& makeClient) {
  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  folly::EventBase base;
  auto client = makeClient(runner, &base);

  // force overloaded
  runner.getThriftServer().setIsOverloaded(
      [](const auto*, const string* method) {
        EXPECT_EQ("voidResponse", *method);
        return true;
      });

  // avoid compressing loadshedding errors even if compression is enabled
  static_cast<ThriftServer*>(&runner.getThriftServer())
      ->setMinCompressBytes(100);

  RpcOptions rpcOptions;
  rpcOptions.setWriteHeader(kClientLoggingHeader.str(), "");
  try {
    client->sync_voidResponse(rpcOptions);
    FAIL() << "Expected that the service call throws TApplicationException";
  } catch (const apache::thrift::TApplicationException& ex) {
    EXPECT_EQ(
        ex.getType(),
        apache::thrift::TApplicationException::TApplicationExceptionType::
            LOADSHEDDING);

    // Latency headers are NOT set, when server is overloaded
    validateLatencyHeaders(
        rpcOptions.getReadHeaders(), LatencyHeaderStatus::NOT_EXPECTED);
  } catch (...) {
    FAIL()
        << "Expected that the service call throws TApplicationException, got "
        << std::current_exception;
  }
}
} // namespace

TEST(ThriftServer, LatencyHeader_ServerOverloaded_HeaderClientChannel) {
  doServerOverloadedTest([](auto& runner, auto* evb) {
    return runner.template newClient<TestServiceAsyncClient>(evb);
  });
}

TEST(
    ThriftServer,
    LatencyHeader_ServerOverloaded_HeaderClientChannel_WithCompression) {
  doServerOverloadedTest([](auto& runner, auto* evb) {
    return runner.template newClient<TestServiceAsyncClient>(
        evb, [](auto socket) mutable {
          auto channel = HeaderClientChannel::newChannel(std::move(socket));
          channel->setTransform(
              apache::thrift::transport::THeader::ZSTD_TRANSFORM);
          return channel;
        });
  });
}

TEST(ThriftServer, LatencyHeader_ServerOverloaded_RocketClientChannel) {
  doServerOverloadedTest([](auto& runner, auto* evb) {
    return runner.template newClient<TestServiceAsyncClient>(
        evb, [](auto socket) mutable {
          return RocketClientChannel::newChannel(std::move(socket));
        });
  });
}

TEST(
    ThriftServer,
    LatencyHeader_ServerOverloaded_RocketClientChannel_WithCompression) {
  doServerOverloadedTest([](auto& runner, auto* evb) {
    return runner.template newClient<TestServiceAsyncClient>(
        evb, [](auto socket) mutable {
          auto channel = RocketClientChannel::newChannel(std::move(socket));
          channel->setNegotiatedCompressionAlgorithm(
              CompressionAlgorithm::ZSTD);
          return channel;
        });
  });
}

TEST(ThriftServer, LatencyHeader_ClientTimeout) {
  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  folly::EventBase base;
  auto client = runner.newClient<TestServiceAsyncClient>(&base);

  runner.getThriftServer().setUseClientTimeout(
      LatencyHeaderStatus::NOT_EXPECTED);

  RpcOptions rpcOptions;
  // Setup client timeout
  rpcOptions.setTimeout(std::chrono::milliseconds(5));
  rpcOptions.setWriteHeader(kClientLoggingHeader.str(), "");
  std::string response;
  EXPECT_ANY_THROW(client->sync_sendResponse(rpcOptions, response, 20000));

  // Latency headers are NOT set, when client times out.
  validateLatencyHeaders(
      rpcOptions.getReadHeaders(), LatencyHeaderStatus::NOT_EXPECTED);
}

TEST(ThriftServer, LatencyHeader_RequestSuccess) {
  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  folly::EventBase base;
  auto client = runner.newClient<TestServiceAsyncClient>(&base);

  RpcOptions rpcOptions;
  rpcOptions.setWriteHeader(kClientLoggingHeader.str(), "");
  client->sync_voidResponse(rpcOptions);
  validateLatencyHeaders(
      rpcOptions.getReadHeaders(), LatencyHeaderStatus::EXPECTED);
}

TEST(ThriftServer, LatencyHeader_RequestFailed) {
  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  folly::EventBase base;
  auto client = runner.newClient<TestServiceAsyncClient>(&base);

  RpcOptions rpcOptions;
  rpcOptions.setWriteHeader(kClientLoggingHeader.str(), "");
  EXPECT_ANY_THROW(client->sync_throwsHandlerException(rpcOptions));

  // Latency headers are set, when handler throws exception
  validateLatencyHeaders(
      rpcOptions.getReadHeaders(), LatencyHeaderStatus::EXPECTED);
}

TEST(ThriftServer, LatencyHeader_TaskExpiry) {
  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  folly::EventBase base;
  auto client = runner.newClient<TestServiceAsyncClient>(&base);

  // setup task expire timeout.
  runner.getThriftServer().setTaskExpireTime(std::chrono::milliseconds(10));
  runner.getThriftServer().setUseClientTimeout(false);

  RpcOptions rpcOptions;
  rpcOptions.setWriteHeader(kClientLoggingHeader.str(), "");
  std::string response;
  EXPECT_ANY_THROW(client->sync_sendResponse(rpcOptions, response, 30000));

  // Latency headers are set, when task expires
  validateLatencyHeaders(
      rpcOptions.getReadHeaders(), LatencyHeaderStatus::EXPECTED);
}

TEST(ThriftServer, LatencyHeader_QueueTimeout) {
  ScopedServerInterfaceThread runner(std::make_shared<TestInterface>());
  folly::EventBase base;
  auto client = runner.newClient<TestServiceAsyncClient>(&base);

  // setup timeout
  runner.getThriftServer().setQueueTimeout(std::chrono::milliseconds(5));

  // Run a long request.
  auto slowRequestFuture = client->future_sendResponse(20000);

  RpcOptions rpcOptions;
  rpcOptions.setWriteHeader(kClientLoggingHeader.str(), "");
  std::string response;
  EXPECT_ANY_THROW(client->sync_sendResponse(rpcOptions, response, 1000));

  // Latency headers are set, when server throws queue timeout
  validateLatencyHeaders(
      rpcOptions.getReadHeaders(), LatencyHeaderStatus::EXPECTED);

  std::move(slowRequestFuture).getVia(&base);
}

TEST(ThriftServer, ClientTimeoutTest) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = factory.create();
  ScopedServerThread sst(server);
  folly::EventBase base;

  auto getClient = [&base, &sst]() {
    std::shared_ptr<folly::AsyncSocket> socket(
        folly::AsyncSocket::newSocket(&base, *sst.getAddress()));

    return std::make_shared<TestServiceAsyncClient>(
        HeaderClientChannel::newChannel(socket));
  };

  int cbCtor = 0;
  int cbCall = 0;

  auto callback = [&cbCall, &cbCtor](
                      std::shared_ptr<TestServiceAsyncClient> client,
                      bool& timeout) {
    cbCtor++;
    return std::unique_ptr<RequestCallback>(new FunctionReplyCallback(
        [&cbCall, client, &timeout](ClientReceiveState&& state) {
          cbCall++;
          if (state.exception()) {
            timeout = true;
            auto ex = state.exception().get_exception();
            auto& e = dynamic_cast<TTransportException const&>(*ex);
            EXPECT_EQ(TTransportException::TIMED_OUT, e.getType());
            return;
          }
          try {
            std::string resp;
            client->recv_sendResponse(resp, state);
          } catch (const TApplicationException& e) {
            timeout = true;
            EXPECT_EQ(TApplicationException::TIMEOUT, e.getType());
            EXPECT_TRUE(
                state.header()->getFlags() & HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
            return;
          }
          timeout = false;
        }));
  };

  // Set the timeout to be 5 milliseconds, but the call will take 10 ms.
  // The server should send a timeout after 5 milliseconds
  RpcOptions options;
  options.setTimeout(std::chrono::milliseconds(5));
  auto client1 = getClient();
  bool timeout1;
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  EXPECT_TRUE(timeout1);
  usleep(10000);

  // This time we set the timeout to be 100 millseconds.  The server
  // should not time out
  options.setTimeout(std::chrono::milliseconds(100));
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  EXPECT_FALSE(timeout1);
  usleep(10000);

  // This time we set server timeout to be 5 millseconds.  However, the
  // task should start processing within that millisecond, so we should
  // not see an exception because the client timeout should be used after
  // processing is started
  server->setTaskExpireTime(std::chrono::milliseconds(5));
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  usleep(10000);

  // The server timeout stays at 5 ms, but we put the client timeout at
  // 5 ms.  We should timeout even though the server starts processing within
  // 5ms.
  options.setTimeout(std::chrono::milliseconds(5));
  client1->sendResponse(options, callback(client1, timeout1), 10000);
  base.loop();
  EXPECT_TRUE(timeout1);
  usleep(50000);

  // And finally, with the server timeout at 50 ms, we send 2 requests at
  // once.  Because the first request will take more than 50 ms to finish
  // processing (the server only has 1 worker thread), the second request
  // won't start processing until after 50ms, and will timeout, despite the
  // very high client timeout.
  // We don't know which one will timeout (race conditions) so we just check
  // the xor
  auto client2 = getClient();
  bool timeout2;
  server->setTaskExpireTime(std::chrono::milliseconds(50));
  options.setTimeout(std::chrono::milliseconds(110));
  client1->sendResponse(options, callback(client1, timeout1), 100000);
  client2->sendResponse(options, callback(client2, timeout2), 100000);
  base.loop();
  EXPECT_TRUE(timeout1 || timeout2);
  EXPECT_FALSE(timeout1 && timeout2);

  EXPECT_EQ(cbCall, cbCtor);
}

TEST(ThriftServer, ConnectionIdleTimeoutTest) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = factory.create();
  server->setIdleTimeout(std::chrono::milliseconds(20));
  apache::thrift::util::ScopedServerThread st(server);

  folly::EventBase base;
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *st.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  std::string response;
  client.sync_sendResponse(response, 200);
  EXPECT_EQ(response, "test200");
  base.loop();
}

TEST(ThriftServer, BadSendTest) {
  class Callback : public RequestCallback {
    void requestSent() override {
      ADD_FAILURE();
    }
    void replyReceived(ClientReceiveState&&) override {
      ADD_FAILURE();
    }
    void requestError(ClientReceiveState&& state) override {
      EXPECT_TRUE(state.exception());
      auto ex =
          state.exception()
              .get_exception<apache::thrift::transport::TTransportException>();
      ASSERT_TRUE(ex);
      EXPECT_THAT(
          ex->what(), testing::StartsWith("transport is closed in write()"));
    }
  };

  TestThriftServerFactory<TestInterface> factory;
  ScopedServerThread sst(factory.create());
  folly::EventBase base;
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  client.sendResponse(std::unique_ptr<RequestCallback>(new Callback), 64);

  socket->shutdownWriteNow();
  base.loop();

  std::string response;
  EXPECT_THROW(client.sync_sendResponse(response, 64), TTransportException);
}

TEST(ThriftServer, ResetStateTest) {
  folly::EventBase base;

  // Create a server socket and bind, don't listen.  This gets us a
  // port to test with which is guaranteed to fail.
  auto ssock = std::unique_ptr<
      folly::AsyncServerSocket,
      folly::DelayedDestruction::Destructor>(new folly::AsyncServerSocket);
  ssock->bind(0);
  EXPECT_FALSE(ssock->getAddresses().empty());

  // We do this loop a bunch of times, because the bug which caused
  // the assertion failure was a lost race, which doesn't happen
  // reliably.
  for (int i = 0; i < 1000; ++i) {
    std::shared_ptr<folly::AsyncSocket> socket(
        folly::AsyncSocket::newSocket(&base, ssock->getAddresses()[0]));

    // Create a client.
    TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

    std::string response;
    // This will fail, because there's no server.
    EXPECT_THROW(client.sync_sendResponse(response, 64), TTransportException);
    // On a failed client object, this should also throw an exception.
    // In the past, this would generate an assertion failure and
    // crash.
    EXPECT_THROW(client.sync_sendResponse(response, 64), TTransportException);
  }
}

TEST(ThriftServer, FailureInjection) {
  enum ExpectedFailure { NONE = 0, ERROR, TIMEOUT, DISCONNECT, END };

  std::atomic<ExpectedFailure> expected(NONE);

  using apache::thrift::transport::TTransportException;

  class Callback : public RequestCallback {
   public:
    explicit Callback(const std::atomic<ExpectedFailure>* expected)
        : expected_(expected) {}

   private:
    void requestSent() override {}

    void replyReceived(ClientReceiveState&& state) override {
      std::string response;
      try {
        TestServiceAsyncClient::recv_sendResponse(response, state);
        EXPECT_EQ(NONE, *expected_);
      } catch (const apache::thrift::TApplicationException&) {
        const auto& headers = state.header()->getHeaders();
        EXPECT_TRUE(
            headers.find("ex") != headers.end() &&
            headers.find("ex")->second == kInjectedFailureErrorCode);
        EXPECT_EQ(ERROR, *expected_);
      } catch (...) {
        ADD_FAILURE() << "Unexpected exception thrown";
      }

      // Now do it again with exception_wrappers.
      auto ew =
          TestServiceAsyncClient::recv_wrapped_sendResponse(response, state);
      if (ew) {
        EXPECT_TRUE(
            ew.is_compatible_with<apache::thrift::TApplicationException>());
        EXPECT_EQ(ERROR, *expected_);
      } else {
        EXPECT_EQ(NONE, *expected_);
      }
    }

    void requestError(ClientReceiveState&& state) override {
      ASSERT_TRUE(state.exception());
      auto ex_ = state.exception().get_exception();
      auto& ex = dynamic_cast<TTransportException const&>(*ex_);
      if (ex.getType() == TTransportException::TIMED_OUT) {
        EXPECT_EQ(TIMEOUT, *expected_);
      } else {
        EXPECT_EQ(DISCONNECT, *expected_);
      }
    }

    const std::atomic<ExpectedFailure>* expected_;
  };

  TestThriftServerFactory<TestInterface> factory;
  ScopedServerThread sst(factory.create());
  folly::EventBase base;
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  auto server = std::dynamic_pointer_cast<ThriftServer>(sst.getServer().lock());
  CHECK(server);
  SCOPE_EXIT {
    server->setFailureInjection(ThriftServer::FailureInjection());
  };

  RpcOptions rpcOptions;
  rpcOptions.setTimeout(std::chrono::milliseconds(100));
  for (int i = 0; i < END; ++i) {
    auto exp = static_cast<ExpectedFailure>(i);
    ThriftServer::FailureInjection fi;

    switch (exp) {
      case NONE:
        break;
      case ERROR:
        fi.errorFraction = 1;
        break;
      case TIMEOUT:
        fi.dropFraction = 1;
        break;
      case DISCONNECT:
        fi.disconnectFraction = 1;
        break;
      case END:
        LOG(FATAL) << "unreached";
    }

    server->setFailureInjection(std::move(fi));

    expected = exp;

    auto callback = std::make_unique<Callback>(&expected);
    client.sendResponse(rpcOptions, std::move(callback), 1);
    base.loop();
  }
}

TEST(ThriftServer, useExistingSocketAndExit) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());
  folly::AsyncServerSocket::UniquePtr serverSocket(
      new folly::AsyncServerSocket);
  serverSocket->bind(0);
  server->useExistingSocket(std::move(serverSocket));
  // In the past, this would cause a SEGV
}

TEST(ThriftServer, useExistingSocketAndConnectionIdleTimeout) {
  // This is ConnectionIdleTimeoutTest, but with an existing socket
  TestThriftServerFactory<TestInterface> factory;
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());
  folly::AsyncServerSocket::UniquePtr serverSocket(
      new folly::AsyncServerSocket);
  serverSocket->bind(0);
  server->useExistingSocket(std::move(serverSocket));

  server->setIdleTimeout(std::chrono::milliseconds(20));
  apache::thrift::util::ScopedServerThread st(server);

  folly::EventBase base;
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *st.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  std::string response;
  client.sync_sendResponse(response, 200);
  EXPECT_EQ(response, "test200");
  base.loop();
}

namespace {
class ReadCallbackTest : public folly::AsyncTransportWrapper::ReadCallback {
 public:
  void getReadBuffer(void**, size_t*) override {}
  void readDataAvailable(size_t) noexcept override {}
  void readEOF() noexcept override {
    eof = true;
  }

  void readErr(const folly::AsyncSocketException&) noexcept override {
    eof = true;
  }

  bool eof = false;
};
} // namespace

TEST(ThriftServer, ShutdownSocketSetTest) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());
  ScopedServerThread sst(server);
  folly::EventBase base;
  ReadCallbackTest cb;

  std::shared_ptr<folly::AsyncSocket> socket2(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));
  socket2->setReadCB(&cb);

  base.tryRunAfterDelay(
      [&]() { folly::tryGetShutdownSocketSet()->shutdownAll(); }, 10);
  base.tryRunAfterDelay([&]() { base.terminateLoopSoon(); }, 30);
  base.loopForever();
  EXPECT_EQ(cb.eof, true);
}

TEST(ThriftServer, ShutdownDegenarateServer) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = factory.create();
  server->setMaxRequests(1);
  server->setNumIOWorkerThreads(1);
  ScopedServerThread sst(server);
}

TEST(ThriftServer, ModifyingIOThreadCountLive) {
  TestThriftServerFactory<TestInterface> factory;
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());
  auto iothreadpool = std::make_shared<folly::IOThreadPoolExecutor>(0);
  server->setIOThreadPool(iothreadpool);

  ScopedServerThread sst(server);
  // If there are no worker threads, generally the server event base
  // will stop loop()ing.  Create a timeout event to make sure
  // it continues to loop for the duration of the test.
  server->getServeEventBase()->runInEventBaseThread(
      [&]() { server->getServeEventBase()->tryRunAfterDelay([]() {}, 5000); });

  server->getServeEventBase()->runInEventBaseThreadAndWait(
      [=]() { iothreadpool->setNumThreads(0); });

  folly::EventBase base;

  std::shared_ptr<folly::AsyncSocket> socket(
      folly ::AsyncSocket::newSocket(&base, *sst.getAddress()));

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  std::string response;

  boost::polymorphic_downcast<HeaderClientChannel*>(client.getChannel())
      ->setTimeout(100);

  // This should fail as soon as it connects:
  // since AsyncServerSocket has no accept callbacks installed,
  // it should close the connection right away.
  ASSERT_ANY_THROW(client.sync_sendResponse(response, 64));

  server->getServeEventBase()->runInEventBaseThreadAndWait(
      [=]() { iothreadpool->setNumThreads(30); });

  std::shared_ptr<folly::AsyncSocket> socket2(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));

  // Can't reuse client since the channel has gone bad
  TestServiceAsyncClient client2(HeaderClientChannel::newChannel(socket2));

  client2.sync_sendResponse(response, 64);
}

TEST(ThriftServer, setIOThreadPool) {
  auto exe = std::make_shared<folly::IOThreadPoolExecutor>(1);
  TestThriftServerFactory<TestInterface> factory;
  factory.useSimpleThreadManager(false);
  auto server = std::static_pointer_cast<ThriftServer>(factory.create());

  // Set the exe, this used to trip various calls like
  // CHECK(ioThreadPool->numThreads() == 0).
  server->setIOThreadPool(exe);
  EXPECT_EQ(1, server->getNumIOWorkerThreads());
}

TEST(ThriftServer, IdleServerTimeout) {
  TestThriftServerFactory<TestInterface> factory;

  auto server = factory.create();
  auto thriftServer = dynamic_cast<ThriftServer*>(server.get());
  thriftServer->setIdleServerTimeout(std::chrono::milliseconds(50));

  ScopedServerThread scopedServer(server);
  scopedServer.join();
}

TEST(ThriftServer, ServerConfigTest) {
  ThriftServer server;

  wangle::ServerSocketConfig defaultConfig;
  // If nothing is set, expect defaults
  auto serverConfig = server.getServerSocketConfig();
  EXPECT_EQ(
      serverConfig.sslHandshakeTimeout, defaultConfig.sslHandshakeTimeout);

  // Idle timeout of 0 with no SSL handshake set, expect it to be 0.
  server.setIdleTimeout(std::chrono::milliseconds::zero());
  serverConfig = server.getServerSocketConfig();
  EXPECT_EQ(
      serverConfig.sslHandshakeTimeout, std::chrono::milliseconds::zero());

  // Expect the explicit to always win
  server.setSSLHandshakeTimeout(std::chrono::milliseconds(100));
  serverConfig = server.getServerSocketConfig();
  EXPECT_EQ(serverConfig.sslHandshakeTimeout, std::chrono::milliseconds(100));

  // Clear it and expect it to be zero again (due to idle timeout = 0)
  server.setSSLHandshakeTimeout(folly::none);
  serverConfig = server.getServerSocketConfig();
  EXPECT_EQ(
      serverConfig.sslHandshakeTimeout, std::chrono::milliseconds::zero());
}

TEST(ThriftServer, MultiPort) {
  class MultiPortThriftServer : public ThriftServer {
   public:
    using ServerBootstrap::getSockets;
  };

  auto server = std::make_shared<MultiPortThriftServer>();
  server->setInterface(std::make_shared<TestInterface>());
  server->setNumIOWorkerThreads(1);
  server->setNumCPUWorkerThreads(1);

  // Add two ports 0 to trigger listening on two random ports.
  folly::SocketAddress addr;
  addr.setFromLocalPort(static_cast<uint16_t>(0));
  server->setAddresses({addr, addr});

  ScopedServerThread t(server);

  auto sockets = server->getSockets();
  ASSERT_EQ(sockets.size(), 2);

  folly::SocketAddress addr1, addr2;
  sockets[0]->getAddress(&addr1);
  sockets[1]->getAddress(&addr2);

  EXPECT_NE(addr1.getPort(), addr2.getPort());

  // Test that we can talk via first port.
  folly::EventBase base;

  auto testFn = [&](folly::SocketAddress& address) {
    std::shared_ptr<TAsyncSocket> socket(
        TAsyncSocket::newSocket(&base, address));
    TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));
    std::string response;
    client.sync_sendResponse(response, 42);
    EXPECT_EQ(response, "test42");
  };

  testFn(addr1);
  testFn(addr2);
}

TEST(ThriftServer, ClientIdentityHook) {
  /* Tests that the server calls the client identity hook when creating a new
     connection context */

  std::atomic<bool> flag{false};
  auto hook = [&flag](
                  const folly::AsyncTransportWrapper* /* unused */,
                  const X509* /* unused */,
                  const folly::SocketAddress& /* unused */) {
    flag = true;
    return std::unique_ptr<void, void (*)(void*)>(nullptr, [](void*) {});
  };

  TestThriftServerFactory<TestInterface> factory;
  auto server = factory.create();
  server->setClientIdentityHook(hook);
  apache::thrift::util::ScopedServerThread st(server);

  folly::EventBase base;
  auto socket = folly::AsyncSocket::newSocket(&base, *st.getAddress());
  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));
  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_TRUE(flag);
}

namespace {
void setupServerSSL(ThriftServer& server) {
  auto sslConfig = std::make_shared<wangle::SSLContextConfig>();
  sslConfig->setCertificate(folly::kTestCert, folly::kTestKey, "");
  sslConfig->clientCAFile = folly::kTestCA;
  sslConfig->sessionContext = "ThriftServerTest";
  server.setSSLConfig(std::move(sslConfig));
}

std::shared_ptr<folly::SSLContext> makeClientSslContext() {
  auto ctx = std::make_shared<folly::SSLContext>();
  ctx->loadCertificate(folly::kTestCert);
  ctx->loadPrivateKey(folly::kTestKey);
  ctx->loadTrustedCertificates(folly::kTestCA);
  ctx->authenticate(
      true /* verify server cert */, false /* don't verify server name */);
  ctx->setVerificationOption(folly::SSLContext::SSLVerifyPeerEnum::VERIFY);
  return ctx;
}

void doBadRequestHeaderTest(bool duplex, bool secure) {
  auto server = std::static_pointer_cast<ThriftServer>(
      TestThriftServerFactory<TestInterface>().create());
  server->setDuplex(duplex);
  if (secure) {
    setupServerSSL(*server);
  }
  ScopedServerThread sst(std::move(server));

  folly::EventBase evb;
  folly::AsyncSocket::UniquePtr socket(
      secure ? new folly::AsyncSSLSocket(makeClientSslContext(), &evb)
             : new folly::AsyncSocket(&evb));
  socket->connect(nullptr /* connect callback */, *sst.getAddress());

  class RecordWriteSuccessCallback
      : public folly::AsyncTransportWrapper::WriteCallback {
   public:
    void writeSuccess() noexcept override {
      EXPECT_FALSE(success_);
      success_.emplace(true);
    }

    void writeErr(
        size_t /* bytesWritten */,
        const folly::AsyncSocketException& /* exception */) noexcept override {
      EXPECT_FALSE(success_);
      success_.emplace(false);
    }

    bool success() const {
      return success_ && *success_;
    }

   private:
    folly::Optional<bool> success_;
  };
  RecordWriteSuccessCallback recordSuccessWriteCallback;

  class CheckClosedReadCallback
      : public folly::AsyncTransportWrapper::ReadCallback {
   public:
    explicit CheckClosedReadCallback(folly::AsyncSocket& socket)
        : socket_(socket) {
      socket_.setReadCB(this);
    }

    ~CheckClosedReadCallback() override {
      // We expect that the server closed the connection
      EXPECT_TRUE(remoteClosed_);
      socket_.close();
    }

    void getReadBuffer(void** bufout, size_t* lenout) override {
      // For this test, we never do anything with the buffered data, but we
      // still need to implement the full ReadCallback interface.
      *bufout = buf_;
      *lenout = sizeof(buf_);
    }

    void readDataAvailable(size_t /* len */) noexcept override {}

    void readEOF() noexcept override {
      remoteClosed_ = true;
    }

    void readErr(const folly::AsyncSocketException& ex) noexcept override {
      ASSERT_EQ(ECONNRESET, ex.getErrno());
      remoteClosed_ = true;
    }

   private:
    folly::AsyncSocket& socket_;
    char buf_[1024];
    bool remoteClosed_{false};
  };

  EXPECT_TRUE(socket->good());
  {
    CheckClosedReadCallback checkClosedReadCallback_(*socket);
    constexpr folly::StringPiece kBadRequest("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    socket->write(
        &recordSuccessWriteCallback, kBadRequest.data(), kBadRequest.size());
    evb.loop();
  }

  EXPECT_TRUE(recordSuccessWriteCallback.success());
  EXPECT_FALSE(socket->good());
}
} // namespace

TEST(ThriftServer, BadRequestHeaderNoDuplexNoSsl) {
  doBadRequestHeaderTest(false /* duplex */, false /* secure */);
}

TEST(ThriftServer, BadRequestHeaderDuplexNoSsl) {
  doBadRequestHeaderTest(true /* duplex */, false /* secure */);
}

TEST(ThriftServer, BadRequestHeaderNoDuplexSsl) {
  doBadRequestHeaderTest(false /* duplex */, true /* secure */);
}

TEST(ThriftServer, BadRequestHeaderDuplexSsl) {
  doBadRequestHeaderTest(true /* duplex */, true /* secure */);
}

TEST(ThriftServer, SSLRequiredRejectsPlaintext) {
  auto server = std::static_pointer_cast<ThriftServer>(
      TestThriftServerFactory<TestInterface>().create());
  server->setSSLPolicy(SSLPolicy::REQUIRED);
  setupServerSSL(*server);
  ScopedServerThread sst(std::move(server));

  folly::EventBase base;
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *sst.getAddress()));
  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  std::string response;
  EXPECT_THROW(client.sync_sendResponse(response, 64);, TTransportException);
}

TEST(ThriftServer, SSLRequiredAllowsLocalPlaintext) {
  auto server = std::static_pointer_cast<ThriftServer>(
      TestThriftServerFactory<TestInterface>().create());
  server->setAllowPlaintextOnLoopback(true);
  server->setSSLPolicy(SSLPolicy::REQUIRED);
  setupServerSSL(*server);
  ScopedServerThread sst(std::move(server));

  folly::EventBase base;
  // ensure that the address is loopback
  auto port = sst.getAddress()->getPort();
  folly::SocketAddress loopback("::1", port);
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, loopback));
  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "test64");
  base.loop();
}

TEST(ThriftServer, SSLRequiredLoopbackUsesSSL) {
  auto server = std::static_pointer_cast<ThriftServer>(
      TestThriftServerFactory<TestInterface>().create());
  server->setAllowPlaintextOnLoopback(true);
  server->setSSLPolicy(SSLPolicy::REQUIRED);
  setupServerSSL(*server);
  ScopedServerThread sst(std::move(server));

  folly::EventBase base;
  // ensure that the address is loopback
  auto port = sst.getAddress()->getPort();
  folly::SocketAddress loopback("::1", port);

  auto ctx = makeClientSslContext();
  auto sslSock = TAsyncSSLSocket::newSocket(ctx, &base);
  sslSock->connect(nullptr /* connect callback */, loopback);

  TestServiceAsyncClient client(HeaderClientChannel::newChannel(sslSock));

  std::string response;
  client.sync_sendResponse(response, 64);
  EXPECT_EQ(response, "test64");
  base.loop();
}

TEST(ThriftServer, SSLPermittedAcceptsPlaintextAndSSL) {
  auto server = std::static_pointer_cast<ThriftServer>(
      TestThriftServerFactory<TestInterface>().create());
  server->setSSLPolicy(SSLPolicy::PERMITTED);
  setupServerSSL(*server);
  ScopedServerThread sst(std::move(server));

  folly::EventBase base;
  {
    SCOPED_TRACE("Plaintext");
    std::shared_ptr<folly::AsyncSocket> socket(
        folly::AsyncSocket::newSocket(&base, *sst.getAddress()));
    TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

    std::string response;
    client.sync_sendResponse(response, 64);
    EXPECT_EQ(response, "test64");
    base.loop();
  }

  {
    SCOPED_TRACE("SSL");
    auto ctx = makeClientSslContext();
    auto sslSock = TAsyncSSLSocket::newSocket(ctx, &base);
    sslSock->connect(nullptr /* connect callback */, *sst.getAddress());

    TestServiceAsyncClient client(HeaderClientChannel::newChannel(sslSock));

    std::string response;
    client.sync_sendResponse(response, 64);
    EXPECT_EQ(response, "test64");
    base.loop();
  }
}

TEST(ThriftServer, ClientOnlyTimeouts) {
  class SendResponseInterface : public TestServiceSvIf {
    void sendResponse(std::string& _return, int64_t shouldSleepMs) override {
      auto header = getConnectionContext()->getHeader();
      if (shouldSleepMs) {
        usleep(shouldSleepMs * 1000);
      }
      _return = fmt::format(
          "{}:{}",
          header->getClientTimeout().count(),
          header->getClientQueueTimeout().count());
    }
  };
  TestThriftServerFactory<SendResponseInterface> factory;
  ScopedServerThread st(factory.create());

  folly::EventBase base;
  std::shared_ptr<folly::AsyncSocket> socket(
      folly::AsyncSocket::newSocket(&base, *st.getAddress()));
  TestServiceAsyncClient client(HeaderClientChannel::newChannel(socket));

  for (bool clientOnly : {false, true}) {
    for (bool shouldTimeOut : {true, false}) {
      std::string response;
      RpcOptions rpcOpts;
      rpcOpts.setTimeout(std::chrono::milliseconds(20));
      rpcOpts.setQueueTimeout(std::chrono::milliseconds(20));
      rpcOpts.setClientOnlyTimeouts(clientOnly);
      try {
        client.sync_sendResponse(rpcOpts, response, shouldTimeOut ? 50 : 0);
        EXPECT_FALSE(shouldTimeOut);
        if (clientOnly) {
          EXPECT_EQ(response, "0:0");
        } else {
          EXPECT_EQ(response, "20:20");
        }
      } catch (...) {
        EXPECT_TRUE(shouldTimeOut);
      }
    }
  }
  base.loop();
}
