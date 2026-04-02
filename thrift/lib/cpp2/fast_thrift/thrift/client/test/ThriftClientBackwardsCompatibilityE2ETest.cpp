/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/BufferAllocator.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineImpl.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/handler/FrameLengthParserHandler.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/handler/FrameLengthEncoderHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/client/handler/RocketClientErrorFrameHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/client/handler/RocketClientFrameCodecHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/client/handler/RocketClientRequestResponseFrameHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/client/handler/RocketClientSetupFrameHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/client/handler/RocketClientStreamStateHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/ThriftClientChannel.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/handler/ThriftClientMetadataPushHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/handler/ThriftClientRocketInterfaceHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/test/if/gen-cpp2/BackwardsCompatibilityTestService.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/test/if/gen-cpp2/BackwardsCompatibilityTestServiceAsyncClient.h>
#include <thrift/lib/cpp2/fast_thrift/transport/TransportHandler.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_constants.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache::thrift::fast_thrift::test {

using apache::thrift::fast_thrift::channel_pipeline::PipelineBuilder;
using apache::thrift::fast_thrift::channel_pipeline::PipelineImpl;
using apache::thrift::fast_thrift::channel_pipeline::SimpleBufferAllocator;

// Handler tags for pipeline construction
HANDLER_TAG(frame_length_parser_handler);
HANDLER_TAG(frame_length_encoder_handler);
HANDLER_TAG(rocket_client_frame_codec_handler);
HANDLER_TAG(rocket_client_setup_handler);
HANDLER_TAG(rocket_client_request_response_frame_handler);
HANDLER_TAG(rocket_client_error_frame_handler);
HANDLER_TAG(rocket_client_stream_state_handler);
HANDLER_TAG(thrift_client_rocket_interface_handler);
HANDLER_TAG(thrift_client_metadata_push_handler);

/**
 * ConnectCallback - Callback for socket connection that triggers
 * transportHandler->onConnect() when connection is established.
 */
class ConnectCallback : public folly::AsyncSocket::ConnectCallback {
 public:
  explicit ConnectCallback(
      apache::thrift::fast_thrift::transport::TransportHandler*
          transportHandler,
      folly::Baton<>& baton,
      bool& connected)
      : transportHandler_(transportHandler),
        baton_(baton),
        connected_(connected) {}

  void connectSuccess() noexcept override {
    connected_ = true;
    transportHandler_->onConnect();
    baton_.post();
  }

  void connectErr(const folly::AsyncSocketException& /*ex*/) noexcept override {
    connected_ = false;
    baton_.post();
  }

 private:
  apache::thrift::fast_thrift::transport::TransportHandler* transportHandler_;
  folly::Baton<>& baton_;
  bool& connected_;
};

/**
 * BackwardsCompatibilityTestHandler - Implementation of the generated
 * BackwardsCompatibilityTestServiceSvIf interface.
 *
 * This handler implements the service methods defined in
 * BackwardsCompatibilityTestService.thrift.
 */
class BackwardsCompatibilityTestHandler
    : public apache::thrift::ServiceHandler<BackwardsCompatibilityTestService> {
 public:
  void echo(
      std::string& response, std::unique_ptr<std::string> message) override {
    response = *message;
  }

  int64_t add(int64_t a, int64_t b) override { return a + b; }

  void sendResponse(std::string& response, int64_t size) override {
    response = std::string(static_cast<size_t>(size), 'x');
  }

  void ping() override {}
};

/**
 * ThriftClientBackwardsCompatibilityE2ETest - E2E integration test for
 * fast_thrift client.
 *
 * Uses a standard ThriftServer (via ScopedServerInterfaceThread) with
 * a fast_thrift client channel to verify end-to-end communication.
 */
class ThriftClientBackwardsCompatibilityE2ETest : public ::testing::Test {
 protected:
  void SetUp() override {
    handler_ = std::make_shared<BackwardsCompatibilityTestHandler>();
    server_ =
        std::make_unique<apache::thrift::ScopedServerInterfaceThread>(handler_);
    clientThread_ = std::make_unique<folly::ScopedEventBaseThread>();
  }

  void TearDown() override {
    clientThread_->getEventBase()->runInEventBaseThreadAndWait([&] {
      clientPipeline_.reset();
      clientTransportHandler_.reset();
    });
    channel_.reset();
    server_.reset();
    clientThread_.reset();
  }

