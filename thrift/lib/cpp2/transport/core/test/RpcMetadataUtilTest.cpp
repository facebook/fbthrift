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

#include <thrift/lib/cpp2/transport/core/RpcMetadataUtil.h>

#include <gtest/gtest.h>
#include <folly/ScopeGuard.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp2/Flags.h>
#include <thrift/lib/cpp2/PluggableFunction.h>
#include <thrift/lib/cpp2/async/RpcOptions.h>
#include <thrift/lib/cpp2/transport/core/RpcMetadataPlugins.h>
#include <thrift/lib/cpp2/transport/rocket/compression/CompressionAlgorithmSelector.h>

using namespace apache::thrift;
using namespace apache::thrift::detail;

THRIFT_FLAG_DECLARE_bool(thrift_client_custom_compression_fallback_to_zstd);
THRIFT_FLAG_DECLARE_bool(thrift_client_lz4_downgrade_when_unsupported);

namespace apache::thrift::detail {
THRIFT_PLUGGABLE_FUNC_SET_TEST(
    std::unique_ptr<folly::IOBuf>,
    makeFrameworkMetadata,
    const RpcOptions& rpcOptions,
    folly::dynamic&) {
  EXPECT_EQ(rpcOptions.getShardId(), "123");
  return folly::IOBuf::copyBuffer(std::string("linked"));
}
} // namespace apache::thrift::detail

TEST(RpcMetadataUtil, frameworkMetadata) {
  RpcOptions rpcOptions;
  auto kind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
  std::string methodName = "foo";
  std::string serviceName = "bar";
  std::chrono::milliseconds timeout(100);
  std::variant<InteractionCreate, int64_t, std::monostate> interactionHandle =
      std::monostate{};
  transport::THeader header;
  header.setProtocolId(protocol::T_COMPACT_PROTOCOL);

  rpcOptions.setShardId("123");
  MethodMetadata methodMetadata(
      MethodMetadata::Data(
          methodName, FunctionQualifier::Unspecified, serviceName));
  auto requestRpcMetadata = makeRequestRpcMetadata(
      rpcOptions,
      kind,
      std::move(methodMetadata),
      timeout,
      interactionHandle,
      /*serverZstdSupported=*/false,
      /*serverLz4Supported=*/false,
      3000,
      header,
      nullptr,
      /*customCompressionEnabled=*/false);
  const auto& buf = **requestRpcMetadata.frameworkMetadata();
  std::string content(reinterpret_cast<const char*>(buf.data()), buf.length());
  EXPECT_EQ(content, "linked");
}

TEST(RpcMetadataUtil, interceptorFrameworkMetadata) {
  RpcOptions rpcOptions;
  auto kind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
  std::string methodName = "foo";
  std::string serviceName = "bar";
  std::chrono::milliseconds timeout(100);
  std::variant<InteractionCreate, int64_t, std::monostate> interactionHandle =
      std::monostate{};
  transport::THeader header;
  header.setProtocolId(protocol::T_COMPACT_PROTOCOL);

  rpcOptions.setShardId("123");
  MethodMetadata methodMetadata(
      MethodMetadata::Data(
          methodName, FunctionQualifier::Unspecified, serviceName));
  auto requestRpcMetadata = makeRequestRpcMetadata(
      rpcOptions,
      kind,
      std::move(methodMetadata),
      timeout,
      interactionHandle,
      /*serverZstdSupported=*/false,
      /*serverLz4Supported=*/false,
      3000,
      header,
      folly::IOBuf::copyBuffer(std::string("interceptor_metadata")),
      /*customCompressionEnabled=*/false);
  const auto& buf = **requestRpcMetadata.frameworkMetadata();
  std::string content(reinterpret_cast<const char*>(buf.data()), buf.length());
  EXPECT_EQ(content, "interceptor_metadata");
}

