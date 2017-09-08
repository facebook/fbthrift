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
#include "thrift/lib/cpp2/transport/core/testutil/RoutingHandlerTest.h"

#include <folly/Baton.h>
#include <folly/ScopeGuard.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.h>
#include <thrift/lib/cpp2/transport/http2/server/ThriftRequestHandlerFactory.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>
#include "thrift/lib/cpp2/transport/core/testutil/gen-cpp2/TestService.h"

// ConnectionManager depends on this flag.
DECLARE_string(transport);

namespace apache {
namespace thrift {

using namespace testing;

DEFINE_string(host, "::1", "host to connect to");

using namespace apache::thrift;
using namespace rsocket;
using namespace testutil::testservice;
using proxygen::RequestHandlerChain;

RoutingHandlerTest::RoutingHandlerTest(RoutingType routingType)
    : workerThread("RSRequestResponseTest_WorkerThread") {
  if (routingType == RoutingType::RSOCKET) { // override the default
    FLAGS_transport = "rsocket"; // client's transport
  }

  startServer(routingType);
}

// Tears down after the test.
RoutingHandlerTest::~RoutingHandlerTest() {
  stopServer();
}

std::unique_ptr<apache::thrift::HTTP2RoutingHandler> createHTTP2RoutingHandler(
    ThriftServer* server) {
  auto h2_options = std::make_unique<proxygen::HTTPServerOptions>();
  h2_options->threads = static_cast<size_t>(server->getNumIOWorkerThreads());
  h2_options->idleTimeout = server->getIdleTimeout();
  h2_options->shutdownOn = {SIGINT, SIGTERM};
  h2_options->handlerFactories =
      RequestHandlerChain()
          .addThen<ThriftRequestHandlerFactory>(server->getThriftProcessor())
          .build();

  return std::make_unique<apache::thrift::HTTP2RoutingHandler>(
      std::move(h2_options));
}

// Event handler to attach to the Thrift server so we know when it is
// ready to serve and also so we can determine the port it is
// listening on.
class RoutingHandlerTestEventHandler : public server::TServerEventHandler {
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

void RoutingHandlerTest::startServer(RoutingType routingType) {
  handler_ = std::make_shared<StrictMock<TestServiceMock>>();
  auto cpp2PFac =
      std::make_shared<ThriftServerAsyncProcessorFactory<TestServiceMock>>(
          handler_);

  server_ = std::make_unique<ThriftServer>();

  // Setting port to 0 will cause a free port to be assigned.
  server_->setPort(0);
  server_->setProcessorFactory(cpp2PFac);
  auto eventHandler = std::make_shared<RoutingHandlerTestEventHandler>();
  server_->setServerEventHandler(eventHandler);

  if (routingType == RoutingType::RSOCKET) {
    server_->addRoutingHandler(
        std::make_unique<apache::thrift::RSRoutingHandler>(
            server_->getThriftProcessor()));
  }

  if (routingType == RoutingType::HTTP2) {
    server_->addRoutingHandler(createHTTP2RoutingHandler(server_.get()));
  }

  server_->setup();

  // Get the port that the server has bound to
  port_ = eventHandler->waitForPortAssignment();
  LOG(INFO) << "Server is running on port: " << port_;
}

void RoutingHandlerTest::stopServer() {
  if (server_) {
    server_->cleanUp();
    server_.reset();
    handler_.reset();
  }
}

void RoutingHandlerTest::connectToServer(
    folly::Function<void(std::unique_ptr<TestServiceAsyncClient>)> callMe) {
  LOG(INFO) << "connectToServer";

  auto mgr = ConnectionManager::getInstance();
  CHECK_GT(port_, 0) << "Check if the server has started already";
  auto connection = mgr->getConnection(FLAGS_host, port_);
  auto channel = ThriftClient::Ptr(
      new ThriftClient(connection, workerThread.getEventBase()));
  channel->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);

  auto client = std::make_unique<TestServiceAsyncClient>(std::move(channel));

  callMe(std::move(client));
}

void RoutingHandlerTest::TestRequestResponse_Simple() {
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

void RoutingHandlerTest::TestRequestResponse_MultipleClients() {
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

void RoutingHandlerTest::TestRequestResponse_ExpectedException() {
  EXPECT_THROW(
      connectToServer(
          [&](auto client) { client->sync_throwExpectedException(1); }),
      TestServiceException);
}

void RoutingHandlerTest::TestRequestResponse_UnexpectedException() {
  EXPECT_THROW(
      connectToServer(
          [&](auto client) { client->sync_throwUnexpectedException(2); }),
      apache::thrift::TApplicationException);
}
}
}
