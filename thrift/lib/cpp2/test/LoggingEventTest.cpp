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
#include <thrift/lib/cpp2/server/ThriftServer.h>

namespace {

using apache::thrift::ApplicationEventHandler;
using apache::thrift::ConnectionEventHandler;
using apache::thrift::LoggingEventRegistry;
using apache::thrift::ServerEventHandler;
using apache::thrift::ThriftServer;

constexpr std::string_view kServe = "serve";

class TestServerEventHandler : public ServerEventHandler {
 public:
  MOCK_METHOD1(log, void(const ThriftServer&));
};

class TestEventRegistry : public LoggingEventRegistry {
 public:
  TestEventRegistry() {
    serverEventMap_[kServe] = makeHandler<TestServerEventHandler>();
  }

  ServerEventHandler& getServerEventHandler(
      std::string_view key) const override {
    return *serverEventMap_.at(key).get();
  }

  ConnectionEventHandler& getConnectionEventHandler(
      std::string_view /* key */) const override {
    static auto* handler = new ConnectionEventHandler();
    return *handler;
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
};

void expectServerEventCall(std::string_view key, size_t times) {
  auto& handler =
      apache::thrift::getLoggingEventRegistry().getServerEventHandler(key);
  auto* testHandler = dynamic_cast<TestServerEventHandler*>(&handler);
  EXPECT_NE(testHandler, nullptr);
  EXPECT_CALL(*testHandler, log(testing::_)).Times(times);
}

} // namespace

namespace fbthrift {
std::unique_ptr<apache::thrift::LoggingEventRegistry>
makeLoggingEventRegistry() {
  return std::make_unique<TestEventRegistry>();
}
} // namespace fbthrift

class ServerEventLogTest : public testing::Test {};

TEST_F(ServerEventLogTest, serverTest) {
  expectServerEventCall(kServe, 1);
  apache::thrift::ThriftServer server;
}
