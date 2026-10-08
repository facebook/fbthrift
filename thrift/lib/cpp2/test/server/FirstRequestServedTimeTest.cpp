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
#include <cstdint>
#include <memory>
#include <optional>

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/DummyMonitor.h>
#include <thrift/lib/cpp2/test/gen-cpp2/DummyStatus.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/test/util/TestHandler.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace apache::thrift;
using namespace apache::thrift::test;

TEST(FirstRequestServedTimeTest, RecordedOnceOnFirstUserRequest) {
  ScopedServerInterfaceThread runner(std::make_shared<TestHandler>());
  auto& server = runner.getThriftServer();
  EXPECT_FALSE(server.getFirstRequestServedTime().has_value());

  const auto before = std::chrono::system_clock::now();
  runner.newClient<Client<TestService>>()->sync_voidResponse();
  const auto firstServed = server.getFirstRequestServedTime();
  ASSERT_TRUE(firstServed.has_value());
  EXPECT_TRUE(*firstServed >= before);
  EXPECT_TRUE(*firstServed <= std::chrono::system_clock::now());

  runner.newClient<Client<TestService>>()->sync_voidResponse();
  EXPECT_TRUE(firstServed == server.getFirstRequestServedTime());
}

TEST(FirstRequestServedTimeTest, ExcludesStatusCalls) {
  class StatusHandler : public ServiceHandler<DummyStatus>,
                        public StatusServerInterface {
    void async_eb_getStatus(
        HandlerCallbackPtr<std::int64_t> callback) override {
      callback->result(0);
    }
  };

  ScopedServerInterfaceThread runner(
      std::make_shared<TestHandler>(), [](ThriftServer& server) {
        server.setStatusInterface(std::make_shared<StatusHandler>());
      });
  runner.newClient<Client<DummyStatus>>()->sync_getStatus();
  EXPECT_FALSE(
      runner.getThriftServer().getFirstRequestServedTime().has_value());
}

// Served over the user interface, so only the method name marks it as
// monitoring.
TEST(FirstRequestServedTimeTest, ExcludesMonitoringMethodsOnUserInterface) {
  class MonitorHandler : public ServiceHandler<DummyMonitor> {
    std::int64_t getCounter() override { return 0; }
  };

  ScopedServerInterfaceThread runner(std::make_shared<MonitorHandler>());
  runner.newClient<Client<DummyMonitor>>()->sync_getCounter();
  EXPECT_FALSE(
      runner.getThriftServer().getFirstRequestServedTime().has_value());
}