  /**
   * Create a fast_thrift client channel connected to the test server.
   *
   * The channel is constructed with the full pipeline:
   * - FrameLengthParserHandler: Parses frames from raw bytes
   * - FrameLengthEncoderHandler: Encodes frames with length prefix
   * - RocketClientFrameCodecHandler: Bidirectional codec for Rocket frames
   * - RocketClientSetupFrameHandler: Handles initial SETUP frame
   * - RocketClientRequestResponseFrameHandler: Handles REQUEST_RESPONSE frames
   * - RocketClientStreamStateHandler: Manages stream state
   * - ThriftClientMetadataHandler: Handles Thrift metadata
   */
  thrift::ThriftClientChannel::UniquePtr createFastThriftChannel() {
    auto* evb = clientThread_->getEventBase();
    thrift::ThriftClientChannel::UniquePtr channel;
    folly::Baton<> connectBaton;
    bool connected = false;

    // All socket and channel operations must happen on the EventBase thread
    evb->runInEventBaseThreadAndWait([&] {
      // Create socket
      auto socket = folly::AsyncSocket::newSocket(evb);
      auto* socketPtr = socket.get();

      // Create transport handler and channel separately
      clientTransportHandler_ =
          apache::thrift::fast_thrift::transport::TransportHandler::create(
              std::move(socket));
      channel = thrift::ThriftClientChannel::newChannel(evb);

      // Create connect callback that will trigger onConnect when connection is
      // established
      connectCallback_ = std::make_unique<ConnectCallback>(
          clientTransportHandler_.get(), connectBaton, connected);

      socketPtr->connect(connectCallback_.get(), server_->getAddress(), 30000);

      // Setup metadata factory - creates properly serialized
      // RequestSetupMetadata
      auto setupFactory = []() {
        apache::thrift::RequestSetupMetadata meta;
        // Set version info like RocketClientChannelBase does
        meta.minVersion() = 8;
        meta.maxVersion() = 10;

        // Add client metadata
        auto& clientMetadata = meta.clientMetadata().ensure();
        clientMetadata.agent() = "fast_thrift_test";

        // Serialize using BinaryProtocol
        apache::thrift::BinaryProtocolWriter writer;
        folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
        writer.setOutput(&queue);
        meta.write(&writer);

        // Prepend the rocket protocol key
        folly::IOBufQueue result(folly::IOBufQueue::cacheChainLength());
        const uint32_t protocolKey =
            apache::thrift::RpcMetadata_constants::kRocketProtocolKey();
        folly::io::QueueAppender appender(&result, sizeof(protocolKey));
        appender.writeBE<uint32_t>(protocolKey);
        result.append(queue.move());

        return std::make_pair(result.move(), std::unique_ptr<folly::IOBuf>());
      };

      clientPipeline_ =
          PipelineBuilder<
              thrift::ThriftClientChannel,
              apache::thrift::fast_thrift::transport::TransportHandler,
              SimpleBufferAllocator>()
              .setEventBase(evb)
              .setTail(clientTransportHandler_.get())
              .setHead(channel.get())
              .setAllocator(&allocator_)
              .addNextInbound<apache::thrift::fast_thrift::frame::read::
                                  handler::FrameLengthParserHandler>(
                  frame_length_parser_handler_tag)
              .addNextOutbound<apache::thrift::fast_thrift::frame::write::
                                   handler::FrameLengthEncoderHandler>(
                  frame_length_encoder_handler_tag)
              .addNextDuplex<apache::thrift::fast_thrift::rocket::client::
                                 handler::RocketClientFrameCodecHandler>(
                  rocket_client_frame_codec_handler_tag)
              .addNextDuplex<apache::thrift::fast_thrift::rocket::client::
                                 handler::RocketClientSetupFrameHandler>(
                  rocket_client_setup_handler_tag, std::move(setupFactory))
              .addNextDuplex<
                  apache::thrift::fast_thrift::rocket::client::handler::
                      RocketClientRequestResponseFrameHandler>(
                  rocket_client_request_response_frame_handler_tag)
              .addNextInbound<apache::thrift::fast_thrift::rocket::client::
                                  handler::RocketClientErrorFrameHandler>(
                  rocket_client_error_frame_handler_tag)
              .addNextDuplex<apache::thrift::fast_thrift::rocket::client::
                                 handler::RocketClientStreamStateHandler>(
                  rocket_client_stream_state_handler_tag)
              .addNextDuplex<
                  thrift::client::handler::ThriftClientRocketInterfaceHandler>(
                  thrift_client_rocket_interface_handler_tag)
              .addNextInbound<
                  thrift::client::handler::ThriftClientMetadataPushHandler>(
                  thrift_client_metadata_push_handler_tag)
              .build();

      channel->setPipeline(clientPipeline_.get());
      clientTransportHandler_->setPipeline(*clientPipeline_);
    });

    // Wait for connection to complete
    connectBaton.wait();

    if (!connected) {
      throw std::runtime_error("Failed to connect to server");
    }

    return channel;
  }

  std::shared_ptr<BackwardsCompatibilityTestHandler> handler_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> server_;
  std::unique_ptr<folly::ScopedEventBaseThread> clientThread_;
  thrift::ThriftClientChannel::UniquePtr channel_;
  apache::thrift::fast_thrift::transport::TransportHandler::Ptr
      clientTransportHandler_;
  PipelineImpl::Ptr clientPipeline_;
  SimpleBufferAllocator allocator_;
  std::unique_ptr<ConnectCallback> connectCallback_;
};

