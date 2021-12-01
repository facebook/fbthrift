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

#include <stdexcept>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBaseManager.h>
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <thrift/lib/py3lite/client/OmniClient.h> // @manual=//thrift/lib/py3lite/client:omni_client__cython-lib
#include <thrift/lib/py3lite/client/test/gen-cpp2/TestService.h>
#include <thrift/lib/py3lite/client/test/gen-cpp2/test_types.h>

using namespace apache::thrift;
using namespace thrift::py3lite::client;
using namespace thrift::py3lite::test;

const std::string kTestHeaderKey = "headerKey";
const std::string kTestHeaderValue = "headerValue";

/**
 * A simple Scaffold service that will be used to test the Thrift OmniClient.
 */
class TestService : virtual public TestServiceSvIf {
 public:
  TestService() {}
  virtual ~TestService() override {}
  int add(int num1, int num2) override { return num1 + num2; }
  void oneway() override {}
  void readHeader(
      std::string& value, std::unique_ptr<std::string> key) override {
    value = getRequestContext()->getHeader()->getHeaders().at(*key);
  }
};

/**
 * Small event-handler to know when a server is ready.
 */
class ServerEventHandler : public server::TServerEventHandler {
 public:
  explicit ServerEventHandler(
      folly::Promise<const folly::SocketAddress*>& promise)
      : promise_(promise) {}

  void preServe(const folly::SocketAddress* address) override {
    promise_.setValue(address);
  }

 private:
  folly::Promise<const folly::SocketAddress*>& promise_;
};

class OmniClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Startup the test server.
    folly::SocketAddress addr;
    addr.setFromLocalPort((uint16_t)0);
    server_ = std::make_unique<ThriftServer>();
    server_->setServerEventHandler(
        std::make_shared<ServerEventHandler>(addressPromise_));
    server_->setAddress(addr);
    server_->setInterface(std::make_shared<TestService>());
    serverThread_ = std::thread([this]() { server_->run(); });

    // Wait for the server to be ready.
    auto port = addressPromise_.getFuture()
                    .get(std::chrono::milliseconds(5000))
                    ->getPort();

    // Create the RequestChannel to pass onto the clients.
    auto channel =
        HeaderClientChannel::newChannel(folly::AsyncSocket::newSocket(
            eb_, folly::SocketAddress("::1", port, true), 5 * 1000LL /* 5 sec */
            ));

    // Create clients.
    client_ = std::make_unique<OmniClient>(std::move(channel));
  }

  void TearDown() override {
    // Stop the server and wait for it to complete.
    server_->stop();
    serverThread_.join();
  }

  // Send a request and compare the results to the expected value.
  template <class S, class Request, class Result, class Client>
  void testSendHeaders(
      const std::unique_ptr<Client>& client,
      const std::string& service,
      const std::string& function,
      const Request& req,
      const std::unordered_map<std::string, std::string>& headers,
      const Result& expected) {
    std::string args = S::template serialize<std::string>(req);
    testContains<S>(
        client->semifuture_send(service, function, args, headers)
            .via(eb_)
            .waitVia(eb_)
            .get(),
        expected);
  }

  // Send a request and compare the results to the expected value.
  template <class Request, class Result, class Client>
  void testSendHeaders(
      const std::unique_ptr<Client>& client,
      const std::string& service,
      const std::string& function,
      const Request& req,
      const std::unordered_map<std::string, std::string>& headers,
      const Result& expected) {
    switch (client->getChannelProtocolId()) {
      case protocol::T_BINARY_PROTOCOL:
        testSendHeaders<BinarySerializer>(
            client, service, function, req, headers, expected);
        break;
      case protocol::T_COMPACT_PROTOCOL:
        testSendHeaders<CompactSerializer>(
            client, service, function, req, headers, expected);
        break;
      default:
        FAIL() << "Channel protocol not supported";
    }
  }

  template <class Request, class Result, class Client>
  void testSend(
      const std::unique_ptr<Client>& client,
      const std::string& service,
      const std::string& function,
      const Request& req,
      const Result& expected) {
    testSendHeaders(client, service, function, req, {}, expected);
  }

  // Send a request and compare the results to the expected value.
  template <class S, class Request, class Client>
  void testOnewaySendHeaders(
      const std::unique_ptr<Client>& client,
      const std::string& service,
      const std::string& function,
      const Request& req,
      const std::unordered_map<std::string, std::string>& headers) {
    std::string args = S::template serialize<std::string>(req);
    client->oneway_send(service, function, args, headers);
  }

  template <class Request, class Client>
  void testOnewaySend(
      const std::unique_ptr<Client>& client,
      const std::string& service,
      const std::string& function,
      const Request& req) {
    std::string args;
    switch (client->getChannelProtocolId()) {
      case protocol::T_BINARY_PROTOCOL:
        testOnewaySendHeaders<BinarySerializer>(
            client, service, function, req, {});
        break;
      case protocol::T_COMPACT_PROTOCOL:
        testOnewaySendHeaders<CompactSerializer>(
            client, service, function, req, {});
        break;
      default:
        FAIL() << "Channel protocol not supported";
    }
  }

  template <class S, typename T>
  void testContains(OmniClientResponseWithHeaders response, const T& expected) {
    std::string expectedStr = S::template serialize<std::string>(expected);
    std::string result = response.buf.value()->moveToFbString().toStdString();
    // Contains instead of equals because of the envelope around the response.
    EXPECT_THAT(result, testing::HasSubstr(expectedStr));
  }

 protected:
  std::thread serverThread_;
  std::unique_ptr<ThriftServer> server_;
  std::unique_ptr<OmniClient> client_;
  folly::Promise<const folly::SocketAddress*> addressPromise_;
  folly::EventBase* eb_ = folly::EventBaseManager::get()->getEventBase();
};

TEST_F(OmniClientTest, AddTest) {
  AddRequest request;
  request.num1_ref() = 1;
  request.num2_ref() = 41;

  testSend(client_, "TestService", "add", request, 42);
}

TEST_F(OmniClientTest, OnewayTest) {
  EmptyRequest request;
  testOnewaySend(client_, "TestService", "oneway", request);
}

TEST_F(OmniClientTest, ReadHeaderTest) {
  ReadHeaderRequest request;
  request.key() = kTestHeaderKey;

  testSendHeaders(
      client_,
      "TestService",
      "readHeader",
      request,
      {{kTestHeaderKey, kTestHeaderValue}},
      kTestHeaderValue);
}
