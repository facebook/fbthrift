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

#include <chrono>
#include <string>
#include <utility>

#include <folly/portability/GTest.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Format.h>
#include <folly/Range.h>
#include <folly/SocketAddress.h>
#include <folly/Try.h>
#include <folly/executors/ManualExecutor.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClientWriteCallback.h>
#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>
#include <thrift/lib/cpp2/transport/rocket/test/network/ClientServerTestUtil.h>
#include <thrift/lib/cpp2/transport/rocket/test/network/Util.h>

using namespace apache::thrift;
using namespace apache::thrift::rocket;
using namespace apache::thrift::rocket::test;
using namespace apache::thrift::transport;

namespace {
// Used for testing RocketClient against rsocket-cpp server and for testing
// RocketClient against RocketTestServer
template <class Server>
class RocketNetworkTest : public testing::Test {
 protected:
  void SetUp() override {
    server_ = std::make_unique<Server>();
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

  folly::ManualExecutor* getUserExecutor() {
    return &userExecutor_;
  }

 protected:
  std::unique_ptr<Server> server_;
  std::unique_ptr<RocketTestClient> client_;
  folly::ManualExecutor userExecutor_;
};

struct OnWriteSuccess : RocketClientWriteCallback {
  bool writeSuccess{false};

  void onWriteSuccess() noexcept final {
    writeSuccess = true;
  }
};
} // namespace

using ServerTypes = ::testing::Types<RsocketTestServer, RocketTestServer>;
TYPED_TEST_CASE(RocketNetworkTest, ServerTypes);

/**
 * REQUEST_RESPONSE tests
 */
TYPED_TEST(RocketNetworkTest, RequestResponseBasic) {
  this->withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata("metadata");
    constexpr folly::StringPiece kData("test_request");

    OnWriteSuccess writeCallback;
    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, kData),
        std::chrono::milliseconds(250) /* timeout */,
        &writeCallback);

    EXPECT_TRUE(writeCallback.writeSuccess);
    EXPECT_TRUE(reply.hasValue());
    EXPECT_EQ(kData, getRange(*reply->data()));
    EXPECT_TRUE(reply->metadata());
    EXPECT_EQ(kMetadata, getRange(*reply->metadata()));
  });
}

TYPED_TEST(RocketNetworkTest, RequestResponseTimeout) {
  this->withClient([](RocketTestClient& client) {
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

TYPED_TEST(RocketNetworkTest, RequestResponseLargeMetadata) {
  this->withClient([](RocketTestClient& client) {
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
    EXPECT_EQ(data, getRange(*reply->data()));
  });
}

TYPED_TEST(RocketNetworkTest, RequestResponseLargeData) {
  this->withClient([](RocketTestClient& client) {
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
    EXPECT_EQ(expectedData, getRange(*reply->data()));
  });
}

TYPED_TEST(RocketNetworkTest, RequestResponseEmptyMetadata) {
  this->withClient([](RocketTestClient& client) {
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

TYPED_TEST(RocketNetworkTest, RequestResponseEmptyData) {
  this->withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata{"metadata"};
    constexpr folly::StringPiece kData{"data_echo:"};

    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, kData));

    EXPECT_TRUE(reply.hasValue());
    EXPECT_TRUE(reply->hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*reply->metadata()));
    EXPECT_TRUE(reply->data()->empty());
  });
}

TYPED_TEST(RocketNetworkTest, RequestResponseError) {
  this->withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata{"metadata"};
    constexpr folly::StringPiece kData{"error:application"};

    auto reply = client.sendRequestResponseSync(
        Payload::makeFromMetadataAndData(kMetadata, kData));

    EXPECT_TRUE(reply.hasException());
    expectRocketExceptionType(
        ErrorCode::APPLICATION_ERROR, std::move(reply.exception()));
  });
}

TYPED_TEST(RocketNetworkTest, RequestResponseDeadServer) {
  constexpr folly::StringPiece kMetadata{"metadata"};
  constexpr folly::StringPiece kData{"data"};

  this->server_.reset();

  OnWriteSuccess writeCallback;
  auto reply = this->client_->sendRequestResponseSync(
      Payload::makeFromMetadataAndData(kMetadata, kData),
      std::chrono::milliseconds(250),
      &writeCallback);

  EXPECT_FALSE(writeCallback.writeSuccess);
  EXPECT_TRUE(reply.hasException());
  expectTransportExceptionType(
      TTransportException::TTransportExceptionType::NOT_OPEN,
      std::move(reply.exception()));
}

