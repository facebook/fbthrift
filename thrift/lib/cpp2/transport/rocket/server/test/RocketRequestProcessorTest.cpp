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

#include <chrono>
#include <limits>
#include <optional>

#include <folly/io/async/EventBase.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/RequestsRegistry.h>
#include <thrift/lib/cpp2/transport/core/testutil/ServerConfigsMock.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketThriftRequests.h>
#include <thrift/lib/cpp2/transport/rocket/server/detail/RocketErrorHandler.h>
#include <thrift/lib/cpp2/transport/rocket/server/detail/RocketRequestOrchestrator.h>
#include <thrift/lib/cpp2/transport/rocket/server/detail/RocketRequestProcessor.h>
#include <thrift/lib/cpp2/transport/rocket/server/test/MockIRocketServerConnection.h>

namespace apache::thrift::rocket {
namespace {

using test::MockIRocketServerConnection;
using ::testing::NiceMock;

RequestRpcMetadata compressedMetadata() {
  RequestRpcMetadata metadata;
  metadata.crc32c() = 0;
  metadata.compression() = CompressionAlgorithm::ZSTD;
  return metadata;
}

TEST(RocketRequestProcessorTest, RejectsChainBackedStreamAndSinkAsBadMetadata) {
  for (const auto kind :
       {RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE, RpcKind::SINK}) {
    SCOPED_TRACE(static_cast<int>(kind));

    folly::EventBase eventBase;
    server::ServerConfigsMock serverConfigs;
    auto worker = Cpp2Worker::createDummy(&eventBase);
    RequestsRegistry requestsRegistry{
        std::numeric_limits<uint64_t>::max(),
        std::numeric_limits<uint64_t>::max(),
        0};
    Cpp2ConnContext connContext;
    folly::once_flag setupLoggingFlag;
    RocketErrorHandler errorHandler(
        serverConfigs.getObserver(), &requestsRegistry);
    NiceMock<MockIRocketServerConnection> connection;
    auto serializer =
        PayloadSerializer::make<DefaultPayloadSerializerStrategy>();
    ON_CALL(connection, getPayloadSerializer()).WillByDefault([&] {
      return serializer.getNonOwningPtr();
    });

    std::optional<ResponseRpcError> responseError;
    EXPECT_CALL(connection, sendError(StreamId{1}, testing::_, testing::_))
        .WillOnce([&](StreamId, RocketException&& error, auto) {
          EXPECT_EQ(ErrorCode::INVALID, error.getErrorCode());
          ResponseRpcError decodedError;
          CompactSerializer::deserialize(
              error.moveErrorData().get(), decodedError);
          responseError = std::move(decodedError);
        });

    RocketRequestOrchestrator orchestrator(
        nullptr,
        connContext,
        setupLoggingFlag,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &serverConfigs,
        &requestsRegistry,
        &errorHandler,
        nullptr,
        worker.get());

    RequestRpcMetadata metadata;
    metadata.protocol() = ProtocolId::COMPACT;
    metadata.name() = "method";
    metadata.kind() = kind;
    auto payload = serializer.packWithFds(
        &metadata,
        folly::IOBuf::copyBuffer("payload"),
        folly::SocketFds{},
        false,
        nullptr);
    const auto metadataSize = payload.metadataSize();
    auto chainPayload = Payload::makeCombined(
        IOBufChain(std::move(payload).buffer()), metadataSize);

    auto makeRequest = [&](RequestRpcMetadata&& requestMetadata,
                           Payload&& debugPayload,
                           std::shared_ptr<folly::RequestContext>&& context) {
      return RequestsRegistry::makeRequest<ThriftServerRequestResponse>(
          eventBase,
          serverConfigs,
          std::move(requestMetadata),
          connContext,
          std::move(context),
          requestsRegistry,
          std::move(debugPayload),
          RocketServerFrameContext(connection, StreamId{1}),
          10,
          std::chrono::milliseconds{0});
    };

    orchestrator.handleRequestCommon(
        std::move(chainPayload),
        std::move(makeRequest),
        kind,
        connection,
        [](const transport::THeader&) {
          return server::TServerObserver::SamplingStatus{};
        },
        std::nullopt);

    ASSERT_TRUE(responseError);
    EXPECT_EQ(
        ResponseRpcErrorCode::REQUEST_PARSING_FAILURE, *responseError->code());
    EXPECT_EQ("Invalid metadata object", *responseError->what_utf8());
  }
}

TEST(RocketRequestProcessorTest, DecompressesIOBufChain) {
  auto compressed = PayloadSerializer::getInstance()->compressBuffer(
      folly::IOBuf::copyBuffer("payload"), CompressionAlgorithm::ZSTD);
  IOBufChain data(std::move(compressed));
  NiceMock<MockIRocketServerConnection> connection;
  RocketRequestProcessor processor(nullptr);

  EXPECT_TRUE(
      processor
          .processPayloadCompression(data, compressedMetadata(), connection)
          .empty());
  EXPECT_EQ(IOBufChain(folly::IOBuf::copyBuffer("payload")), data);
}

TEST(RocketRequestProcessorTest, ReportsIOBufChainDecompressionFailure) {
  IOBufChain data(folly::IOBuf::copyBuffer("not compressed data"));
  NiceMock<MockIRocketServerConnection> connection;
  RocketRequestProcessor processor(nullptr);

  EXPECT_FALSE(
      processor
          .processPayloadCompression(data, compressedMetadata(), connection)
          .empty());
}

} // namespace
} // namespace apache::thrift::rocket