TEST(RpcMetadataUtil, CustomCompressionFallback) {
  RpcOptions rpcOptions;
  auto kind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
  std::string methodName = "foo";
  std::string serviceName = "bar";
  std::chrono::milliseconds timeout(100);
  std::variant<InteractionCreate, int64_t, std::monostate> interactionHandle =
      std::monostate{};

  transport::THeader header;
  header.setProtocolId(protocol::T_COMPACT_PROTOCOL);

  {
    CompressionConfig compressionConfig;
    compressionConfig.codecConfig().ensure().customConfig().emplace();
    header.setDesiredCompressionConfig(std::move(compressionConfig));

    MethodMetadata methodMetadata(
        MethodMetadata::Data(
            methodName, FunctionQualifier::Unspecified, serviceName));
    auto requestRpcMetadata = makeRequestRpcMetadata(
        rpcOptions,
        kind,
        std::move(methodMetadata),
        timeout,
        interactionHandle,
        /*serverZstdSupported=*/false,
        /*serverLz4Supported=*/false,
        3000,
        header,
        folly::IOBuf::copyBuffer(std::string("")),
        /*customCompressionEnabled=*/false);

    // custom compression not yet negotiated: fall back to zstd even when
    // serverZstdSupported is not known on this side (all Rocket servers support
    // zstd decompression), rather than the pessimal zlib.
    EXPECT_TRUE(requestRpcMetadata.compressionConfig()
                    .value()
                    .codecConfig()
                    ->zstdConfig()
                    .has_value());
    EXPECT_EQ(
        requestRpcMetadata.compression().value(), CompressionAlgorithm::ZSTD);
  }

  {
    CompressionConfig compressionConfig;
    compressionConfig.codecConfig().ensure().customConfig().emplace();
    header.setDesiredCompressionConfig(std::move(compressionConfig));

    MethodMetadata methodMetadata(
        MethodMetadata::Data(
            methodName, FunctionQualifier::Unspecified, serviceName));
    auto requestRpcMetadata = makeRequestRpcMetadata(
        rpcOptions,
        kind,
        std::move(methodMetadata),
        timeout,
        interactionHandle,
        /*serverZstdSupported=*/true,
        /*serverLz4Supported=*/false,
        3000,
        header,
        folly::IOBuf::copyBuffer(std::string("")),
        /*customCompressionEnabled=*/false);

    // use zstd if server supports zstd and custom compression is not supported
    EXPECT_TRUE(requestRpcMetadata.compressionConfig()
                    .value()
                    .codecConfig()
                    ->zstdConfig()
                    .has_value());
    EXPECT_EQ(
        requestRpcMetadata.compression().value(), CompressionAlgorithm::ZSTD);
  }

  {
    CompressionConfig compressionConfig;
    compressionConfig.codecConfig().ensure().customConfig().emplace();
    header.setDesiredCompressionConfig(std::move(compressionConfig));

    MethodMetadata methodMetadata(
        MethodMetadata::Data(
            methodName, FunctionQualifier::Unspecified, serviceName));
    auto requestRpcMetadata = makeRequestRpcMetadata(
        rpcOptions,
        kind,
        std::move(methodMetadata),
        timeout,
        interactionHandle,
        /*serverZstdSupported=*/false,
        /*serverLz4Supported=*/false,
        3000,
        header,
        folly::IOBuf::copyBuffer(std::string("")),
        /*customCompressionEnabled=*/true);

    // use custom compression if it is supported
    EXPECT_TRUE(requestRpcMetadata.compressionConfig()
                    .value()
                    .codecConfig()
                    ->customConfig()
                    .has_value());
    EXPECT_EQ(
        requestRpcMetadata.compression().value(), CompressionAlgorithm::CUSTOM);
  }

  {
    CompressionConfig compressionConfig;
    compressionConfig.codecConfig().ensure().zstdConfig().emplace();
    header.setDesiredCompressionConfig(std::move(compressionConfig));

    MethodMetadata methodMetadata(
        MethodMetadata::Data(
            methodName, FunctionQualifier::Unspecified, serviceName));
    auto requestRpcMetadata = makeRequestRpcMetadata(
        rpcOptions,
        kind,
        std::move(methodMetadata),
        timeout,
        interactionHandle,
        /*serverZstdSupported=*/false,
        /*serverLz4Supported=*/false,
        3000,
        header,
        folly::IOBuf::copyBuffer(std::string("")),
        /*customCompressionEnabled=*/true);

    // even if custom compression is supported, only use it if user requests it
    EXPECT_TRUE(requestRpcMetadata.compressionConfig()
                    .value()
                    .codecConfig()
                    ->zstdConfig()
                    .has_value());
    EXPECT_EQ(
        requestRpcMetadata.compression().value(), CompressionAlgorithm::ZSTD);
  }
}

