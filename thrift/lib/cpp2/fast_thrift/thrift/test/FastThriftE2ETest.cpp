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

#include <folly/Synchronized.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp2/Flags.h>
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
#include <thrift/lib/cpp2/fast_thrift/rocket/server/connection/ConnectionHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/connection/ConnectionManager.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerFrameCodecHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerRequestResponseFrameHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerSetupFrameHandler.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerStreamStateHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/ThriftClientChannel.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/handler/ThriftClientMetadataPushHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/handler/ThriftClientRocketInterfaceHandler.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/test/if/gen-cpp2/BackwardsCompatibilityTestService.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/test/if/gen-cpp2/BackwardsCompatibilityTestServiceAsyncClient.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/ThriftServerChannel.h>
#include <thrift/lib/cpp2/fast_thrift/transport/TransportHandler.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_constants.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

THRIFT_FLAG_DECLARE_bool(rocket_client_binary_rpc_metadata_encoding);

namespace apache::thrift::fast_thrift::test {

using apache::thrift::fast_thrift::channel_pipeline::PipelineBuilder;
using apache::thrift::fast_thrift::channel_pipeline::PipelineImpl;
using apache::thrift::fast_thrift::channel_pipeline::SimpleBufferAllocator;

// Client handler tags
HANDLER_TAG(client_frame_length_parser_handler);
HANDLER_TAG(client_frame_length_encoder_handler);
HANDLER_TAG(rocket_client_frame_codec_handler);
HANDLER_TAG(rocket_client_setup_handler);
HANDLER_TAG(rocket_client_request_response_frame_handler);
HANDLER_TAG(rocket_client_error_frame_handler);
HANDLER_TAG(rocket_client_stream_state_handler);
HANDLER_TAG(thrift_client_rocket_interface_handler);
HANDLER_TAG(thrift_client_metadata_push_handler);

// Server handler tags
HANDLER_TAG(server_frame_length_parser_handler);
HANDLER_TAG(server_frame_length_encoder_handler);
HANDLER_TAG(rocket_server_frame_codec_handler);
HANDLER_TAG(rocket_server_setup_frame_handler);
HANDLER_TAG(rocket_server_request_response_frame_handler);
HANDLER_TAG(rocket_server_stream_state_handler);

/**
 * ConnectCallback - Triggers transportHandler->onConnect() when the
 * TCP connection is established.
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

  void connectErr(const folly::AsyncSocketException&) noexcept override {
    connected_ = false;
    baton_.post();
  }

 private:
  apache::thrift::fast_thrift::transport::TransportHandler* transportHandler_;
  folly::Baton<>& baton_;
  bool& connected_;
};

/**
 * TestHandler - Implements BackwardsCompatibilityTestServiceSvIf.
 */
class TestHandler
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
 * FastThriftE2ETest - End-to-end integration test with both a fast_thrift
 * client and a fast_thrift server.
 *
 * Server pipeline:
 *   TransportHandler -> FrameLengthParserHandler -> FrameLengthEncoderHandler
 *   -> RocketServerFrameCodecHandler -> RocketServerSetupFrameHandler
 *   -> RocketServerRequestResponseFrameHandler ->
 * RocketServerStreamStateHandler
 *   -> ThriftServerChannel
 *
 * Client pipeline:
 *   TransportHandler -> FrameLengthParserHandler -> FrameLengthEncoderHandler
 *   -> RocketClientFrameCodecHandler -> RocketClientSetupFrameHandler
 *   -> RocketClientRequestResponseFrameHandler -> RocketClientErrorFrameHandler
 *   -> RocketClientStreamStateHandler -> ThriftClientRocketInterfaceHandler
 *   -> ThriftClientMetadataHandler -> ThriftClientMetadataPushHandler
 *   -> ThriftClientRequestResponseHandler -> ThriftClientChannel
 */
class FastThriftE2ETest : public ::testing::Test {
 protected:
  void SetUp() override {
    THRIFT_FLAG_SET_MOCK(rocket_client_binary_rpc_metadata_encoding, true);

    handler_ = std::make_shared<TestHandler>();
    executor_ = std::make_shared<folly::IOThreadPoolExecutor>(1);

    // Server pipeline factory — called for each accepted connection
    apache::thrift::fast_thrift::rocket::server::connection::PipelineFactory<
        apache::thrift::fast_thrift::transport::TransportHandler>
        pipelineFactory =
            [this](
                folly::EventBase* evb,
                apache::thrift::fast_thrift::transport::TransportHandler*
                    transportHandler) -> PipelineImpl::Ptr {
      auto serverChannel =
          std::make_shared<thrift::ThriftServerChannel>(handler_);

      auto pipeline =
          PipelineBuilder<
              apache::thrift::fast_thrift::transport::TransportHandler,
              thrift::ThriftServerChannel,
              SimpleBufferAllocator>()
              .setEventBase(evb)
              .setHead(transportHandler)
              .setTail(serverChannel.get())
              .setAllocator(&serverAllocator_)
              .setHeadToTailOp(
                  apache::thrift::fast_thrift::channel_pipeline::HeadToTailOp::
                      Read)
              .addNextDuplex<apache::thrift::fast_thrift::rocket::server::
                                 handler::RocketServerStreamStateHandler>(
                  rocket_server_stream_state_handler_tag)
              .addNextDuplex<
                  apache::thrift::fast_thrift::rocket::server::handler::
                      RocketServerRequestResponseFrameHandler>(
                  rocket_server_request_response_frame_handler_tag)
              .addNextDuplex<apache::thrift::fast_thrift::rocket::server::
                                 handler::RocketServerSetupFrameHandler>(
                  rocket_server_setup_frame_handler_tag)
              .addNextDuplex<apache::thrift::fast_thrift::rocket::server::
                                 handler::RocketServerFrameCodecHandler>(
                  rocket_server_frame_codec_handler_tag)
              .addNextOutbound<apache::thrift::fast_thrift::frame::write::
                                   handler::FrameLengthEncoderHandler>(
                  server_frame_length_encoder_handler_tag)
              .addNextInbound<apache::thrift::fast_thrift::frame::read::
                                  handler::FrameLengthParserHandler>(
                  server_frame_length_parser_handler_tag)
              .build();

      serverChannel->setPipelineRef(*pipeline);
      serverChannel->setWorker(apache::thrift::Cpp2Worker::createDummy(evb));

      serverChannels_.withWLock([&](auto& channels) {
        channels.push_back(std::move(serverChannel));
      });

      return pipeline;
    };

    connectionManager_ = apache::thrift::fast_thrift::rocket::server::
        connection::ConnectionManager<
            apache::thrift::fast_thrift::transport::TransportHandler>::
            create(
                folly::SocketAddress("::1", 0),
                folly::getKeepAliveToken(executor_.get()),
                std::move(pipelineFactory));
    connectionManager_->start();

    clientThread_ = std::make_unique<folly::ScopedEventBaseThread>();
  }
  void TearDown() override {
    clientThread_->getEventBase()->runInEventBaseThreadAndWait([&] {
      clientPipeline_.reset();
      clientTransportHandler_.reset();
    });
    clientThread_.reset();
    serverChannels_.withWLock([](auto& channels) { channels.clear(); });
    connectionManager_->stop();
    connectionManager_.reset();
    executor_->join();
    executor_.reset();
  }

