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

#include <folly/ScopeGuard.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/core/testutil/TestServiceMock.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSResponder.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSThriftServer.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>
#include "thrift/lib/cpp2/transport/core/testutil/gen-cpp2/TestService.h"

// ConnectionManager depends on this flag.
DECLARE_string(transport);

namespace apache {
namespace thrift {

using namespace testing;

DEFINE_string(host, "localhost", "host to connect to");

using namespace apache::thrift;
using namespace rsocket;
using namespace testutil::testservice;

class RSRequestResponseTest : public testing::Test {
 public:
  RSRequestResponseTest() : workerThread("RSRequestResponseTest_WorkerThread") {
    FLAGS_transport = "rsocket";

    startServer();
  }

  // Tears down after the test.
  ~RSRequestResponseTest() override {
    stopServer();
  }

  void startServer() {
    handler_ = std::make_shared<StrictMock<TestServiceMock>>();
    auto cpp2PFac =
        std::make_shared<ThriftServerAsyncProcessorFactory<TestServiceMock>>(
            handler_);

    server_ = std::make_unique<RSThriftServer>();
    server_->setPort(0);
    server_->setProcessorFactory(cpp2PFac);

    server_->serve();
    port_ = (uint16_t)server_->getPort();
  }

  void stopServer() {
    if (server_) {
      server_->stop();
      server_.reset();
      handler_.reset();
    }
  }

  void connectToServer(
      folly::Function<void(std::unique_ptr<TestServiceAsyncClient>)> callMe) {
    auto mgr = ConnectionManager::getInstance();
    auto connection = mgr->getConnection(FLAGS_host, port_);
    auto channel = ThriftClient::Ptr(
        new ThriftClient(connection, workerThread.getEventBase()));
    channel->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);

    auto client = std::make_unique<TestServiceAsyncClient>(std::move(channel));

    callMe(std::move(client));
  }

 protected:
  std::shared_ptr<StrictMock<TestServiceMock>> handler_;
  std::unique_ptr<RSThriftServer> server_;
  uint16_t port_;
  folly::ScopedEventBaseThread workerThread;
};

TEST_F(RSRequestResponseTest, RequestResponse_Simple) {
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

TEST_F(RSRequestResponseTest, RequestResponse_ExpectedException) {
  EXPECT_THROW(
      connectToServer(
          [&](auto client) { client->sync_throwExpectedException(1); }),
      TestServiceException);
}

TEST_F(RSRequestResponseTest, RequestResponse_UnexpectedException) {
  EXPECT_THROW(
      connectToServer(
          [&](auto client) { client->sync_throwUnexpectedException(2); }),
      apache::thrift::TApplicationException);
}
}
}
