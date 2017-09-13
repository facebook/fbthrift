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

#include <gflags/gflags.h>

DECLARE_int32(num_client_connections);

namespace apache {
namespace thrift {

class H2RoutingHandlerTest : public testing::Test {
 public:
  H2RoutingHandlerTest() {
    routingHandlerTest_ = std::make_unique<RoutingHandlerTest>(
        RoutingHandlerTest::RoutingType::HTTP2);
  }

 protected:
  std::unique_ptr<RoutingHandlerTest> routingHandlerTest_;
};

TEST_F(H2RoutingHandlerTest, RequestResponse_Simple) {
  routingHandlerTest_->TestRequestResponse_Simple();
}

TEST_F(H2RoutingHandlerTest, RequestResponse_MultipleClients) {
  routingHandlerTest_->TestRequestResponse_MultipleClients();
}

TEST_F(H2RoutingHandlerTest, RequestResponse_ExpectedException) {
  routingHandlerTest_->TestRequestResponse_ExpectedException();
}

TEST_F(H2RoutingHandlerTest, RequestResponse_UnexpectedException) {
  routingHandlerTest_->TestRequestResponse_UnexpectedException();
}

} // namespace thrift
} // namespace apache