  /**
   * Create a fast_thrift client connected to the fast_thrift server.
   */
  std::unique_ptr<apache::thrift::Client<BackwardsCompatibilityTestService>>
  createClient() {
    auto* evb = clientThread_->getEventBase();
    thrift::ThriftClientChannel::UniquePtr channel;
    folly::Baton<> connectBaton;
    bool connected = false;

    evb->runInEventBaseThreadAndWait([&] {
      auto socket = folly::AsyncSocket::newSocket(evb);
      auto* socketPtr = socket.get();

      clientTransportHandler_ =
          apache::thrift::fast_thrift::transport::TransportHandler::create(
              std::move(socket));
      channel = thrift::ThriftClientChannel::newChannel(evb);

      connectCallback_ = std::make_unique<ConnectCallback>(
          clientTransportHandler_.get(), connectBaton, connected);
      socketPtr->connect(
          connectCallback_.get(), connectionManager_->getAddress(), 30000);

      auto setupFactory = []() {
        apache::thrift::RequestSetupMetadata meta;
        meta.minVersion() = 8;
        meta.maxVersion() = 10;
        meta.clientMetadata().ensure().agent() = "fast_thrift_e2e_test";

        apache::thrift::BinaryProtocolWriter writer;
        folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
        writer.setOutput(&queue);
        meta.write(&writer);

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
              .setAllocator(&clientAllocator_)
              .addNextInbound<apache::thrift::fast_thrift::frame::read::
                                  handler::FrameLengthParserHandler>(
                  client_frame_length_parser_handler_tag)
              .addNextOutbound<apache::thrift::fast_thrift::frame::write::
                                   handler::FrameLengthEncoderHandler>(
                  client_frame_length_encoder_handler_tag)
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

    connectBaton.wait();

    if (!connected) {
      throw std::runtime_error("Failed to connect to server");
    }

    return std::make_unique<
        apache::thrift::Client<BackwardsCompatibilityTestService>>(
        std::move(channel));
  }

  void destroyClientOnEvb(
      std::unique_ptr<
          apache::thrift::Client<BackwardsCompatibilityTestService>>& client) {
    clientThread_->getEventBase()->runInEventBaseThreadAndWait(
        [&] { client.reset(); });
  }

  std::shared_ptr<TestHandler> handler_;
  std::shared_ptr<folly::IOThreadPoolExecutor> executor_;
  apache::thrift::fast_thrift::rocket::server::connection::ConnectionManager<
      apache::thrift::fast_thrift::transport::TransportHandler>::Ptr
      connectionManager_;
  std::unique_ptr<folly::ScopedEventBaseThread> clientThread_;
  SimpleBufferAllocator clientAllocator_;
  SimpleBufferAllocator serverAllocator_;
  apache::thrift::fast_thrift::transport::TransportHandler::Ptr
      clientTransportHandler_;
  PipelineImpl::Ptr clientPipeline_;
  folly::Synchronized<std::vector<std::shared_ptr<thrift::ThriftServerChannel>>>
      serverChannels_;
  std::unique_ptr<ConnectCallback> connectCallback_;
};

// =============================================================================
// Test Cases
// =============================================================================

TEST_F(FastThriftE2ETest, Ping) {
  auto client = createClient();

  folly::Baton<> baton;
  bool success = false;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_ping()
        .via(clientThread_->getEventBase())
        .thenValue([&](auto&&) {
          success = true;
          baton.post();
        })
        .thenError([&](const folly::exception_wrapper& ew) {
          LOG(ERROR) << "ping failed: " << folly::exceptionStr(ew);
          baton.post();
        });
  });

  baton.wait();
  EXPECT_TRUE(success);

  destroyClientOnEvb(client);
}

TEST_F(FastThriftE2ETest, EchoRequestResponse) {
  auto client = createClient();

  folly::Baton<> baton;
  std::string result;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_echo("hello world")
        .via(clientThread_->getEventBase())
        .thenValue([&](std::string response) {
          result = std::move(response);
          baton.post();
        })
        .thenError([&](const folly::exception_wrapper& ew) {
          LOG(ERROR) << "echo failed: " << folly::exceptionStr(ew);
          baton.post();
        });
  });

