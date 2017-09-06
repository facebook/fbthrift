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
#include <thrift/lib/cpp2/transport/core/testutil/TestServiceMock.h>

namespace apache {
namespace thrift {

class RoutingHandlerTest {
 public:
  enum class RoutingType { NONE = 0, HTTP = 1, HTTP2 = 2, RSOCKET = 3 };

  explicit RoutingHandlerTest(RoutingType routingType);
  virtual ~RoutingHandlerTest();

  void TestRequestResponse_Simple();
  void TestRequestResponse_MultipleClients();
  void TestRequestResponse_ExpectedException();
  void TestRequestResponse_UnexpectedException();

 protected:
  void startServer(RoutingType routingType);
  void stopServer();
  void connectToServer(
      folly::Function<
          void(std::unique_ptr<testutil::testservice::TestServiceAsyncClient>)>
          callMe);

 protected:
  std::shared_ptr<testing::StrictMock<testutil::testservice::TestServiceMock>>
      handler_;
  std::unique_ptr<ThriftServer> server_;
  uint16_t port_;
  folly::ScopedEventBaseThread workerThread;
};
}
}