TEST(RpcMetadataUtil, CustomCompressionFallbackToZlibWhenFlagDisabled) {
  THRIFT_FLAG_SET_MOCK(
      thrift_client_custom_compression_fallback_to_zstd, false);
  SCOPE_EXIT {
    THRIFT_FLAG_UNMOCK(thrift_client_custom_compression_fallback_to_zstd);
  };

  RpcOptions rpcOptions;
  auto kind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
  std::string methodName = "foo";
  std::string serviceName = "bar";
  std::chrono::milliseconds timeout(100);
  std::variant<InteractionCreate, int64_t, std::monostate> interactionHandle =
      std::monostate{};

  transport::THeader header;
  header.setProtocolId(protocol::T_COMPACT_PROTOCOL);

  CompressionConfig compressionConfig;
  compressionConfig.codecConfig().ensure().customConfig().emplace();
  header.setDesiredCompressionConfig(std::move(compressionConfig));

  MethodMetadata methodMetadata(
      MethodMetadata::Data(
          methodName, FunctionQualifier::Unspecified, serviceName));
  auto requestRpcMetadata = makeRequestRpcMetadata(
      rpcOptions,
      kind,
      std::move(methodMetadata),
      timeout,
      interactionHandle,
      /*serverZstdSupported=*/false,
      /*serverLz4Supported=*/false,
      3000,
      header,
      folly::IOBuf::copyBuffer(std::string("")),
      /*customCompressionEnabled=*/false);

  // Flag disabled restores the legacy behavior: custom compression not yet
  // negotiated falls back to zlib, and with serverZstdSupported=false there is
  // no subsequent zlib -> zstd upgrade.
  EXPECT_TRUE(requestRpcMetadata.compressionConfig()
                  .value()
                  .codecConfig()
                  ->zlibConfig()
                  .has_value());
  EXPECT_EQ(
      requestRpcMetadata.compression().value(), CompressionAlgorithm::ZLIB);
}

namespace {
// Builds the metadata a client would send for a request that asked for LZ4.
RequestRpcMetadata makeLz4RequestMetadata(
    bool serverLz4Supported, bool serverZstdSupported) {
  RpcOptions rpcOptions;
  std::string methodName = "foo";
  std::string serviceName = "bar";
  std::chrono::milliseconds timeout(100);
  std::variant<InteractionCreate, int64_t, std::monostate> interactionHandle =
      std::monostate{};

  transport::THeader header;
  header.setProtocolId(protocol::T_COMPACT_PROTOCOL);

  CompressionConfig compressionConfig;
  compressionConfig.codecConfig().ensure().lz4Config().emplace();
  header.setDesiredCompressionConfig(std::move(compressionConfig));

  MethodMetadata methodMetadata(
      MethodMetadata::Data(
          methodName, FunctionQualifier::Unspecified, serviceName));
  return makeRequestRpcMetadata(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE,
      std::move(methodMetadata),
      timeout,
      interactionHandle,
      serverZstdSupported,
      serverLz4Supported,
      3000,
      header,
      folly::IOBuf::copyBuffer(std::string("")),
      /*customCompressionEnabled=*/false);
}
} // namespace