TYPED_TEST(RocketNetworkTest, RocketClientEventBaseDestruction) {
  auto evb = std::make_unique<folly::EventBase>();
  folly::AsyncSocket::UniquePtr socket(new folly::AsyncSocket(
      evb.get(),
      folly::SocketAddress("::1", this->server_->getListeningPort())));
  auto client = RocketClient::create(*evb, std::move(socket));
  EXPECT_NE(nullptr, client->getTransportWrapper());

  evb.reset();
  EXPECT_EQ(nullptr, client->getTransportWrapper());
}

/**
 * REQUEST_FNF tests
 */
TYPED_TEST(RocketNetworkTest, RequestFnfBasic) {
  this->withClient([](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata("metadata");
    constexpr folly::StringPiece kData("test_request");

    auto reply = client.sendRequestFnfSync(
        Payload::makeFromMetadataAndData(kMetadata, kData));

    EXPECT_TRUE(reply.hasValue());
  });
}

/**
 * REQUEST_STREAM tests
 */
TYPED_TEST(RocketNetworkTest, RequestStreamBasic) {
  this->withClient([this](RocketTestClient& client) {
    constexpr size_t kNumRequestedPayloads = 200;
    constexpr folly::StringPiece kMetadata("metadata");
    const auto data =
        folly::to<std::string>("generate:", kNumRequestedPayloads);

    auto stream = client.sendRequestStreamSync(
        Payload::makeFromMetadataAndData(kMetadata, folly::StringPiece{data}));
    EXPECT_TRUE(stream.hasValue());

    size_t received = 0;
    auto subscription =
        std::move(*stream)
            .via(this->getUserExecutor())
            .subscribe(
                [&received](Payload&& payload) {
                  const auto x = folly::to<size_t>(getRange(*payload.data()));
                  EXPECT_EQ(received++, x);
                },
                [](auto ew) { FAIL() << ew.what(); });

    std::move(subscription).futureJoin().waitVia(this->getUserExecutor());
    EXPECT_EQ(kNumRequestedPayloads, received);
  });
}

TYPED_TEST(RocketNetworkTest, RequestStreamError) {
  this->withClient([this](RocketTestClient& client) {
    constexpr folly::StringPiece kMetadata("metadata");
    constexpr folly::StringPiece kData("error:application");

    auto stream = client.sendRequestStreamSync(
        Payload::makeFromMetadataAndData(kMetadata, kData));
    EXPECT_TRUE(stream.hasValue());

    size_t received = 0;
    bool error = false;
    auto subscription =
        std::move(*stream)
            .via(this->getUserExecutor())
            .subscribe(
                [&received](Payload&& payload) {
                  const auto x = folly::to<size_t>(getRange(*payload.data()));
                  EXPECT_EQ(received++, x);
                },
                [&error](auto ew) {
                  error = true;
                  expectRocketExceptionType(
                      ErrorCode::APPLICATION_ERROR, std::move(ew));
                });

    std::move(subscription).futureJoin().waitVia(this->getUserExecutor());
    EXPECT_TRUE(error);
    EXPECT_EQ(0, received);
  });
}

TYPED_TEST(RocketNetworkTest, RequestStreamSmallInitialRequestN) {
  this->withClient([this](RocketTestClient& client) {
    constexpr size_t kNumRequestedPayloads = 200;
    constexpr folly::StringPiece kMetadata("metadata");
    const auto data =
        folly::to<std::string>("generate:", kNumRequestedPayloads);

    auto stream = client.sendRequestStreamSync(
        Payload::makeFromMetadataAndData(kMetadata, folly::StringPiece{data}));
    EXPECT_TRUE(stream.hasValue());

    size_t received = 0;
    auto subscription =
        std::move(*stream)
            .via(this->getUserExecutor())
            .subscribe(
                [&received](Payload&& payload) {
                  const auto x = folly::to<size_t>(getRange(*payload.data()));
                  EXPECT_EQ(received++, x);
                },
                [](auto ew) { FAIL() << ew.what(); },
                5 /* batch size */);

    std::move(subscription).futureJoin().waitVia(this->getUserExecutor());
    EXPECT_EQ(kNumRequestedPayloads, received);
  });
}

TYPED_TEST(RocketNetworkTest, RequestStreamCancelSubscription) {
  this->withClient([this](RocketTestClient& client) {
    // Open an essentially infinite stream and ensure stream is able to be
    // canceled within a reasonable amount of time.
    constexpr size_t kNumRequestedPayloads =
        std::numeric_limits<int32_t>::max();
    constexpr folly::StringPiece kMetadata("metadata");
    const auto data =
        folly::to<std::string>("generate:", kNumRequestedPayloads);

    auto stream = client.sendRequestStreamSync(
        Payload::makeFromMetadataAndData(kMetadata, folly::StringPiece{data}));
    EXPECT_TRUE(stream.hasValue());

    size_t received = 0;
    auto subscription =
        std::move(*stream)
            .via(this->getUserExecutor())
            .subscribe(
                [&received](Payload&& payload) {
                  const auto x = folly::to<size_t>(getRange(*payload.data()));
                  EXPECT_EQ(received++, x);
                },
                [](auto ew) { FAIL() << ew.what(); });

    subscription.cancel();
    std::move(subscription).futureJoin().waitVia(this->getUserExecutor());
    EXPECT_LT(received, kNumRequestedPayloads);
  });
}