  baton.wait();
  EXPECT_EQ(result, "hello world");

  destroyClientOnEvb(client);
}

TEST_F(FastThriftE2ETest, SequentialRequests) {
  auto client = createClient();

  // First request — completes before the second is sent
  folly::Baton<> baton1;
  std::string result1;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_echo("first")
        .via(clientThread_->getEventBase())
        .thenValue([&](std::string response) {
          result1 = std::move(response);
          baton1.post();
        })
        .thenError([&](folly::exception_wrapper) { baton1.post(); });
  });

  baton1.wait();
  EXPECT_EQ(result1, "first");

  // Second request — sent after the first completes
  folly::Baton<> baton2;
  std::string result2;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_echo("second")
        .via(clientThread_->getEventBase())
        .thenValue([&](std::string response) {
          result2 = std::move(response);
          baton2.post();
        })
        .thenError([&](folly::exception_wrapper) { baton2.post(); });
  });

  baton2.wait();
  EXPECT_EQ(result2, "second");

  // Third request — validates stream IDs continue advancing
  folly::Baton<> baton3;
  std::string result3;

  clientThread_->getEventBase()->runInEventBaseThread([&] {
    client->semifuture_echo("third")
        .via(clientThread_->getEventBase())
        .thenValue([&](std::string response) {
          result3 = std::move(response);
          baton3.post();
        })
        .thenError([&](folly::exception_wrapper) { baton3.post(); });
  });

  baton3.wait();
  EXPECT_EQ(result3, "third");

  destroyClientOnEvb(client);
}

TEST_F(FastThriftE2ETest, MultipleRequests) {
  auto client = createClient();

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

  destroyClientOnEvb(client);
}

TEST_F(FastThriftE2ETest, LargeResponse) {
  auto client = createClient();

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
  EXPECT_EQ(result, std::string(kResponseSize, 'x'));

  destroyClientOnEvb(client);
}

} // namespace apache::thrift::fast_thrift::test
