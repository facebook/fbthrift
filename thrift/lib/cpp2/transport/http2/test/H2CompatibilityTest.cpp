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

enum ChannelType {
  Default,
  SingleRpc,
  MetadataInBody,
  MultiRpc,
};

class H2CompatibilityTest : public testing::Test,
                            public testing::WithParamInterface<ChannelType> {
 public:
  H2CompatibilityTest() {
    FLAGS_transport = "http2"; // client's transport
    switch (GetParam()) {
      case Default:
        // Default behavior is to let the negotiation happen as normal.
        FLAGS_force_channel_version = 0;
        break;
      case SingleRpc:
        FLAGS_force_channel_version = 1;
        break;
      case MetadataInBody:
        FLAGS_force_channel_version = 2;
        break;
      case MultiRpc:
        FLAGS_force_channel_version = 3;
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

TEST_P(H2CompatibilityTest, RequestResponse_Simple) {
  compatibilityTest_->TestRequestResponse_Simple();
}

TEST_P(H2CompatibilityTest, RequestResponse_Sync) {
  compatibilityTest_->TestRequestResponse_Sync();
}

TEST_P(H2CompatibilityTest, RequestResponse_MultipleClients) {
  compatibilityTest_->TestRequestResponse_MultipleClients();
}

TEST_P(H2CompatibilityTest, RequestResponse_ExpectedException) {
  compatibilityTest_->TestRequestResponse_ExpectedException();
}

TEST_P(H2CompatibilityTest, RequestResponse_UnexpectedException) {
  compatibilityTest_->TestRequestResponse_UnexpectedException();
}

// Warning: This test may be flaky due to use of timeouts.
TEST_P(H2CompatibilityTest, RequestResponse_Timeout) {
  compatibilityTest_->TestRequestResponse_Timeout();
}

TEST_P(H2CompatibilityTest, RequestResponse_Header) {
  compatibilityTest_->TestRequestResponse_Header();
}

TEST_P(H2CompatibilityTest, RequestResponse_Header_ExpectedException) {
  compatibilityTest_->TestRequestResponse_Header_ExpectedException();
}

TEST_P(H2CompatibilityTest, RequestResponse_Header_UnexpectedException) {
  compatibilityTest_->TestRequestResponse_Header_UnexpectedException();
}

TEST_P(H2CompatibilityTest, RequestResponse_Saturation) {
  // TODO:
  // This tests fails for MultiRpc.  We need to fix this.
  if (GetParam() != MultiRpc) {
    compatibilityTest_->TestRequestResponse_Saturation();
  }
}

TEST_P(H2CompatibilityTest, RequestResponse_Connection_CloseNow) {
  compatibilityTest_->TestRequestResponse_Connection_CloseNow();
}

TEST_P(H2CompatibilityTest, RequestResponse_ServerQueueTimeout) {
  // TODO:
  // This test fails for Default where handshake converts legacy
  // behavior to MultiRpcChannel after handshake is completed.
  // Need to investigate this.
  if (GetParam() != Default) {
    compatibilityTest_->TestRequestResponse_ServerQueueTimeout();
  }
}

TEST_P(H2CompatibilityTest, RequestResponse_ResponseSizeTooBig) {
  compatibilityTest_->TestRequestResponse_ResponseSizeTooBig();
}

TEST_P(H2CompatibilityTest, Oneway_Simple) {
  compatibilityTest_->TestOneway_Simple();
}

TEST_P(H2CompatibilityTest, Oneway_WithDelay) {
  compatibilityTest_->TestOneway_WithDelay();
}

TEST_P(H2CompatibilityTest, Oneway_Saturation) {
  // TODO:
  // This tests fails for MultiRpc.  We need to fix this.
  if (GetParam() != MultiRpc) {
    compatibilityTest_->TestOneway_Saturation();
  }
}

TEST_P(H2CompatibilityTest, Oneway_UnexpectedException) {
  compatibilityTest_->TestOneway_UnexpectedException();
}

TEST_P(H2CompatibilityTest, Oneway_Connection_CloseNow) {
  compatibilityTest_->TestOneway_Connection_CloseNow();
}

TEST_P(H2CompatibilityTest, Oneway_ServerQueueTimeout) {
  compatibilityTest_->TestOneway_ServerQueueTimeout();
}

TEST_P(H2CompatibilityTest, RequestContextIsPreserved) {
  compatibilityTest_->TestRequestContextIsPreserved();
}

TEST_P(H2CompatibilityTest, BadPayload) {
  // Default and SingleRpc versions don't extract metadata from the payload, so
  // they will just send the payload to the server.
  if (GetParam() != ChannelType::Default &&
      GetParam() != ChannelType::SingleRpc) {
    compatibilityTest_->TestBadPayload();
  }
}

TEST_P(H2CompatibilityTest, EvbSwitch) {
  if (GetParam() != ChannelType::Default &&
      GetParam() != ChannelType::MultiRpc) {
    compatibilityTest_->TestEvbSwitch();
  }
}

INSTANTIATE_TEST_CASE_P(
    WithAndWithoutMetadataInBody,
    H2CompatibilityTest,
    testing::Values(
        ChannelType::Default,
        ChannelType::SingleRpc,
        ChannelType::MetadataInBody,
        ChannelType::MultiRpc));

} // namespace thrift
} // namespace apache
