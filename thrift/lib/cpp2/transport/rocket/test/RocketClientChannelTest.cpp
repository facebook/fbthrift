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
#include <string>

#include <gtest/gtest.h>

#include <folly/Conv.h>
#include <folly/Try.h>
#include <folly/fibers/Baton.h>
#include <folly/fibers/Fiber.h>
#include <folly/fibers/FiberManager.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>

#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace apache::thrift;

namespace {
class Handler : public test::TestServiceSvIf {
 public:
  folly::SemiFuture<std::unique_ptr<std::string>> semifuture_sendResponse(
      int64_t size) final {
    lastTimeoutMsec_ =
        getConnectionContext()->getHeader()->getClientTimeout().count();
    return folly::makeSemiFuture()
        .delayed(std::chrono::milliseconds(sleepDelayMsec_))
        .defer([size](auto&&) {
          return std::make_unique<std::string>(folly::to<std::string>(size));
        });
  }

  folly::SemiFuture<folly::Unit> semifuture_noResponse(int64_t) final {
    lastTimeoutMsec_ =
        getConnectionContext()->getHeader()->getClientTimeout().count();
    return folly::makeSemiFuture();
  }

  int32_t getLastTimeoutMsec() const {
    return lastTimeoutMsec_;
  }
  void setSleepDelayMs(int32_t delay) {
    sleepDelayMsec_ = delay;
  }

 private:
  int32_t lastTimeoutMsec_{-1};
  int32_t sleepDelayMsec_{0};
};

class RocketClientChannelTest : public testing::Test {
 public:
  test::TestServiceAsyncClient makeClient(folly::EventBase& evb) {
    return test::TestServiceAsyncClient(
        RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
            new folly::AsyncSocket(&evb, runner_.getAddress()))));
  }

 protected:
  std::shared_ptr<Handler> handler_{std::make_shared<Handler>()};
  ScopedServerInterfaceThread runner_{handler_};
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

TEST_F(RocketClientChannelTest, SyncThreadCheckTimeoutPropagated) {
  folly::EventBase evb;
  auto client = makeClient(evb);

  RpcOptions opts;
  std::string response;
  // Ensure that normally, the timeout value gets propagated.
  opts.setTimeout(std::chrono::milliseconds(20));
  client.sync_sendResponse(opts, response, 123);
  EXPECT_EQ("123", response);
  EXPECT_EQ(20, handler_->getLastTimeoutMsec());
  // And when we set client-only, it's not propagated.
  opts.setClientOnlyTimeouts(true);
  client.sync_sendResponse(opts, response, 456);
  EXPECT_EQ("456", response);
  EXPECT_EQ(0, handler_->getLastTimeoutMsec());

  // Double-check that client enforces the timeouts in both cases.
  handler_->setSleepDelayMs(50);
  ASSERT_ANY_THROW(client.sync_sendResponse(opts, response, 456));
  opts.setClientOnlyTimeouts(false);
  ASSERT_ANY_THROW(client.sync_sendResponse(opts, response, 456));
}

TEST_F(RocketClientChannelTest, ThriftClientLifetime) {
  folly::EventBase evb;
  folly::Optional<test::TestServiceAsyncClient> client = makeClient(evb);

  auto& fm = folly::fibers::getFiberManager(evb);
  auto future = fm.addTaskFuture([&] {
    std::string response;
    client->sync_sendResponse(response, 123);
    EXPECT_EQ("123", response);
  });

  // Trigger request sending.
  evb.loopOnce();

  // Reset the client.
  client.reset();

  // Wait for the response.
  future.getVia(&evb);
}
