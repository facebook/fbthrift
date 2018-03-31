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

#include <gflags/gflags.h>
#include <thrift/lib/cpp2/async/RSocketClientChannel.h>
#include <thrift/lib/cpp2/transport/core/testutil/TransportCompatibilityTest.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>

DECLARE_int32(num_client_connections);
DECLARE_string(transport); // ConnectionManager depends on this flag.

namespace apache {
namespace thrift {

using namespace rsocket;
using namespace testutil::testservice;
using namespace apache::thrift::transport;

class RSCompatibilityTest : public testing::Test {
 public:
  RSCompatibilityTest() {
    // override the default
    FLAGS_transport = "rsocket"; // client's transport

    compatibilityTest_ = std::make_unique<TransportCompatibilityTest>();
    compatibilityTest_->addRoutingHandler(
        std::make_unique<apache::thrift::RSRoutingHandler>(
            compatibilityTest_->getServer()->getThriftProcessor(),
            *compatibilityTest_->getServer()));
    compatibilityTest_->startServer();
  }

 protected:
  std::unique_ptr<TransportCompatibilityTest> compatibilityTest_;
};

TEST_F(RSCompatibilityTest, RequestResponse_Simple) {
  compatibilityTest_->TestRequestResponse_Simple();
}

TEST_F(RSCompatibilityTest, RequestResponse_Sync) {
  compatibilityTest_->TestRequestResponse_Sync();
}

TEST_F(RSCompatibilityTest, RequestResponse_MultipleClients) {
  compatibilityTest_->TestRequestResponse_MultipleClients();
}

TEST_F(RSCompatibilityTest, RequestResponse_ExpectedException) {
  compatibilityTest_->TestRequestResponse_ExpectedException();
}

TEST_F(RSCompatibilityTest, RequestResponse_UnexpectedException) {
  compatibilityTest_->TestRequestResponse_UnexpectedException();
}

// Warning: This test may be flaky due to use of timeouts.
TEST_F(RSCompatibilityTest, RequestResponse_Timeout) {
  compatibilityTest_->TestRequestResponse_Timeout();
}

TEST_F(RSCompatibilityTest, RequestResponse_Header) {
  compatibilityTest_->TestRequestResponse_Header();
}

TEST_F(RSCompatibilityTest, RequestResponse_Header_ExpectedException) {
  compatibilityTest_->TestRequestResponse_Header_ExpectedException();
}

TEST_F(RSCompatibilityTest, RequestResponse_Header_UnexpectedException) {
  compatibilityTest_->TestRequestResponse_Header_UnexpectedException();
}

TEST_F(RSCompatibilityTest, RequestResponse_Saturation) {
  compatibilityTest_->connectToServer([this](auto client) {
    EXPECT_CALL(*compatibilityTest_->handler_.get(), add_(3)).Times(2);
    // note that no EXPECT_CALL for add_(5)

    auto channel = static_cast<RSocketClientChannel*>(client->getChannel());
    channel->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { channel->setMaxPendingRequests(0u); });
    EXPECT_THROW(client->sync_add(5), TTransportException);

    channel->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { channel->setMaxPendingRequests(1u); });
    EXPECT_EQ(3, client->sync_add(3));
    EXPECT_EQ(6, client->sync_add(3));
  });
}

TEST_F(RSCompatibilityTest, RequestResponse_Connection_CloseNow) {
  compatibilityTest_->TestRequestResponse_Connection_CloseNow();
}

TEST_F(RSCompatibilityTest, RequestResponse_ServerQueueTimeout) {
  compatibilityTest_->TestRequestResponse_ServerQueueTimeout();
}

TEST_F(RSCompatibilityTest, RequestResponse_ResponseSizeTooBig) {
  compatibilityTest_->TestRequestResponse_ResponseSizeTooBig();
}

TEST_F(RSCompatibilityTest, Oneway_Simple) {
  compatibilityTest_->TestOneway_Simple();
}

TEST_F(RSCompatibilityTest, Oneway_WithDelay) {
  compatibilityTest_->TestOneway_WithDelay();
}

TEST_F(RSCompatibilityTest, Oneway_Saturation) {
  compatibilityTest_->connectToServer([this](auto client) {
    EXPECT_CALL(*compatibilityTest_->handler_.get(), addAfterDelay_(100, 5));
    EXPECT_CALL(*compatibilityTest_->handler_.get(), addAfterDelay_(50, 5));

    auto channel = static_cast<RSocketClientChannel*>(client->getChannel());
    channel->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { channel->setMaxPendingRequests(0u); });
    client->sync_addAfterDelay(0, 5);

    // the first call is not completed as the connection was saturated
    channel->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { channel->setMaxPendingRequests(1u); });

    // Client should be able to issue both of these functions as
    // SINGLE_REQUEST_NO_RESPONSE doesn't need to wait for server response
    client->sync_addAfterDelay(100, 5);
    client->sync_addAfterDelay(50, 5); // TODO: H2 fails in this call.
  });
}

TEST_F(RSCompatibilityTest, Oneway_UnexpectedException) {
  compatibilityTest_->TestOneway_UnexpectedException();
}

TEST_F(RSCompatibilityTest, Oneway_Connection_CloseNow) {
  compatibilityTest_->TestOneway_Connection_CloseNow();
}

TEST_F(RSCompatibilityTest, Oneway_ServerQueueTimeout) {
  compatibilityTest_->TestOneway_ServerQueueTimeout();
}

TEST_F(RSCompatibilityTest, RequestContextIsPreserved) {
  compatibilityTest_->TestRequestContextIsPreserved();
}

TEST_F(RSCompatibilityTest, BadPayload) {
  compatibilityTest_->TestBadPayload();
}

TEST_F(RSCompatibilityTest, EvbSwitch) {
  compatibilityTest_->TestEvbSwitch();
}

TEST_F(RSCompatibilityTest, EvbSwitch_Failure) {
  compatibilityTest_->TestEvbSwitch_Failure();
}

TEST_F(RSCompatibilityTest, CloseCallback) {
  compatibilityTest_->TestCloseCallback();
}

TEST_F(RSCompatibilityTest, ConnectionStats) {
  compatibilityTest_->TestConnectionStats();
}

TEST_F(RSCompatibilityTest, ObserverSendReceiveRequests) {
  compatibilityTest_->TestObserverSendReceiveRequests();
}

} // namespace thrift
} // namespace apache
