/*
 * Copyright 2017-present Facebook, Inc.
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
#include "thrift/lib/cpp2/transport/core/testutil/TransportCompatibilityTest.h"

#include <folly/Baton.h>
#include <folly/ScopeGuard.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>
#include "thrift/lib/cpp2/transport/core/testutil/gen-cpp2/TestService.h"

namespace apache {
namespace thrift {

using namespace testing;

DEFINE_string(host, "::1", "host to connect to");

using namespace apache::thrift;
using namespace testutil::testservice;

TransportCompatibilityTest::TransportCompatibilityTest()
    : workerThread_("RSRequestResponseTest_WorkerThread") {
  setupServer();
}

// Tears down after the test.
TransportCompatibilityTest::~TransportCompatibilityTest() {
  stopServer();
}

// Event handler to attach to the Thrift server so we know when it is
// ready to serve and also so we can determine the port it is
// listening on.
class TransportCompatibilityTestEventHandler
    : public server::TServerEventHandler {
 public:
  // This is a callback that is called when the Thrift server has
  // initialized and is ready to serve RPCs.
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

void TransportCompatibilityTest::addRoutingHandler(
    std::unique_ptr<TransportRoutingHandler> routingHandler) {
  DCHECK(server_) << "First call setupServer() function";

  server_->addRoutingHandler(std::move(routingHandler));
}

ThriftServer* TransportCompatibilityTest::getServer() {
  DCHECK(server_) << "First call setupServer() function";

  return server_.get();
}

void TransportCompatibilityTest::setupServer() {
  DCHECK(!server_) << "First close the server with stopServer()";

  handler_ = std::make_shared<StrictMock<TestServiceMock>>();
  auto cpp2PFac =
      std::make_shared<ThriftServerAsyncProcessorFactory<TestServiceMock>>(
          handler_);

  server_ = std::make_unique<ThriftServer>();
  server_->setPort(0);
  server_->setProcessorFactory(cpp2PFac);
}

void TransportCompatibilityTest::startServer() {
  DCHECK(server_) << "First call setupServer() function";

  auto eventHandler =
      std::make_shared<TransportCompatibilityTestEventHandler>();
  server_->setServerEventHandler(eventHandler);
  server_->setup();

  // Get the port that the server has bound to
  port_ = eventHandler->waitForPortAssignment();
}

void TransportCompatibilityTest::stopServer() {
  if (server_) {
    server_->cleanUp();
    server_.reset();
    handler_.reset();
  }
}

void TransportCompatibilityTest::connectToServer(
    folly::Function<void(std::unique_ptr<TestServiceAsyncClient>)> callMe) {
  auto mgr = ConnectionManager::getInstance();
  CHECK_GT(port_, 0) << "Check if the server has started already";
  auto connection = mgr->getConnection(FLAGS_host, port_);
  auto channel = ThriftClient::Ptr(
      new ThriftClient(connection, workerThread_.getEventBase()));
  channel->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);

  auto client = std::make_unique<TestServiceAsyncClient>(std::move(channel));

  callMe(std::move(client));
}

class TimeoutTestCallback : public RequestCallback {
 public:
  TimeoutTestCallback(bool shouldTimeout) : shouldTimeout_(shouldTimeout) {}
  virtual ~TimeoutTestCallback() {
    EXPECT_TRUE(callbackReceived_);
  }
  void requestSent() override {
    EXPECT_FALSE(requestSentCalled_);
    requestSentCalled_ = true;
  }
  void replyReceived(ClientReceiveState&& /*crs*/) override {
    // EXPECT_TRUE(requestSentCalled_); TODO: uncomment after fixing code.
    EXPECT_FALSE(callbackReceived_);
    EXPECT_FALSE(shouldTimeout_);
    callbackReceived_ = true;
  }
  void requestError(ClientReceiveState&& /*crs*/) override {
    // EXPECT_TRUE(requestSentCalled_); TODO: uncomment after fixing code.
    EXPECT_FALSE(callbackReceived_);
    EXPECT_TRUE(shouldTimeout_);
    callbackReceived_ = true;
  }
  bool callbackReceived() {
    return callbackReceived_;
  }

 private:
  bool shouldTimeout_;
  bool requestSentCalled_{false};
  bool callbackReceived_{false};
};

