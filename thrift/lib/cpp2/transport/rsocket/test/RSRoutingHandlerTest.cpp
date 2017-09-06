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

#include <thrift/lib/cpp2/transport/core/testutil/RoutingHandlerTest.h>

namespace apache {
namespace thrift {

class RSRoutingHandlerTest : public testing::Test {
 public:
  RSRoutingHandlerTest() {
    routingHandlerTest_ = std::make_unique<RoutingHandlerTest>(
        RoutingHandlerTest::RoutingType::RSOCKET);
  }

 protected:
  std::unique_ptr<RoutingHandlerTest> routingHandlerTest_;
};

TEST_F(RSRoutingHandlerTest, RequestResponse_Simple) {
  routingHandlerTest_->TestRequestResponse_Simple();
}

TEST_F(RSRoutingHandlerTest, RequestResponse_MultipleClients) {
  routingHandlerTest_->TestRequestResponse_MultipleClients();
}

TEST_F(RSRoutingHandlerTest, RequestResponse_ExpectedException) {
  routingHandlerTest_->TestRequestResponse_ExpectedException();
}

TEST_F(RSRoutingHandlerTest, RequestResponse_UnepectedException) {
  routingHandlerTest_->TestRequestResponse_UnexpectedException();
}
}
}
