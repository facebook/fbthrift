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
#include <folly/io/Cursor.h>
#include <thrift/lib/cpp2/Flags.h>
#include <thrift/lib/cpp2/IOBufChainCursor.h>
#include <thrift/lib/cpp2/async/StreamPayload.h>
#include <thrift/lib/cpp2/transport/rocket/RequestPayload.h>
#include <thrift/lib/cpp2/transport/rocket/payload/CustomCompressionPayloadSerializerStrategy.h>
#include <thrift/lib/cpp2/transport/rocket/payload/DefaultPayloadSerializerStrategy.h>
#include <thrift/lib/cpp2/transport/rocket/payload/PayloadSerializer.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache::thrift::rocket {

namespace {
std::string chainString(const IOBufChain& chain) {
  io::IOBufChainCursor cursor(chain);
  return cursor.readFixedString(chain.chainLength());
}

Payload movePayloadToIOBufChain(Payload payload) {
  const auto metadataSize = payload.metadataSize();
  return Payload::makeCombined(
      IOBufChain(std::move(payload).buffer()), metadataSize);
}

Payload movePayloadToFragmentedIOBufChain(Payload payload) {
  const auto metadataSize = payload.metadataSize();
  const auto buffer = std::move(payload).buffer();
  const auto bytes = buffer->coalesce();
  IOBufChain chain;
  for (size_t offset = 0; offset < bytes.size(); ++offset) {
    chain.append(folly::IOBuf::copyBuffer(bytes.data() + offset, 1));
  }
  return Payload::makeCombined(std::move(chain), metadataSize);
}
} // namespace

void testPackAndUnpackWithCompactProtocol(PayloadSerializer& serializer) {
  RequestRpcMetadata metadata;
  metadata.protocol() = ProtocolId::COMPACT;
  auto payload = serializer.packCompact(metadata);
  EXPECT_GT(payload->computeChainDataLength(), 1);

  RequestRpcMetadata other;
  serializer.unpack<RequestRpcMetadata>(other, payload.get(), false);
  EXPECT_EQ(other, metadata);
  EXPECT_EQ(other.protocol(), ProtocolId::COMPACT);
}

TEST(PayloadSerializerTest, TestPackWithDefaultStrategy) {
  PayloadSerializer::reset();
  PayloadSerializer::initialize(DefaultPayloadSerializerStrategy());
  auto& serializer = *PayloadSerializer::getInstance().get();
  testPackAndUnpackWithCompactProtocol(serializer);
}

TEST(PayloadSerializerTest, TestPackWithoutChecksumUsingFacade) {
  PayloadSerializer::reset();
  PayloadSerializer::initialize(
      ChecksumPayloadSerializerStrategy<DefaultPayloadSerializerStrategy>());
  RequestRpcMetadata metadata;
  metadata.protocol() = ProtocolId::COMPACT;
  auto payload = PayloadSerializer::getInstance()->packWithFds(
      &metadata,
      folly::IOBuf::copyBuffer("test"),
      folly::SocketFds(),
      false, /* encodeMetadataUsingBinary */
      nullptr);

  auto other = PayloadSerializer::getInstance()->unpack<RequestPayload>(
      std::move(payload), false);
  EXPECT_EQ(other.hasException(), false);
  EXPECT_NE(other->payload, nullptr);
  EXPECT_FALSE(other->payloadChain);
}

TEST(PayloadSerializerTest, ChainPayloadRemainsAChainAfterMetadataUnpack) {
  PayloadSerializer::reset();
  PayloadSerializer::initialize(DefaultPayloadSerializerStrategy());
  auto& serializer = *PayloadSerializer::getInstance();

  RequestRpcMetadata metadata;
  metadata.protocol() = ProtocolId::BINARY;
  auto serializedMetadata = serializer.packCompact(metadata);
  const auto metadataSize = serializedMetadata->computeChainDataLength();
  IOBufChain combined;
  combined.append(std::move(serializedMetadata));
  combined.append(folly::IOBuf::copyBuffer("request-data"));

  auto result = serializer.unpack<RequestPayload>(
      Payload::makeCombined(std::move(combined), metadataSize), false);

  ASSERT_FALSE(result.hasException());
  EXPECT_EQ(nullptr, result->payload);
  ASSERT_TRUE(result->payloadChain);
  EXPECT_EQ("request-data", chainString(*result->payloadChain));
}