// The peer never advertised LZ4, so the request is downgraded. It advertised
// zstd, so the zlib -> zstd upgrade carries the downgrade on to zstd -- which
// is what a zlib-configured service was already sent before the LZ4 migration.
TEST(RpcMetadataUtil, Lz4DowngradedToZstdWhenPeerSupportsZstd) {
  THRIFT_FLAG_SET_MOCK(thrift_client_lz4_downgrade_when_unsupported, true);
  SCOPE_EXIT {
    THRIFT_FLAG_UNMOCK(thrift_client_lz4_downgrade_when_unsupported);
  };

  auto metadata = makeLz4RequestMetadata(
      /*serverLz4Supported=*/false, /*serverZstdSupported=*/true);

  EXPECT_TRUE(metadata.compressionConfig()
                  .value()
                  .codecConfig()
                  ->zstdConfig()
                  .has_value());
  EXPECT_EQ(metadata.compression().value(), CompressionAlgorithm::ZSTD);
}

// A peer that advertised neither codec gets zlib: the downgrade lands there and
// there is no zstd to upgrade it to.
TEST(RpcMetadataUtil, Lz4DowngradedToZlibWhenPeerSupportsNeither) {
  THRIFT_FLAG_SET_MOCK(thrift_client_lz4_downgrade_when_unsupported, true);
  SCOPE_EXIT {
    THRIFT_FLAG_UNMOCK(thrift_client_lz4_downgrade_when_unsupported);
  };

  auto metadata = makeLz4RequestMetadata(
      /*serverLz4Supported=*/false, /*serverZstdSupported=*/false);

  EXPECT_TRUE(metadata.compressionConfig()
                  .value()
                  .codecConfig()
                  ->zlibConfig()
                  .has_value());
  EXPECT_EQ(metadata.compression().value(), CompressionAlgorithm::ZLIB);
}

// A peer that advertises LZ4 keeps it.
TEST(RpcMetadataUtil, Lz4PreservedWhenPeerAdvertisesSupport) {
  THRIFT_FLAG_SET_MOCK(thrift_client_lz4_downgrade_when_unsupported, true);
  SCOPE_EXIT {
    THRIFT_FLAG_UNMOCK(thrift_client_lz4_downgrade_when_unsupported);
  };

  auto metadata = makeLz4RequestMetadata(
      /*serverLz4Supported=*/true, /*serverZstdSupported=*/true);

  EXPECT_TRUE(metadata.compressionConfig()
                  .value()
                  .codecConfig()
                  ->lz4Config()
                  .has_value());
  EXPECT_EQ(metadata.compression().value(), CompressionAlgorithm::LZ4);
}

// At the flag's default the downgrade is inert, so landing this cannot change
// what any client sends until the flag is turned on. Deliberately does not mock
// the flag, so it asserts the shipped default.
TEST(RpcMetadataUtil, Lz4KeptWhenFlagDisabled) {
  auto metadata = makeLz4RequestMetadata(
      /*serverLz4Supported=*/false, /*serverZstdSupported=*/true);

  EXPECT_TRUE(metadata.compressionConfig()
                  .value()
                  .codecConfig()
                  ->lz4Config()
                  .has_value());
  EXPECT_EQ(metadata.compression().value(), CompressionAlgorithm::LZ4);
}

namespace {
RequestRpcMetadata makeMetadataWithCompression(
    std::optional<CompressionConfig> requestConfig,
    std::optional<CompressionConfig> responseConfig,
    bool customCompressionEnabled = true) {
  transport::THeader header;
  header.setProtocolId(protocol::T_COMPACT_PROTOCOL);
  if (requestConfig) {
    header.setDesiredCompressionConfig(std::move(*requestConfig));
  }
  if (responseConfig) {
    header.setDesiredResponseCompressionConfig(std::move(*responseConfig));
  }

  MethodMetadata methodMetadata(
      MethodMetadata::Data("foo", FunctionQualifier::Unspecified, "bar"));
  return makeRequestRpcMetadata(
      RpcOptions(),
      RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE,
      std::move(methodMetadata),
      std::chrono::milliseconds(100),
      std::monostate{},
      /*serverZstdSupported=*/true,
      /*serverLz4Supported=*/false,
      3000,
      header,
      folly::IOBuf::copyBuffer(std::string("")),
      customCompressionEnabled);
}

CompressionConfig makeCompressionConfig(CompressionAlgorithm algorithm) {
  CompressionConfig config;
  config.codecConfig() =
      rocket::CompressionAlgorithmSelector::toCodecConfig(algorithm);
  return config;
}
} // namespace

