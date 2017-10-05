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

#include <thrift/lib/cpp2/transport/core/testutil/ComplianceTest.h>

#include <gflags/gflags.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>

DECLARE_int32(num_client_connections);
DECLARE_string(transport); // ConnectionManager depends on this flag.

namespace apache {
namespace thrift {

using namespace rsocket;
using namespace testutil::testservice;

class RSComplianceTest : public testing::Test {
 public:
  RSComplianceTest() {
    // override the default
    FLAGS_transport = "rsocket"; // client's transport

    complianceTest_ = std::make_unique<ComplianceTest>();
    complianceTest_->addRoutingHandler(
        std::make_unique<apache::thrift::RSRoutingHandler>(
            complianceTest_->getServer()->getThriftProcessor()));
    complianceTest_->startServer();
  }

 protected:
  std::unique_ptr<ComplianceTest> complianceTest_;
};

TEST_F(RSComplianceTest, RequestResponse_Simple) {
  complianceTest_->TestRequestResponse_Simple();
}

TEST_F(RSComplianceTest, RequestResponse_MultipleClients) {
  complianceTest_->TestRequestResponse_MultipleClients();
}

TEST_F(RSComplianceTest, RequestResponse_ExpectedException) {
  complianceTest_->TestRequestResponse_ExpectedException();
}

TEST_F(RSComplianceTest, RequestResponse_UnexpectedException) {
  complianceTest_->TestRequestResponse_UnexpectedException();
}

/* TODO: uncomment after timeouts are implemented
// Warning: This test may be flaky due to use of timeouts.
TEST_F(RSComplianceTest, RequestResponse_Timeout) {
  complianceTest_->TestRequestResponse_Timeout();
}
*/

} // namespace thrift
} // namespace apache
