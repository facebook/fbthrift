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

#pragma once

#include <folly/io/async/ScopedEventBaseThread.h>
#include <gmock/gmock.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>
#include <thrift/lib/cpp2/transport/core/TransportRoutingHandler.h>
#include <thrift/lib/cpp2/transport/core/testutil/TestServiceMock.h>

namespace apache {
namespace thrift {

// Transport layer compliance tests.
class TransportCompatibilityTest {
 public:
  TransportCompatibilityTest();
  virtual ~TransportCompatibilityTest();

  // Don't forget to start the server before running the tests.
  void startServer();

  void addRoutingHandler(
      std::unique_ptr<TransportRoutingHandler> routingHandler);
  ThriftServer* getServer();

  void TestRequestResponse_Simple();
  void TestRequestResponse_MultipleClients();
  void TestRequestResponse_ExpectedException();
  void TestRequestResponse_UnexpectedException();
  void TestRequestResponse_Timeout();
  void TestRequestResponse_Header();
  void TestRequestResponse_Header_ExpectedException();
  void TestRequestResponse_Header_UnexpectedException();
  void TestRequestResponse_Saturation();
  void TestRequestResponse_Connection_CloseNow();
  void TestRequestResponse_ServerQueueTimeout();
  void TestRequestResponse_ResponseSizeTooBig();

  void TestOneway_Simple();
  void TestOneway_WithDelay();
  void TestOneway_Saturation();
  void TestOneway_UnexpectedException();
  void TestOneway_Connection_CloseNow();
  void TestOneway_ServerQueueTimeout();

  void TestRequestContextIsPreserved();

 protected:
  void setupServer();
  void stopServer();
  void connectToServer(
      folly::Function<
          void(std::unique_ptr<testutil::testservice::TestServiceAsyncClient>)>
          callMe);
  void connectToServer(
      folly::Function<void(
          std::unique_ptr<testutil::testservice::TestServiceAsyncClient>,
          std::shared_ptr<ClientConnectionIf>)> callMe);

  void callSleep(
      testutil::testservice::TestServiceAsyncClient* client,
      int32_t timeoutMs,
      int32_t sleepMs);

  std::shared_ptr<testing::StrictMock<testutil::testservice::TestServiceMock>>
      handler_;
  std::unique_ptr<ThriftServer> server_;
  uint16_t port_;
  folly::ScopedEventBaseThread workerThread_;
};

} // namespace thrift
} // namespace apache