// The override replaces only the response policy; the request codec is
// untouched.
TEST(RpcMetadataUtil, ResponseCompressionConfigOverridesResponsePolicy) {
  auto metadata = makeMetadataWithCompression(
      makeCompressionConfig(CompressionAlgorithm::CUSTOM),
      makeCompressionConfig(CompressionAlgorithm::ZSTD));

  EXPECT_EQ(metadata.compression().value(), CompressionAlgorithm::CUSTOM);
  EXPECT_TRUE(metadata.compressionConfig()
                  .value()
                  .codecConfig()
                  ->zstdConfig()
                  .has_value());
}

// An empty override reaches the wire as a response policy with no codec, which
// the server treats as no compression. The request codec is untouched.
TEST(RpcMetadataUtil, EmptyResponseCompressionConfigRequestsNoCodec) {
  auto metadata = makeMetadataWithCompression(
      makeCompressionConfig(CompressionAlgorithm::CUSTOM), CompressionConfig{});

  EXPECT_EQ(metadata.compression().value(), CompressionAlgorithm::CUSTOM);
  ASSERT_TRUE(metadata.compressionConfig().has_value());
  EXPECT_FALSE(metadata.compressionConfig()->codecConfig().has_value());
}

// The override applies even when the request has no compression config.
TEST(RpcMetadataUtil, ResponseCompressionConfigWithoutRequestCompression) {
  auto metadata = makeMetadataWithCompression(
      std::nullopt, makeCompressionConfig(CompressionAlgorithm::ZSTD));

  EXPECT_FALSE(metadata.compression().has_value());
  EXPECT_TRUE(metadata.compressionConfig()
                  .value()
                  .codecConfig()
                  ->zstdConfig()
                  .has_value());
}

// The response override gets the same server-capability adjustment as the
// request config: CUSTOM falls back to zstd before negotiation.
TEST(RpcMetadataUtil, ResponseOverrideAdjustedToServerCapabilities) {
  auto metadata = makeMetadataWithCompression(
      makeCompressionConfig(CompressionAlgorithm::ZSTD),
      makeCompressionConfig(CompressionAlgorithm::CUSTOM),
      /*customCompressionEnabled=*/false);

  EXPECT_TRUE(metadata.compressionConfig()
                  .value()
                  .codecConfig()
                  ->zstdConfig()
                  .has_value());
}

// A normal-sized exception message is copied through unchanged.
TEST(RpcMetadataUtil, ClampExceptionWhatSmallMessageUnchanged) {
  const std::string what = "boom: something went wrong";
  EXPECT_EQ(clampExceptionWhatForHeader(what), what);
}

// A message exactly at the limit still fits, so it is left unchanged.
TEST(RpcMetadataUtil, ClampExceptionWhatAtLimitUnchanged) {
  const std::string what(kMaxExceptionWhatHeaderSize, 'x');
  EXPECT_EQ(clampExceptionWhatForHeader(what), what);
}

// An oversized message (e.g. an ~8MB proxied exception) is clamped so it can
// never overflow the THeader 16-bit header-length field. See S669483.
TEST(RpcMetadataUtil, ClampExceptionWhatTruncatesOversizedMessage) {
  const std::string what(kMaxExceptionWhatHeaderSize + 4096, 'x');
  const std::string clamped = clampExceptionWhatForHeader(what);

  // Result fits exactly within the budget...
  EXPECT_EQ(clamped.size(), kMaxExceptionWhatHeaderSize);
  // ...keeps the original prefix...
  EXPECT_EQ(clamped.substr(0, 32), std::string(32, 'x'));
  // ...and is marked as truncated rather than silently cut.
  EXPECT_NE(clamped.find("truncated"), std::string::npos);
}