void testFragmentedChainMetadata(bool encodeMetadataUsingBinary) {
  PayloadSerializer::reset();
  PayloadSerializer::initialize(DefaultPayloadSerializerStrategy());
  auto& serializer = *PayloadSerializer::getInstance();

  RequestRpcMetadata metadata;
  metadata.protocol() = ProtocolId::BINARY;
  auto payload = serializer.packWithFds(
      &metadata,
      folly::IOBuf::copyBuffer("request-data"),
      folly::SocketFds(),
      encodeMetadataUsingBinary,
      nullptr);

  auto result = serializer.unpack<RequestPayload>(
      movePayloadToFragmentedIOBufChain(std::move(payload)),
      encodeMetadataUsingBinary);

  ASSERT_FALSE(result.hasException());
  EXPECT_EQ(ProtocolId::BINARY, result->metadata.protocol());
  ASSERT_TRUE(result->payloadChain);
  EXPECT_EQ("request-data", chainString(*result->payloadChain));
}

TEST(PayloadSerializerTest, BinaryMetadataSpansIOBufChainElements) {
  testFragmentedChainMetadata(true);
}

TEST(PayloadSerializerTest, CompactMetadataSpansIOBufChainElements) {
  testFragmentedChainMetadata(false);
}

TEST(PayloadSerializerTest, ChainPayloadSupportsCompression) {
  PayloadSerializer::reset();
  PayloadSerializer::initialize(DefaultPayloadSerializerStrategy());
  auto& serializer = *PayloadSerializer::getInstance();

  RequestRpcMetadata metadata;
  metadata.protocol() = ProtocolId::BINARY;
  metadata.compression() = CompressionAlgorithm::ZSTD;
  auto payload = serializer.packWithFds(
      &metadata,
      folly::IOBuf::copyBuffer("request-data"),
      folly::SocketFds(),
      false,
      nullptr);

  auto result = serializer.unpack<RequestPayload>(
      movePayloadToIOBufChain(std::move(payload)), false);

  ASSERT_FALSE(result.hasException());
  ASSERT_TRUE(result->payloadChain);
  EXPECT_EQ("request-data", chainString(*result->payloadChain));
}

TEST(PayloadSerializerTest, ChainPayloadRejectsStreamPayload) {
  PayloadSerializer::reset();
  PayloadSerializer::initialize(DefaultPayloadSerializerStrategy());
  auto& serializer = *PayloadSerializer::getInstance();

  IOBufChain data;
  data.append(folly::IOBuf::copyBuffer("stream-data"));
  auto result = serializer.unpack<StreamPayload>(
      Payload::makeCombined(std::move(data), 0), false);

  EXPECT_TRUE(result.hasException<TApplicationException>());
}

TEST(PayloadSerializerTest, TestPtrCoOwnership) {
  std::unique_ptr<PayloadSerializer::Ptr> ptr = nullptr;

  {
    PayloadSerializer::initialize(
        ChecksumPayloadSerializerStrategy<DefaultPayloadSerializerStrategy>());
    ptr = std::make_unique<PayloadSerializer::Ptr>(
        PayloadSerializer::getInstance());
    testPackAndUnpackWithCompactProtocol(**ptr);
  }

  PayloadSerializer::initialize(
      ChecksumPayloadSerializerStrategy<DefaultPayloadSerializerStrategy>());

  // **ptr is still valid here, despite the re-initialization
  testPackAndUnpackWithCompactProtocol(**ptr);
}

TEST(PayloadSerializerTest, TestMakeAndNonOwningPtr) {
  std::unique_ptr<PayloadSerializer::Ptr> ptr = nullptr;

  {
    auto ps = PayloadSerializer::make();
    ptr = std::make_unique<PayloadSerializer::Ptr>(ps.getNonOwningPtr());
    // valid here while ps is in scope
    testPackAndUnpackWithCompactProtocol(**ptr);
  }

  // ptr does not own, so it is not valid here
  // testPackAndUnpackWithCompactProtocol(**ptr);
}

struct MyCustomCompressor : public CustomCompressor {
  std::unique_ptr<folly::IOBuf> compressBuffer(
      std::unique_ptr<folly::IOBuf>&& buffer) override {
    return folly::compression::getCodec(folly::compression::CodecType::ZSTD)
        ->compress(buffer.get());
  }

  std::unique_ptr<folly::IOBuf> uncompressBuffer(
      std::unique_ptr<folly::IOBuf>&& buffer) override {
    return folly::compression::getCodec(folly::compression::CodecType::ZSTD)
        ->uncompress(buffer.get());
  }

  IOBufChain uncompressBuffer(IOBufChain&& buffer) override {
    return CompressionManager().uncompressBuffer(
        std::move(buffer), CompressionAlgorithm::ZSTD);
  }
};

