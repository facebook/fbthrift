/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <chrono>
#include <stdexcept>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/SocketAddress.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBaseManager.h>
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/python/client/OmniClient.h> // @manual=//thrift/lib/python/client:omni_client__cython-lib
#include <thrift/lib/python/client/test/gen-cpp2/TestService.h>
#include <thrift/lib/python/client/test/gen-cpp2/test_types.h>

using namespace apache::thrift;
using namespace thrift::python::client;
using namespace thrift::python::test;

const std::string kTestHeaderKey = "headerKey";
const std::string kTestHeaderValue = "headerValue";

/**
 * A simple Scaffold service that will be used to test the Thrift OmniClient.
 */
class TestServiceHandler : virtual public TestServiceSvIf {
 public:
  TestServiceHandler() {}
  virtual ~TestServiceHandler() override {}
  int add(int num1, int num2) override { return num1 + num2; }
  void oneway() override {}
  void readHeader(
      std::string& value, std::unique_ptr<std::string> key) override {
    value = getRequestContext()->getHeader()->getHeaders().at(*key);
  }
  ResponseAndSinkConsumer<SimpleResponse, EmptyChunk, SimpleResponse> dumbSink(
      std::unique_ptr<EmptyRequest> request) override {
    (void)request;
    SinkConsumer<EmptyChunk, SimpleResponse> consumer{
        [&](folly::coro::AsyncGenerator<EmptyChunk&&> gen)
            -> folly::coro::Task<SimpleResponse> {
          SimpleResponse response;
          response.value_ref() = "final";
          co_return response;
        },
        1};
    SimpleResponse response;
    response.value_ref() = "initial";
    return {std::move(response), std::move(consumer)};
  }
};

/**
 * Small event-handler to know when a server is ready.
 */
class ServerReadyEventHandler : public server::TServerEventHandler {
 public:
  void preServe(const folly::SocketAddress* address) override {
    port_ = address->getPort();
    baton_.post();
  }

  int32_t waitForPortAssignment() {
    baton_.wait();
    return port_;
  }

 private:
  folly::Baton<> baton_;
  int32_t port_;
};

std::unique_ptr<ThriftServer> createServer(
    std::shared_ptr<AsyncProcessorFactory> processorFactory, uint16_t& port) {
  auto server = std::make_unique<ThriftServer>();
  server->setPort(0);
  server->setInterface(std::move(processorFactory));
  server->setNumIOWorkerThreads(1);
  server->setNumCPUWorkerThreads(1);
  server->setQueueTimeout(std::chrono::milliseconds(0));
  server->setIdleTimeout(std::chrono::milliseconds(0));
  server->setTaskExpireTime(std::chrono::milliseconds(0));
  server->setStreamExpireTime(std::chrono::milliseconds(0));
  auto eventHandler = std::make_shared<ServerReadyEventHandler>();
  server->setServerEventHandler(eventHandler);
  server->setup();

  // Get the port that the server has bound to
  port = eventHandler->waitForPortAssignment();
  return server;
}

class OmniClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Startup the test server.
    server_ = createServer(std::make_shared<TestServiceHandler>(), serverPort_);
  }

  void TearDown() override {
    // Stop the server and wait for it to complete.
    if (server_) {
      server_->cleanUp();
      server_.reset();
    }
  }

  template <class S>
  void connectToServer(
      folly::Function<folly::coro::Task<void>(OmniClient&)> callMe) {
    constexpr protocol::PROTOCOL_TYPES prot =
        std::is_same_v<S, apache::thrift::BinarySerializer>
        ? protocol::T_BINARY_PROTOCOL
        : protocol::T_COMPACT_PROTOCOL;
    folly::coro::blockingWait([this, &callMe]() -> folly::coro::Task<void> {
      CHECK_GT(serverPort_, 0) << "Check if the server has started already";
      folly::Executor* executor = co_await folly::coro::co_current_executor;
      auto channel = PooledRequestChannel::newChannel(
          executor,
          ioThread_,
          [this](folly::EventBase& evb) {
            auto chan = apache::thrift::RocketClientChannel::newChannel(
                folly::AsyncSocket::UniquePtr(
                    new folly::AsyncSocket(&evb, "::1", serverPort_)));
            chan->setProtocolId(prot);
            chan->setTimeout(500 /* ms */);
            return chan;
          },
          prot);
      OmniClient client(std::move(channel));
      co_await callMe(client);
    }());
  }

  // Send a request and compare the results to the expected value.
  template <class S = CompactSerializer, class Request, class Result>
  void testSendHeaders(
      const std::string& service,
      const std::string& function,
      const Request& req,
      const std::unordered_map<std::string, std::string>& headers,
      const Result& expected,
      const RpcKind rpcKind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE) {
    connectToServer<S>([=](OmniClient& client) -> folly::coro::Task<void> {
      std::string args = S::template serialize<std::string>(req);
      testContains<S>(
          co_await client.semifuture_send(
              service, function, args, headers, rpcKind),
          expected);
    });
  }

  template <class S = CompactSerializer, class Request, class Result>
  void testSend(
      const std::string& service,
      const std::string& function,
      const Request& req,
      const Result& expected,
      const RpcKind rpcKind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE) {
    testSendHeaders<S>(service, function, req, {}, expected, rpcKind);
  }

  // Send a request and compare the results to the expected value.
  template <class S, class Request>
  void testOnewaySendHeaders(
      const std::string& service,
      const std::string& function,
      const Request& req,
      const std::unordered_map<std::string, std::string>& headers = {}) {
    connectToServer<S>([=](OmniClient& client) -> folly::coro::Task<void> {
      std::string args = S::template serialize<std::string>(req);
      client.oneway_send(service, function, args, headers);
      co_return;
    });
  }

  template <class S, typename T>
  void testContains(OmniClientResponseWithHeaders response, const T& expected) {
    std::string expectedStr = S::template serialize<std::string>(expected);
    std::string result = response.buf.value()->moveToFbString().toStdString();
    // Contains instead of equals because of the envelope around the response.
    EXPECT_THAT(result, testing::HasSubstr(expectedStr));
  }

 protected:
  std::unique_ptr<ThriftServer> server_;
  folly::EventBase* eb_ = folly::EventBaseManager::get()->getEventBase();
  uint16_t serverPort_{0};
  std::shared_ptr<folly::IOExecutor> ioThread_{
      std::make_shared<folly::ScopedEventBaseThread>()};
};

TEST_F(OmniClientTest, AddTest) {
  AddRequest request;
  request.num1_ref() = 1;
  request.num2_ref() = 41;

  testSend<CompactSerializer>("TestService", "add", request, 42);
  testSend<BinarySerializer>("TestService", "add", request, 42);
}

TEST_F(OmniClientTest, OnewayTest) {
  EmptyRequest request;
  testOnewaySendHeaders<CompactSerializer>("TestService", "oneway", request);
  testOnewaySendHeaders<BinarySerializer>("TestService", "oneway", request);
}

TEST_F(OmniClientTest, ReadHeaderTest) {
  ReadHeaderRequest request;
  request.key() = kTestHeaderKey;

  testSendHeaders(
      "TestService",
      "readHeader",
      request,
      {{kTestHeaderKey, kTestHeaderValue}},
      kTestHeaderValue);
}

TEST_F(OmniClientTest, SinkRequestTest) {
  EmptyRequest request;
  SimpleResponse response;
  response.value_ref() = "initial";
  testSend("TestService", "dumbSink", request, response, RpcKind::SINK);
}
