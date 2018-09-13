/*
 * Copyright 2018-present Facebook, Inc.
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

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Format.h>
#include <folly/Range.h>
#include <folly/SocketAddress.h>
#include <folly/Try.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>
#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>
#include <thrift/lib/cpp2/transport/rocket/test/network/ClientServerTestUtil.h>

using namespace apache::thrift;
using namespace apache::thrift::rocket;
using namespace apache::thrift::rocket::test;
using namespace apache::thrift::transport;

namespace {
std::string repeatPattern(folly::StringPiece pattern, size_t nbytes) {
  std::string rv;
  rv.reserve(nbytes);
  for (size_t remaining = nbytes; remaining != 0;) {
    const size_t toCopy = std::min<size_t>(pattern.size(), remaining);
    std::copy_n(pattern.begin(), toCopy, std::back_inserter(rv));
    remaining -= toCopy;
  }
  return rv;
}

folly::StringPiece getRange(folly::IOBuf& iobuf) {
  return folly::StringPiece(iobuf.coalesce());
}

void expectTransportExceptionType(
    TTransportException::TTransportExceptionType expectedType,
    folly::exception_wrapper ew) {
  const auto* const tex =
      dynamic_cast<transport::TTransportException*>(ew.get_exception());
  EXPECT_NE(nullptr, tex);
  EXPECT_EQ(expectedType, tex->getType());
}

void expectRocketExceptionType(
    ErrorCode expectedCode,
    folly::exception_wrapper ew) {
  const auto* const rex = dynamic_cast<RocketException*>(ew.get_exception());
  EXPECT_NE(nullptr, rex);
  EXPECT_EQ(expectedCode, rex->getErrorCode());
}

class RocketClientTest : public testing::Test {
 protected:
  void SetUp() override {
    server_ = std::make_unique<RsocketTestServer>();
    client_ = std::make_unique<RocketTestClient>(
        folly::SocketAddress("::1", server_->getListeningPort()));
  }

  void TearDown() override {
    client_.reset();
    server_.reset();
  }

 public:
  void withClient(folly::Function<void(RocketTestClient&)> f) {
    f(*client_);
  }

 protected:
  std::unique_ptr<RsocketTestServer> server_;
  std::unique_ptr<RocketTestClient> client_;
};
} // namespace

/**
 * REQUEST_RESPONSE tests
 */
TEST_F(RocketClientTest, RequestResponseBasic) {
  withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata("metadata");
    constexpr folly::StringPiece kData("test_request");

    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, kData));

    EXPECT_TRUE(reply.hasValue());
    EXPECT_EQ(kData, getRange(reply->data()));
    EXPECT_TRUE(reply->metadata());
    EXPECT_EQ(kMetadata, getRange(*reply->metadata()));
  });
}

TEST_F(RocketClientTest, RequestResponseTimeout) {
  withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata("metadata");
    constexpr folly::StringPiece kData("sleep_ms:200");

    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, kData),
        std::chrono::milliseconds(100));

    EXPECT_TRUE(reply.hasException());
    expectTransportExceptionType(
        TTransportException::TTransportExceptionType::TIMED_OUT,
        std::move(reply.exception()));
  });
}

TEST_F(RocketClientTest, RequestResponseLargeMetadata) {
  withClient([](RocketTestClient& client) {
    // Ensure metadata will be split across multiple frames
    constexpr size_t kReplyMetadataSize = 0x2ffffff;
    constexpr folly::StringPiece kPattern =
        "abcdefghijklmnopqrstuvwxyz0123456789";

    constexpr folly::StringPiece kMetadata("metadata");
    const auto expectedMetadata = repeatPattern(kPattern, kReplyMetadataSize);
    const std::string data =
        folly::to<std::string>("metadata_echo:", expectedMetadata);

    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, folly::StringPiece{data}),
        std::chrono::seconds(5));

    EXPECT_TRUE(reply.hasValue());
    EXPECT_TRUE(reply->hasNonemptyMetadata());
    EXPECT_EQ(expectedMetadata, getRange(*reply->metadata()));
    EXPECT_EQ(data, getRange(reply->data()));
  });
}

TEST_F(RocketClientTest, RequestResponseLargeData) {
  withClient([](RocketTestClient& client) {
    // Ensure metadata will be split across multiple frames
    constexpr size_t kReplyDataSize = 0x2ffffff;
    constexpr folly::StringPiece kPattern =
        "abcdefghijklmnopqrstuvwxyz0123456789";

    constexpr folly::StringPiece kMetadata{"metadata"};
    const auto expectedData = repeatPattern(kPattern, kReplyDataSize);
    const std::string data = folly::to<std::string>("data_echo:", expectedData);

    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, folly::StringPiece{data}),
        std::chrono::seconds(5));

    EXPECT_TRUE(reply.hasValue());
    EXPECT_TRUE(reply->hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*reply->metadata()));
    EXPECT_EQ(expectedData, getRange(reply->data()));
  });
}

TEST_F(RocketClientTest, RequestResponseEmptyMetadata) {
  withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata{"metadata"};
    constexpr folly::StringPiece kData{"metadata_echo:"};

    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, kData));

    EXPECT_TRUE(reply.hasValue());
    EXPECT_FALSE(reply->hasNonemptyMetadata());
    // Parser should never construct empty metadata
    EXPECT_FALSE(reply->metadata());
  });
}

TEST_F(RocketClientTest, RequestResponseEmptyData) {
  withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata{"metadata"};
    constexpr folly::StringPiece kData{"data_echo:"};

    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, kData));

    EXPECT_TRUE(reply.hasValue());
    EXPECT_TRUE(reply->hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*reply->metadata()));
    EXPECT_TRUE(reply->data().empty());
  });
}

TEST_F(RocketClientTest, RequestResponseError) {
  withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata{"metadata"};
    constexpr folly::StringPiece kData{"error:application"};

    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, kData));

    EXPECT_TRUE(reply.hasException());
    expectRocketExceptionType(
        ErrorCode::APPLICATION_ERROR, std::move(reply.exception()));
  });
}

TEST_F(RocketClientTest, RequestResponseDeadServer) {
  constexpr folly::StringPiece kMetadata{"metadata"};
  constexpr folly::StringPiece kData{"data"};

  server_.reset();

  auto reply = client_->sendRequestResponseSync(
      Payload::makeFromMetadataAndData(kMetadata, kData),
      std::chrono::milliseconds(250));

  EXPECT_TRUE(reply.hasException());
  expectTransportExceptionType(
      TTransportException::TTransportExceptionType::NOT_OPEN,
      std::move(reply.exception()));
}

TEST_F(RocketClientTest, RocketClientEventBaseDestruction) {
  auto evb = std::make_unique<folly::EventBase>();
  folly::AsyncSocket::UniquePtr socket(new folly::AsyncSocket(
      evb.get(), folly::SocketAddress("::1", server_->getListeningPort())));
  auto client = RocketClient::create(*evb, std::move(socket));
  EXPECT_NE(nullptr, client->getTransportWrapper());

  evb.reset();
  EXPECT_EQ(nullptr, client->getTransportWrapper());
}

/**
 * REQUEST_FNF tests
 */
TEST_F(RocketClientTest, RequestFnfBasic) {
  withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata("metadata");
    constexpr folly::StringPiece kData("test_request");

    auto reply = client.sendRequestFnfSync(
        Payload::makeFromMetadataAndData(kMetadata, kData));

    EXPECT_TRUE(reply.hasValue());
  });
}
