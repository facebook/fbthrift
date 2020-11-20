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

#include <thrift/lib/cpp2/server/LoggingEvent.h>

#include <memory>
#include <unordered_map>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/PluggableFunction.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

namespace {

using apache::thrift::ApplicationEventHandler;
using apache::thrift::ConnectionEventHandler;
using apache::thrift::LoggingEventRegistry;
using apache::thrift::ServerEventHandler;
using apache::thrift::ThriftServer;

constexpr std::string_view kServe = "serve";
constexpr std::string_view kNonTls = "non_tls";

class TestServerEventHandler : public ServerEventHandler {
 public:
  MOCK_METHOD1(log, void(const ThriftServer&));
};

class TestConnectionEventHandler : public ConnectionEventHandler {
 public:
  MOCK_METHOD2(
      log,
      void(const apache::thrift::Cpp2Worker&, const folly::AsyncTransport&));
};

class TestEventRegistry : public LoggingEventRegistry {
 public:
  TestEventRegistry() {
    serverEventMap_[kServe] = makeHandler<TestServerEventHandler>();
    connectionEventMap_[kNonTls] = makeHandler<TestConnectionEventHandler>();
  }

  ServerEventHandler& getServerEventHandler(
      std::string_view key) const override {
    return *serverEventMap_.at(key).get();
  }

  ConnectionEventHandler& getConnectionEventHandler(
      std::string_view key) const override {
    return *connectionEventMap_.at(key).get();
  }

  ApplicationEventHandler& getApplicationEventHandler(
      std::string_view /* key */) const override {
    static auto* handler = new ApplicationEventHandler();
    return *handler;
  }

 private:
  template <typename T>
  std::unique_ptr<T> makeHandler() {
    auto obj = std::make_unique<T>();
    testing::Mock::AllowLeak(obj.get());
    return obj;
  }

  std::unordered_map<std::string_view, std::unique_ptr<ServerEventHandler>>
      serverEventMap_;
  std::unordered_map<std::string_view, std::unique_ptr<ConnectionEventHandler>>
      connectionEventMap_;
};

THRIFT_PLUGGABLE_FUNC_SET(
    std::unique_ptr<apache::thrift::LoggingEventRegistry>,
    makeLoggingEventRegistry) {
  return std::make_unique<TestEventRegistry>();
}
} // namespace

template <typename T>
class LoggingEventTest : public testing::Test {
 protected:
  void SetUp() override {
    apache::thrift::useMockLoggingEventRegistry();
  }

  template <typename H>
  T& fetchHandler(
      H& (LoggingEventRegistry::*method)(std::string_view) const,
      std::string_view key) {
    if (!handler_) {
      auto& handler = (apache::thrift::getLoggingEventRegistry().*method)(key);
      handler_ = dynamic_cast<T*>(&handler);
      EXPECT_NE(handler_, nullptr);
    }
    return *handler_;
  }

  void TearDown() override {
    if (handler_) {
      ASSERT_TRUE(testing::Mock::VerifyAndClearExpectations(handler_));
    }
  }

 private:
  T* handler_{nullptr};
};

class ServerEventLogTest : public LoggingEventTest<TestServerEventHandler> {
 protected:
  void expectServerEventCall(std::string_view key, size_t times) {
    auto& handler =
        fetchHandler(&LoggingEventRegistry::getServerEventHandler, key);
    EXPECT_CALL(handler, log(testing::_)).Times(times);
  }
};

class ConnectionEventLogTest
    : public LoggingEventTest<TestConnectionEventHandler> {
 protected:
  void expectConnectionEventCall(std::string_view key, size_t times) {
    auto& handler =
        fetchHandler(&LoggingEventRegistry::getConnectionEventHandler, key);
    EXPECT_CALL(handler, log(testing::_, testing::_)).Times(times);
  }
};

class TestServiceHandler : public apache::thrift::test::TestServiceSvIf {
 public:
  void voidResponse() override {}
};

TEST_F(ServerEventLogTest, serverTest) {
  expectServerEventCall(kServe, 1);
  auto handler = std::make_shared<TestServiceHandler>();
  apache::thrift::ScopedServerInterfaceThread server(handler);
}

TEST_F(ConnectionEventLogTest, connectionTest) {
  expectConnectionEventCall(kNonTls, 1);
  auto handler = std::make_shared<TestServiceHandler>();
  apache::thrift::ScopedServerInterfaceThread server(handler);
  auto client =
      server.newClient<apache::thrift::test::TestServiceAsyncClient>();

  // block to make sure request is actually sent.
  client->semifuture_voidResponse().get();
}
