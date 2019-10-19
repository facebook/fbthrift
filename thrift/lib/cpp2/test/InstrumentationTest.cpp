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
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/async/FutureRequest.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/server/ServerInstrumentation.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/InstrumentationTestService.h>

#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using apache::thrift::ServerInstrumentation;
using apache::thrift::ThriftServer;
using apache::thrift::test::cpp2::InstrumentationTestServiceSvIf;

class InstrumentationTest : public testing::Test {};

class TestInterface : public InstrumentationTestServiceSvIf {};

TEST_F(InstrumentationTest, simpleServerTest) {
  ThriftServer server0;
  {
    auto handler = std::make_shared<TestInterface>();
    apache::thrift::ScopedServerInterfaceThread server1(handler);
    EXPECT_EQ(ServerInstrumentation::getServerCount(), 2);
  }
  EXPECT_EQ(ServerInstrumentation::getServerCount(), 1);
}