void TransportCompatibilityTest::callSleep(
    TestServiceAsyncClient* client,
    int32_t timeoutMs,
    int32_t sleepMs) {
  auto cb = std::make_unique<TimeoutTestCallback>(timeoutMs < sleepMs);
  RpcOptions opts;
  opts.setTimeout(std::chrono::milliseconds(timeoutMs));
  client->sleep(opts, std::move(cb), sleepMs);
}

void TransportCompatibilityTest::TestRequestResponse_Simple() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    EXPECT_CALL(*handler_.get(), sumTwoNumbers_(1, 2)).Times(2);
    EXPECT_CALL(*handler_.get(), add_(1));
    EXPECT_CALL(*handler_.get(), add_(2));
    EXPECT_CALL(*handler_.get(), add_(5));

    // Send a message
    CHECK_EQ(client->sync_sumTwoNumbers(1, 2), 3);
    CHECK_EQ(client->sync_add(1), 1);

    auto future = client->future_add(2);
    CHECK_EQ(future.get(), 3);

    CHECK_EQ(client->sync_sumTwoNumbers(1, 2), 3);
    CHECK_EQ(client->sync_add(5), 8);
  });
}

void TransportCompatibilityTest::TestRequestResponse_MultipleClients() {
  const int clientCount = 10;
  EXPECT_CALL(*handler_.get(), sumTwoNumbers_(1, 2)).Times(2 * clientCount);
  EXPECT_CALL(*handler_.get(), add_(1)).Times(clientCount);
  EXPECT_CALL(*handler_.get(), add_(2)).Times(clientCount);
  EXPECT_CALL(*handler_.get(), add_(5)).Times(clientCount);

  auto lambda = [](std::unique_ptr<TestServiceAsyncClient> client) {
    // Send a message
    CHECK_EQ(client->sync_sumTwoNumbers(1, 2), 3);
    CHECK_GE(client->sync_add(1), 1);

    auto future = client->future_add(2);
    CHECK_GE(future.get(), 3);

    CHECK_EQ(client->sync_sumTwoNumbers(1, 2), 3);
    CHECK_GE(client->sync_add(5), 8);
  };

  std::vector<folly::ScopedEventBaseThread> threads(clientCount);
  std::vector<folly::Promise<folly::Unit>> promises(clientCount);
  std::vector<folly::Future<folly::Unit>> futures;
  for (int i = 0; i < clientCount; ++i) {
    auto& promise = promises[i];
    futures.emplace_back(promise.getFuture());
    threads[i].getEventBase()->runInEventBaseThread([&promise, lambda, this]() {
      connectToServer(lambda);
      promise.setValue();
    });
  }
  folly::collectAll(futures);
  threads.clear();
}

void TransportCompatibilityTest::TestRequestResponse_ExpectedException() {
  EXPECT_THROW(
      connectToServer(
          [&](auto client) { client->sync_throwExpectedException(1); }),
      TestServiceException);

  EXPECT_THROW(
      connectToServer(
          [&](auto client) { client->future_throwExpectedException(1).get(); }),
      TestServiceException);
}

void TransportCompatibilityTest::TestRequestResponse_UnexpectedException() {
  EXPECT_THROW(
      connectToServer(
          [&](auto client) { client->sync_throwUnexpectedException(2); }),
      apache::thrift::TApplicationException);

  EXPECT_THROW(
      connectToServer([&](auto client) {
        client->future_throwUnexpectedException(2).get();
      }),
      apache::thrift::TApplicationException);
}

void TransportCompatibilityTest::TestRequestResponse_Timeout() {
  // Note: This test requires sufficient number of CPU threads on the
  // server so that the sleep calls are not backed up.
  // Warning: This test may be flaky due to use of timeouts.
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    // These are all async calls.  The first batch of calls get
    // dispatched immediately, then there is a sleep, and then the
    // second batch of calls get dispatched.  All calls have separate
    // timeouts and different delays on the server side.
    callSleep(client.get(), 1, 100);
    callSleep(client.get(), 100, 0);
    callSleep(client.get(), 1, 100);
    callSleep(client.get(), 100, 0);
    callSleep(client.get(), 2000, 500);
    /* sleep override */
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    callSleep(client.get(), 1, 100);
    callSleep(client.get(), 100, 0);
    /* Sleep to give time for all callback to be completed */
    /* sleep override */
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  });
}