TEST_F(ThriftClientBackwardsCompatibilityE2ETest, Ping) {
  channel_ = createFastThriftChannel();

  auto client = std::make_unique<
      apache::thrift::Client<BackwardsCompatibilityTestService>>(
      std::move(channel_));

  folly::Baton<> baton;
  bool success = false;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_ping()
        .via(clientThread_->getEventBase())
        .thenValue([&](auto&&) {
          LOG(INFO) << "ping succeeded";
          success = true;
          baton.post();
        })
        .thenError([&](const folly::exception_wrapper& ew) {
          LOG(INFO) << "exception: " << folly::exceptionStr(ew);
          baton.post();
        });
  });

  baton.wait();
  EXPECT_TRUE(success);

  // Destroy the client in the EventBase thread
  clientThread_->getEventBase()->runInEventBaseThreadAndWait(
      [&] { client.reset(); });
}

TEST_F(ThriftClientBackwardsCompatibilityE2ETest, EchoRequestResponse) {
  channel_ = createFastThriftChannel();

  auto client = std::make_unique<
      apache::thrift::Client<BackwardsCompatibilityTestService>>(
      std::move(channel_));

  folly::Baton<> baton;
  std::string result;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_echo("hello world")
        .via(clientThread_->getEventBase())
        .thenValue([&](std::string response) {
          LOG(INFO) << "response: " << response;
          result = std::move(response);
          baton.post();
        })
        .thenError([&](const folly::exception_wrapper& ew) {
          LOG(INFO) << "exception: " << folly::exceptionStr(ew);
          baton.post();
        });
  });

  baton.wait();
  EXPECT_EQ(result, "hello world");

  // Destroy the client in the EventBase thread
  clientThread_->getEventBase()->runInEventBaseThreadAndWait(
      [&] { client.reset(); });
}

TEST_F(ThriftClientBackwardsCompatibilityE2ETest, AddNumbers) {
  channel_ = createFastThriftChannel();

  auto client = std::make_unique<
      apache::thrift::Client<BackwardsCompatibilityTestService>>(
      std::move(channel_));

  folly::Baton<> baton;
  int64_t result = 0;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_add(17, 25)
        .via(clientThread_->getEventBase())
        .thenValue([&](int64_t sum) {
          result = sum;
          baton.post();
        })
        .thenError([&](folly::exception_wrapper) { baton.post(); });
  });

  baton.wait();
  EXPECT_EQ(result, 42);

  // Destroy the client in the EventBase thread
  clientThread_->getEventBase()->runInEventBaseThreadAndWait(
      [&] { client.reset(); });
}

TEST_F(ThriftClientBackwardsCompatibilityE2ETest, MultipleRequests) {
  channel_ = createFastThriftChannel();

  auto client = std::make_unique<
      apache::thrift::Client<BackwardsCompatibilityTestService>>(
      std::move(channel_));

  folly::Baton<> baton1, baton2, baton3;
  std::string echo1, echo2, echo3;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_echo("first")
        .via(clientThread_->getEventBase())
        .thenValue([&](std::string response) {
          echo1 = std::move(response);
          baton1.post();
        })
        .thenError([&](folly::exception_wrapper) { baton1.post(); });

    client->semifuture_echo("second")
        .via(clientThread_->getEventBase())
        .thenValue([&](std::string response) {
          echo2 = std::move(response);
          baton2.post();
        })
        .thenError([&](folly::exception_wrapper) { baton2.post(); });

    client->semifuture_echo("third")
        .via(clientThread_->getEventBase())
        .thenValue([&](std::string response) {
          echo3 = std::move(response);
          baton3.post();
        })
        .thenError([&](folly::exception_wrapper) { baton3.post(); });
  });

  baton1.wait();
  baton2.wait();
  baton3.wait();

  EXPECT_EQ(echo1, "first");
  EXPECT_EQ(echo2, "second");
  EXPECT_EQ(echo3, "third");

  // Destroy the client in the EventBase thread
  clientThread_->getEventBase()->runInEventBaseThreadAndWait(
      [&] { client.reset(); });
}

TEST_F(ThriftClientBackwardsCompatibilityE2ETest, LargeResponse) {
  channel_ = createFastThriftChannel();

  auto client = std::make_unique<
      apache::thrift::Client<BackwardsCompatibilityTestService>>(
      std::move(channel_));

  constexpr int64_t kResponseSize = 10000;

  folly::Baton<> baton;
  std::string result;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_sendResponse(kResponseSize)
        .via(clientThread_->getEventBase())
        .thenValue([&](std::string response) {
          result = std::move(response);
          baton.post();
        })
        .thenError([&](folly::exception_wrapper) { baton.post(); });
  });

  baton.wait();
  EXPECT_EQ(result.size(), kResponseSize);
  EXPECT_EQ(result, std::string(kResponseSize, 'x'));

  // Destroy the client in the EventBase thread
  clientThread_->getEventBase()->runInEventBaseThreadAndWait(
      [&] { client.reset(); });
}

} // namespace apache::thrift::fast_thrift::test
