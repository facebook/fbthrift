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

#include <folly/io/async/EventBase.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/FutureRequest.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/ServerInstrumentation.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/InstrumentationTestService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <atomic>
#include <thread>

using apache::thrift::ServerInstrumentation;
using apache::thrift::ThriftServer;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::test::InstrumentationTestServiceAsyncClient;
using apache::thrift::test::InstrumentationTestServiceSvIf;

class TestInterface : public InstrumentationTestServiceSvIf {
 public:
  auto requestGuard() {
    reqCount_++;
    return folly::makeGuard([&] { --reqCount_; });
  }

  folly::coro::Task<void> co_sendRequest() override {
    auto rg = requestGuard();
    co_await finished_;
  }

  folly::coro::Task<apache::thrift::Stream<int32_t>> co_sendStreamingRequest()
      override {
    auto rg = requestGuard();
    co_await finished_;
    co_return createStreamGenerator(
        []() -> folly::coro::AsyncGenerator<int32_t> { co_yield 0; });
  }

  void stopRequests() {
    finished_.post();
    while (reqCount_ > 0) {
      std::this_thread::yield();
    }
  }

  void waitForRequests(int32_t num) {
    while (reqCount_ != num) {
      std::this_thread::yield();
    }
  }

 private:
  folly::coro::Baton finished_;
  std::atomic<int32_t> reqCount_{0};
};

class RequestInstrumentationTest : public testing::Test {
 protected:
  RequestInstrumentationTest()
      : handler_(std::make_shared<TestInterface>()),
        server_(handler_, "::1", 0),
        thriftServer_(dynamic_cast<ThriftServer*>(&server_.getThriftServer())) {
    EXPECT_NE(thriftServer_, nullptr);
  }

  std::vector<ThriftServer::RequestSnapshot> getRequestSnapshots(
      size_t reqNum) {
    SCOPE_EXIT {
      handler_->stopRequests();
    };
    handler_->waitForRequests(reqNum);

    auto serverReqSnapshots = thriftServer_->snapshotActiveRequests().get();

    EXPECT_EQ(serverReqSnapshots.size(), reqNum);
    return serverReqSnapshots;
  }

  std::shared_ptr<TestInterface> handler_;
  apache::thrift::ScopedServerInterfaceThread server_;
  ThriftServer* thriftServer_;
};

TEST_F(RequestInstrumentationTest, simpleRocketRequestTest) {
  size_t reqNum = 5;
  auto client = server_.newClient<InstrumentationTestServiceAsyncClient>(
      nullptr, [](auto socket) mutable {
        return apache::thrift::RocketClientChannel::newChannel(
            std::move(socket));
      });

  std::vector<folly::SemiFuture<apache::thrift::ClientBufferedStream<int32_t>>>
      streams;
  for (size_t i = 0; i < reqNum; i++) {
    streams.emplace_back(client->semifuture_sendStreamingRequest());
    client->semifuture_sendRequest();
  }
  // Consume streams, otherwise server thread manager can get stuck on
  // remaining stream generators when exiting.
  auto g = folly::makeGuard([streams = std::move(streams)]() mutable {
    for (auto& s : streams) {
      std::move(s).get().subscribeInline([](folly::Try<int32_t>) {});
    }
  });
  for (auto& reqSnapshot : getRequestSnapshots(2 * reqNum)) {
    auto methodName = reqSnapshot.getMethodName();
    EXPECT_TRUE(
        methodName == "sendRequest" || methodName == "sendStreamingRequest");
  }
}

TEST_F(RequestInstrumentationTest, simpleHeaderRequestTest) {
  size_t reqNum = 5;
  auto client = server_.newClient<InstrumentationTestServiceAsyncClient>(
      nullptr, [](auto socket) mutable {
        return apache::thrift::HeaderClientChannel::newChannel(
            std::move(socket));
      });

  for (size_t i = 0; i < reqNum; i++) {
    client->semifuture_sendRequest();
  }

  for (auto& reqSnapshot : getRequestSnapshots(reqNum)) {
    EXPECT_EQ(reqSnapshot.getMethodName(), "sendRequest");
  }
}

class ServerInstrumentationTest : public testing::Test {};

TEST_F(ServerInstrumentationTest, simpleServerTest) {
  ThriftServer server0;
  {
    auto handler = std::make_shared<TestInterface>();
    apache::thrift::ScopedServerInterfaceThread server1(handler);
    EXPECT_EQ(ServerInstrumentation::getServerCount(), 2);
  }
  EXPECT_EQ(ServerInstrumentation::getServerCount(), 1);
}