TEST(PayloadSerializerTest, TestMakeCustomCompression) {
  CustomCompressionPayloadSerializerStrategyOptions options;
  options.compressor = std::make_shared<MyCustomCompressor>();

  auto ps = PayloadSerializer::make<CustomCompressionPayloadSerializerStrategy<
      DefaultPayloadSerializerStrategy>>(options);
  testPackAndUnpackWithCompactProtocol(ps);
}

TEST(PayloadSerializerTest, ChainPayloadSupportsCustomCompression) {
  CustomCompressionPayloadSerializerStrategyOptions options;
  options.compressor = std::make_shared<MyCustomCompressor>();
  CustomCompressionPayloadSerializerStrategy<DefaultPayloadSerializerStrategy>
      strategy(options);

  RequestRpcMetadata metadata;
  metadata.protocol() = ProtocolId::BINARY;
  metadata.compression() = CompressionAlgorithm::CUSTOM;
  auto payload = strategy.packWithFds(
      &metadata,
      folly::IOBuf::copyBuffer("request-data"),
      folly::SocketFds(),
      false,
      nullptr);

  auto result = strategy.unpack<RequestPayload>(
      movePayloadToIOBufChain(std::move(payload)), false);

  ASSERT_FALSE(result.hasException());
  ASSERT_TRUE(result->payloadChain);
  EXPECT_EQ("request-data", chainString(*result->payloadChain));
}

TEST(PayloadSerializerTest, TestCompressionAndUncompression) {
  if (!folly::kIsLinux) {
    // on non-linux platforms
    return;
  }

  std::vector<std::pair<
      std::unique_ptr<PayloadSerializer>,
      bool /*supports custom compression*/>>
      payloadSerializers;
  payloadSerializers.emplace_back(
      std::make_unique<PayloadSerializer>(DefaultPayloadSerializerStrategy()),
      false);
  payloadSerializers.emplace_back(
      std::make_unique<PayloadSerializer>(ChecksumPayloadSerializerStrategy<
                                          DefaultPayloadSerializerStrategy>()),
      false);

  CustomCompressionPayloadSerializerStrategyOptions options;
  options.compressor = std::make_shared<MyCustomCompressor>();
  payloadSerializers.emplace_back(
      std::make_unique<PayloadSerializer>(
          CustomCompressionPayloadSerializerStrategy<
              DefaultPayloadSerializerStrategy>(options)),
      true);

  std::vector<CompressionAlgorithm> compressionAlgorithms;
  if (folly::kIsApple) {
    compressionAlgorithms = {
        CompressionAlgorithm::NONE,
        CompressionAlgorithm::ZSTD,
        CompressionAlgorithm::ZLIB,
        CompressionAlgorithm::CUSTOM,
    };
  } else {
    compressionAlgorithms = {
        CompressionAlgorithm::NONE,
        CompressionAlgorithm::ZSTD,
        CompressionAlgorithm::ZLIB,
        CompressionAlgorithm::LZ4,
        CompressionAlgorithm::CUSTOM,
    };
  }

  std::string const expected = "hello world";

  for (auto& [ps, supportsCustomCompression] : payloadSerializers) {
    for (const auto& compressionAlgorithm : compressionAlgorithms) {
      auto compressedBuf = ps->compressBuffer(
          folly::IOBuf::fromString(expected), compressionAlgorithm);

      bool compressionIsTrivial = false;
      if (compressionAlgorithm == CompressionAlgorithm::NONE) {
        compressionIsTrivial = true;
      } else if (
          compressionAlgorithm == CompressionAlgorithm::CUSTOM &&
          !supportsCustomCompression) {
        compressionIsTrivial = true;
      }

      if (compressionIsTrivial) {
        EXPECT_EQ(compressedBuf->toString(), expected);
      } else {
        EXPECT_NE(compressedBuf->toString(), expected);
      }

      const auto actual =
          ps->uncompressBuffer(std::move(compressedBuf), compressionAlgorithm);
      EXPECT_EQ(actual->toString(), expected);

      auto compressedChain = ps->compressBuffer(
          folly::IOBuf::fromString(expected), compressionAlgorithm);
      if (compressionAlgorithm == CompressionAlgorithm::LZ4) {
        EXPECT_THROW(
            ps->uncompressBuffer(
                IOBufChain(std::move(compressedChain)), compressionAlgorithm),
            TApplicationException);
        continue;
      }
      auto actualChain = ps->uncompressBuffer(
          IOBufChain(std::move(compressedChain)), compressionAlgorithm);
      EXPECT_EQ(actualChain, IOBufChain(folly::IOBuf::fromString(expected)));
    }
  }
}

} // namespace apache::thrift::rocket