TYPED_TEST(RocketNetworkTest, RequestStreamNeverSubscribe) {
  this->withClient([](RocketTestClient& client) {
    constexpr size_t kNumRequestedPayloads = 200;
    constexpr folly::StringPiece kMetadata("metadata");
    const auto data =
        folly::to<std::string>("generate:", kNumRequestedPayloads);

    {
      auto stream =
          client.sendRequestStreamSync(Payload::makeFromMetadataAndData(
              kMetadata, folly::StringPiece{data}));
      EXPECT_TRUE(stream.hasValue());
    }
  });
}

TYPED_TEST(RocketNetworkTest, RequestStreamCloseClient) {
  constexpr size_t kNumRequestedPayloads = 200;
  constexpr folly::StringPiece kMetadata("metadata");
  const auto data = folly::to<std::string>("generate:", kNumRequestedPayloads);

  auto stream = this->client_->sendRequestStreamSync(
      Payload::makeFromMetadataAndData(kMetadata, folly::StringPiece{data}));
  EXPECT_TRUE(stream.hasValue());

  bool onErrorCalled = false;
  auto subscription =
      std::move(*stream)
          .via(this->getUserExecutor())
          .subscribe(
              [](Payload&&) {},
              [&](auto ew) {
                onErrorCalled = true;
                expectTransportExceptionType(
                    TTransportException::TTransportExceptionType::NOT_OPEN,
                    std::move(ew));
              });

  this->client_.reset();

  std::move(subscription).futureJoin().waitVia(this->getUserExecutor());
  EXPECT_TRUE(onErrorCalled);
}

TYPED_TEST(RocketNetworkTest, ClientCreationAndReconnectStreamOutlivesClient) {
  this->withClient([](RocketTestClient& client) {
    constexpr size_t kNumRequestedPayloads = 1000000;
    constexpr folly::StringPiece kMetadata("metadata");
    const auto data =
        folly::to<std::string>("generate:", kNumRequestedPayloads);

    // Open a stream and reconnect many times, having each stream slightly
    // outlive its associated RocketClient.
    for (size_t i = 0; i < 1000; ++i) {
      auto stream =
          client.sendRequestStreamSync(Payload::makeFromMetadataAndData(
              kMetadata, folly::StringPiece{data}));
      EXPECT_TRUE(stream.hasValue());
      client.reconnect();
    }
  });
}

TYPED_TEST(
    RocketNetworkTest,
    ClientCreationAndReconnectSubscriptionOutlivesClient) {
  this->withClient([this](RocketTestClient& client) {
    constexpr size_t kNumRequestedPayloads = 1000000;
    constexpr folly::StringPiece kMetadata("metadata");
    const auto data =
        folly::to<std::string>("generate:", kNumRequestedPayloads);

    // Open a stream and reconnect many times, subscribing to the stream before
    // and having the subscription outlive the client.
    for (size_t i = 0; i < 1000; ++i) {
      auto stream =
          client.sendRequestStreamSync(Payload::makeFromMetadataAndData(
              kMetadata, folly::StringPiece{data}));
      EXPECT_TRUE(stream.hasValue());
      size_t received = 0;
      auto subscription =
          std::move(*stream)
              .via(this->getUserExecutor())
              .subscribe(
                  [&received](Payload&& payload) {
                    const auto x = folly::to<size_t>(getRange(*payload.data()));
                    EXPECT_EQ(received++, x);
                  },
                  [](auto /* ew */) {});
      client.reconnect();
      subscription.cancel();
      std::move(subscription).futureJoin().waitVia(this->getUserExecutor());
    }
  });
}

TYPED_TEST(RocketNetworkTest, ClientCreationAndReconnectClientOutlivesStream) {
  this->withClient([](RocketTestClient& client) {
    constexpr size_t kNumRequestedPayloads = 1000000;
    constexpr folly::StringPiece kMetadata("metadata");
    const auto data =
        folly::to<std::string>("generate:", kNumRequestedPayloads);

    // Open a stream and reconnect many times, having each RocketClient slightly
    // outlive the associated stream.
    for (size_t i = 0; i < 1000; ++i) {
      {
        auto stream =
            client.sendRequestStreamSync(Payload::makeFromMetadataAndData(
                kMetadata, folly::StringPiece{data}));
        EXPECT_TRUE(stream.hasValue());
      }
      client.reconnect();
    }
  });
}
