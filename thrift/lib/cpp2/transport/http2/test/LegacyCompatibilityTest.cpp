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
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <thrift/lib/cpp2/transport/core/testutil/TransportCompatibilityTest.h>
#include <thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.h>

DECLARE_uint32(force_channel_version);
DECLARE_string(transport);

namespace apache {
namespace thrift {

using proxygen::RequestHandlerChain;

std::unique_ptr<HTTP2RoutingHandler> createHTTP2RoutingHandler(
    ThriftServer* server) {
  auto h2_options = std::make_unique<proxygen::HTTPServerOptions>();
  h2_options->threads = static_cast<size_t>(server->getNumIOWorkerThreads());
  h2_options->idleTimeout = server->getIdleTimeout();
  h2_options->shutdownOn = {SIGINT, SIGTERM};

  return std::make_unique<HTTP2RoutingHandler>(
      std::move(h2_options), server->getThriftProcessor());
}

// Only these two channel types are compatible with legacy.
enum ChannelType {
  Default,
  SingleRPC,
};

class LegacyCompatibilityTest
    : public testing::Test,
      public testing::WithParamInterface<ChannelType> {
 public:
  LegacyCompatibilityTest() {
    FLAGS_transport = "legacy-http2"; // client's transport
    switch (GetParam()) {
      case Default:
        // Default behavior is to let the negotiation happen as normal.
        FLAGS_force_channel_version = 0;
        break;
      case SingleRPC:
        FLAGS_force_channel_version = 1;
        break;
    }

    compatibilityTest_ = std::make_unique<TransportCompatibilityTest>();
    compatibilityTest_->addRoutingHandler(
        createHTTP2RoutingHandler(compatibilityTest_->getServer()));
    compatibilityTest_->startServer();
  }

 protected:
  std::unique_ptr<TransportCompatibilityTest> compatibilityTest_;
};

TEST_P(LegacyCompatibilityTest, RequestResponse_Sync) {
  compatibilityTest_->TestRequestResponse_Sync();
}

TEST_P(LegacyCompatibilityTest, RequestResponse_ResponseSizeTooBig) {
  compatibilityTest_->TestRequestResponse_ResponseSizeTooBig();
}

TEST_P(LegacyCompatibilityTest, Oneway_Simple) {
  compatibilityTest_->TestOneway_Simple();
}

TEST_P(LegacyCompatibilityTest, Oneway_WithDelay) {
  compatibilityTest_->TestOneway_WithDelay();
}

TEST_P(LegacyCompatibilityTest, Oneway_UnexpectedException) {
  compatibilityTest_->TestOneway_UnexpectedException();
}

INSTANTIATE_TEST_CASE_P(
    WithAndWithoutMetadataInBody,
    LegacyCompatibilityTest,
    testing::Values(ChannelType::Default, ChannelType::SingleRPC));

} // namespace thrift
} // namespace apache
