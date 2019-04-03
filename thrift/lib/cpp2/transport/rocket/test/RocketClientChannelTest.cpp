/*
 * Copyright 2019-present Facebook, Inc.
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

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <folly/Try.h>
#include <folly/fibers/Baton.h>
#include <folly/fibers/Fiber.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/futures/Future.h>
#include <folly/futures/helpers.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace apache::thrift;

namespace {
class Handler : public test::TestServiceSvIf {
 public:
  folly::SemiFuture<std::unique_ptr<std::string>> semifuture_sendResponse(
      int64_t size) final {
    return folly::makeSemiFuture(
        std::make_unique<std::string>(std::to_string(size)));
  }

  folly::SemiFuture<folly::Unit> semifuture_noResponse(int64_t) final {
    return folly::makeSemiFuture();
  }
};

class RocketClientChannelTest : public testing::Test {
 public:
  void SetUp() override {
    dynamic_cast<ThriftServer&>(runner_.getThriftServer())
        .addRoutingHandler(std::make_unique<RSRoutingHandler>());
  }

  test::TestServiceAsyncClient makeClient(folly::EventBase& evb) {
    return test::TestServiceAsyncClient(
        RocketClientChannel::newChannel(async::TAsyncSocket::UniquePtr(
            new async::TAsyncSocket(&evb, runner_.getAddress()))));
  }

 private:
  ScopedServerInterfaceThread runner_{std::make_shared<Handler>()};
};
} // namespace

TEST_F(RocketClientChannelTest, SyncThread) {
  folly::EventBase evb;
  auto client = makeClient(evb);

  std::string response;
  client.sync_sendResponse(response, 123);
  EXPECT_EQ("123", response);
}

TEST_F(RocketClientChannelTest, SyncFiber) {
  folly::EventBase evb;
  auto& fm = folly::fibers::getFiberManager(evb);
  auto client = makeClient(evb);

  size_t responses = 0;
  fm.addTaskFinally(
      [&client] {
        std::string response;
        client.sync_sendResponse(response, 123);
        return response;
      },
      [&responses](folly::Try<std::string>&& tryResponse) {
        EXPECT_TRUE(tryResponse.hasValue());
        EXPECT_EQ("123", *tryResponse);
        ++responses;
      });
  while (fm.hasTasks()) {
    evb.loopOnce();
  }
  EXPECT_EQ(1, responses);
}

TEST_F(RocketClientChannelTest, SyncThreadOneWay) {
  folly::EventBase evb;
  auto client = makeClient(evb);
  client.sync_noResponse(123);
}

TEST_F(RocketClientChannelTest, SyncFiberOneWay) {
  folly::EventBase evb;
  auto& fm = folly::fibers::getFiberManager(evb);
  auto client = makeClient(evb);

  size_t sent = 0;
  fm.addTaskFinally(
      [&client] { client.sync_noResponse(123); },
      [&sent](folly::Try<void>&& tryResponse) {
        EXPECT_TRUE(tryResponse.hasValue());
        ++sent;
      });
  while (fm.hasTasks()) {
    evb.loopOnce();
  }
  EXPECT_EQ(1, sent);
}