void TransportCompatibilityTest::TestRequestResponse_Header() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    { // Sync
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      client->sync_headers(rpcOptions);
      auto keyValue = rpcOptions.getReadHeaders();
      CHECK(keyValue.find("header_from_server") != keyValue.end());
      CHECK_STREQ(keyValue.find("header_from_server")->second.c_str(), "1");
    }

    { // Future
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      auto future = client->header_future_headers(rpcOptions);
      auto tHeader = future.get().second;
      auto keyValue = tHeader->getHeaders();
      CHECK(keyValue.find("header_from_server") != keyValue.end());
      CHECK_STREQ(keyValue.find("header_from_server")->second.c_str(), "1");
    }

    { // Callback
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      folly::Promise<folly::Unit> executed;
      auto future = executed.getFuture();
      client->headers(
          rpcOptions,
          std::unique_ptr<RequestCallback>(
              new FunctionReplyCallback([&](ClientReceiveState&& state) {
                auto keyValue = state.header()->getHeaders();
                CHECK(keyValue.find("header_from_server") != keyValue.end());
                CHECK_STREQ(
                    keyValue.find("header_from_server")->second.c_str(), "1");

                auto exw = TestServiceAsyncClient::recv_wrapped_headers(state);
                EXPECT_FALSE(exw);
                executed.setValue();
              })));
      auto& waited = future.wait(folly::Duration(100));
      CHECK(waited.isReady());
    }
  });
}

void TransportCompatibilityTest::
    TestRequestResponse_Header_ExpectedException() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    { // Sync
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("expected_exception", "1");
      bool thrown = false;
      try {
        client->sync_headers(rpcOptions);
      } catch (const std::exception& ex) {
        thrown = true;
      }
      EXPECT_TRUE(thrown);
      auto keyValue = rpcOptions.getReadHeaders();
      CHECK(keyValue.find("header_from_server") != keyValue.end());
    }

    { // Future
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("expected_exception", "1");
      auto future = client->header_future_headers(rpcOptions);
      auto& waited = future.wait();
      auto& ftry = waited.getTry();
      EXPECT_TRUE(ftry.hasException());
      EXPECT_THAT(
          ftry.tryGetExceptionObject()->what(),
          HasSubstr("TestServiceException"));
    }

    { // Callback
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("expected_exception", "1");
      folly::Promise<folly::Unit> executed;
      auto future = executed.getFuture();
      client->headers(
          rpcOptions,
          std::unique_ptr<RequestCallback>(
              new FunctionReplyCallback([&](ClientReceiveState&& state) {
                auto exw = TestServiceAsyncClient::recv_wrapped_headers(state);
                CHECK_NOTNULL(exw.get_exception());
                EXPECT_THAT(
                    exw.what().c_str(), HasSubstr("TestServiceException"));
                executed.setValue();
              })));
      auto& waited = future.wait(folly::Duration(100));
      CHECK(waited.isReady());
    }
  });
}

void TransportCompatibilityTest::
    TestRequestResponse_Header_UnexpectedException() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    { // Sync
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("unexpected_exception", "1");
      EXPECT_THROW(
          client->sync_headers(rpcOptions),
          apache::thrift::TApplicationException);
      auto keyValue = rpcOptions.getReadHeaders();
      CHECK(keyValue.find("header_from_server") != keyValue.end());
    }

    { // Future
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("unexpected_exception", "1");
      auto future = client->header_future_headers(rpcOptions);
      EXPECT_THROW(future.get(), apache::thrift::TApplicationException);
    }

    { // Callback
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("unexpected_exception", "1");
      folly::Promise<folly::Unit> executed;
      auto future = executed.getFuture();
      client->headers(
          rpcOptions,
          std::unique_ptr<RequestCallback>(
              new FunctionReplyCallback([&](ClientReceiveState&& state) {
                auto exw = TestServiceAsyncClient::recv_wrapped_headers(state);
                CHECK_NOTNULL(exw.get_exception());
                EXPECT_THAT(
                    exw.what().c_str(), HasSubstr("TApplicationException"));
                executed.setValue();
              })));
      auto& waited = future.wait(folly::Duration(100));
      CHECK(waited.isReady());
    }
  });
}

} // namespace thrift
} // namespace apache
