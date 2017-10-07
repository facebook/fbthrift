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
#include <thrift/lib/cpp2/transport/core/testutil/TransportCompatibilityTest.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>

DECLARE_int32(num_client_connections);
DECLARE_string(transport); // ConnectionManager depends on this flag.

namespace apache {
namespace thrift {

using namespace rsocket;
using namespace testutil::testservice;

class RSCompatibilityTest : public testing::Test {
 public:
  RSCompatibilityTest() {
    // override the default
    FLAGS_transport = "rsocket"; // client's transport

    compatibilityTest_ = std::make_unique<TransportCompatibilityTest>();
    compatibilityTest_->addRoutingHandler(
        std::make_unique<apache::thrift::RSRoutingHandler>(
            compatibilityTest_->getServer()->getThriftProcessor()));
    compatibilityTest_->startServer();
  }

 protected:
  std::unique_ptr<TransportCompatibilityTest> compatibilityTest_;
};

TEST_F(RSCompatibilityTest, RequestResponse_Simple) {
  compatibilityTest_->TestRequestResponse_Simple();
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

/* TODO: uncomment after timeouts are implemented
// Warning: This test may be flaky due to use of timeouts.
TEST_F(RSCompatibilityTest, RequestResponse_Timeout) {
  compatibilityTest_->TestRequestResponse_Timeout();
}
*/

TEST_F(RSCompatibilityTest, RequestResponse_Header) {
  compatibilityTest_->TestRequestResponse_Header();
}

TEST_F(RSCompatibilityTest, RequestResponse_Header_ExpectedException) {
  compatibilityTest_->TestRequestResponse_Header_ExpectedException();
}

TEST_F(RSCompatibilityTest, RequestResponse_Header_UnexpectedException) {
  compatibilityTest_->TestRequestResponse_Header_UnexpectedException();
}

} // namespace thrift
} // namespace apache
