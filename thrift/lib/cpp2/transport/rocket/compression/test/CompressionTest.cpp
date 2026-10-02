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

#include <string.h>

#include <gtest/gtest.h>

#include <folly/compression/Zstd.h>
#include <folly/io/Cursor.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/transport/rocket/compression/CompressionManager.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::rocket;

// Test helpers.

const auto baseBuffer = folly::IOBuf::copyBuffer(
    "Compression, a force so strong,\n"
    "It shrinks and shrinks, until it's gone.\n"
    "A tiny package, easy to store,\n"
    "But what lies within, we can't ignore.\n"
    "\n"
    "Uncompression, a process so grand,\n"
    "It reveals the secrets of the land.\n"
    "The data flows, like a river wide,\n"
    "And all is revealed, with no need to hide.\n"
    "\n"
    "Compression and uncompression, two sides of the same coin,\n"
    "Both important, both needed, to make our digital lives more fine.\n"
    "\n"
    "- Metamate\n");
const auto basePayloadSize = baseBuffer->computeChainDataLength();

enum class ExpectCompression { Empty, NotEmpty };

CodecConfig toCodecConfig(const ZlibCompressionCodecConfig& zlibConfig) {
  CodecConfig codecConfig;
  codecConfig.zlibConfig() = zlibConfig;
  return codecConfig;
}
CodecConfig toCodecConfig(const ZstdCompressionCodecConfig& zstdConfig) {
  CodecConfig codecConfig;
  codecConfig.zstdConfig() = zstdConfig;
  return codecConfig;
}
CodecConfig toCodecConfig(const Lz4CompressionCodecConfig& lz4Config) {
  CodecConfig codecConfig;
  codecConfig.lz4Config() = lz4Config;
  return codecConfig;
}

// Test compress.

void testCompress(const CompressionAlgorithm& compressionAlgorithm) {
  auto buffer = CompressionManager().compressBuffer(
      baseBuffer->clone(), compressionAlgorithm);
  EXPECT_FALSE(folly::IOBufEqualTo()(buffer, baseBuffer));
}

TEST(CompressionTest, unsetCompressDoesNotCompress) {
  auto buffer = CompressionManager().compressBuffer(
      baseBuffer->clone(), CompressionAlgorithm::NONE);
  EXPECT_TRUE(folly::IOBufEqualTo()(buffer, baseBuffer));
}

TEST(CompressionTest, zlibCompressSucceeds) {
  testCompress(CompressionAlgorithm::ZLIB);
  testCompress(CompressionAlgorithm::ZLIB_LESS);
  testCompress(CompressionAlgorithm::ZLIB_MORE);
}

TEST(CompressionTest, zstdCompressSucceeds) {
  testCompress(CompressionAlgorithm::ZSTD);
  testCompress(CompressionAlgorithm::ZSTD_LESS);
  testCompress(CompressionAlgorithm::ZSTD_MORE);
}

#if FOLLY_HAVE_LIBLZ4
TEST(CompressionTest, Lz4CompressSucceeds) {
  testCompress(CompressionAlgorithm::LZ4);
  testCompress(CompressionAlgorithm::LZ4_LESS);
  testCompress(CompressionAlgorithm::LZ4_MORE);
}
#endif

// Test uncompress.

void testUncompress(const CompressionAlgorithm& compressionAlgorithm) {
  auto buffer = CompressionManager().compressBuffer(
      baseBuffer->clone(), compressionAlgorithm);

  buffer = CompressionManager().uncompressBuffer(
      std::move(buffer), compressionAlgorithm);
  EXPECT_TRUE(folly::IOBufEqualTo()(buffer, baseBuffer));
}

IOBufChain makeSegmentedChain(const folly::IOBuf& buffer) {
  const auto length = buffer.computeChainDataLength();
  IOBufChain result((length + 6) / 7 + 1);
  folly::io::Cursor cursor(&buffer);
  size_t remaining = length;
  size_t segmentLength = 1;
  while (remaining != 0) {
    const auto size = std::min(segmentLength, remaining);
    auto segment = folly::IOBuf::create(size);
    cursor.pull(segment->writableData(), size);
    segment->append(size);
    result.append(std::move(segment));
    remaining -= size;
    segmentLength = 7;
  }
  return result;
}

void testChainUncompress(const CompressionAlgorithm& compressionAlgorithm) {
  auto compressed = CompressionManager().compressBuffer(
      baseBuffer->clone(), compressionAlgorithm);
  auto buffer = CompressionManager().uncompressBuffer(
      makeSegmentedChain(*compressed), compressionAlgorithm);
  EXPECT_EQ(buffer, IOBufChain(baseBuffer->clone()));
}

void testLargeChainUncompress(
    const CompressionAlgorithm& compressionAlgorithm) {
  constexpr size_t kLength = 1 << 20;
  auto source = folly::IOBuf::create(kLength);
  std::memset(source->writableData(), 'a', kLength);
  source->append(kLength);
  auto compressed = CompressionManager().compressBuffer(
      source->clone(), compressionAlgorithm);
  auto buffer = CompressionManager().uncompressBuffer(
      makeSegmentedChain(*compressed), compressionAlgorithm);
  EXPECT_EQ(buffer, IOBufChain(std::move(source)));
  EXPECT_GT(buffer.chainElements(), 1);
}

TEST(CompressionTest, unsetUncompressSucceeds) {
  testUncompress(CompressionAlgorithm::NONE);
}

TEST(CompressionTest, unsetChainUncompressSucceeds) {
  testChainUncompress(CompressionAlgorithm::NONE);
}

TEST(CompressionTest, zlibUncompressSucceeds) {
  testUncompress(CompressionAlgorithm::ZLIB);
  testUncompress(CompressionAlgorithm::ZLIB_LESS);
  testUncompress(CompressionAlgorithm::ZLIB_MORE);
}

TEST(CompressionTest, zlibChainUncompressSucceeds) {
  testChainUncompress(CompressionAlgorithm::ZLIB);
  testChainUncompress(CompressionAlgorithm::ZLIB_LESS);
  testChainUncompress(CompressionAlgorithm::ZLIB_MORE);
  testLargeChainUncompress(CompressionAlgorithm::ZLIB);
}

TEST(CompressionTest, zstdUncompressSucceeds) {
  testUncompress(CompressionAlgorithm::ZSTD);
  testUncompress(CompressionAlgorithm::ZSTD_LESS);
  testUncompress(CompressionAlgorithm::ZSTD_MORE);
}

TEST(CompressionTest, zstdChainUncompressSucceeds) {
  testChainUncompress(CompressionAlgorithm::ZSTD);
  testChainUncompress(CompressionAlgorithm::ZSTD_LESS);
  testChainUncompress(CompressionAlgorithm::ZSTD_MORE);
  testLargeChainUncompress(CompressionAlgorithm::ZSTD);
}

#if FOLLY_HAVE_LIBLZ4
TEST(CompressionTest, lz4UncompressSucceeds) {
  testUncompress(CompressionAlgorithm::LZ4);
  testUncompress(CompressionAlgorithm::LZ4_LESS);
  testUncompress(CompressionAlgorithm::LZ4_MORE);
}
#endif

TEST(CompressionTest, lz4ChainUncompressThrows) {
  for (const auto algorithm : {
           CompressionAlgorithm::LZ4,
           CompressionAlgorithm::LZ4_LESS,
           CompressionAlgorithm::LZ4_MORE,
       }) {
    EXPECT_THROW(
        CompressionManager().uncompressBuffer(
            IOBufChain(baseBuffer->clone()), algorithm),
        TApplicationException);
  }
}

// isLz4Supported() is what a server advertises in SetupResponse.lz4Supported,
// so it has to track the build. Over-reporting makes servers advertise a codec
// they cannot decode; under-reporting makes clients downgrade to zlib for no
// reason.
TEST(CompressionTest, isLz4SupportedTracksBuild) {
#if FOLLY_HAVE_LIBLZ4
  EXPECT_TRUE(isLz4Supported());
#else
  EXPECT_FALSE(isLz4Supported());
#endif
}

// The invariant that actually matters, asserted against observed behaviour
// rather than the build macro: if we claim LZ4, a round trip must work; if we
// do not claim it, using it must fail rather than silently corrupt.
TEST(CompressionTest, isLz4SupportedAgreesWithCodecUsability) {
  if (isLz4Supported()) {
    auto buffer = CompressionManager().compressBuffer(
        baseBuffer->clone(), CompressionAlgorithm::LZ4);
    buffer = CompressionManager().uncompressBuffer(
        std::move(buffer), CompressionAlgorithm::LZ4);
    EXPECT_TRUE(folly::IOBufEqualTo()(buffer, baseBuffer));
  } else {
    EXPECT_THROW(
        CompressionManager().compressBuffer(
            baseBuffer->clone(), CompressionAlgorithm::LZ4),
        TApplicationException);
  }
}

// Test setCompressionCodec.

template <typename Metadata, typename Config>
Metadata testSetCompressionCodec(
    const ExpectCompression& expectCompression = ExpectCompression::NotEmpty,
    const std::optional<size_t>& compressionSizeLimitOpt = std::nullopt) {
  CompressionManager cmb;
  Metadata metadata;
  auto codecConfig = toCodecConfig(Config());
  CompressionConfig compressionConfig;
  compressionConfig.codecConfig() = codecConfig;
  if (compressionSizeLimitOpt) {
    compressionConfig.compressionSizeLimit() = *compressionSizeLimitOpt;
  }

  cmb.setCompressionCodec(compressionConfig, metadata, basePayloadSize);

  if (expectCompression == ExpectCompression::Empty) {
    EXPECT_FALSE(metadata.compression_ref().has_value());
  } else if (expectCompression == ExpectCompression::NotEmpty) {
    EXPECT_TRUE(metadata.compression_ref().has_value());
  } // else expectCompression == ExpectCompression::NotApplicable

  return metadata;
}

template <typename Config>
void testSetCompressionCodec(
    const ExpectCompression& expectCompression = ExpectCompression::NotEmpty,
    const std::optional<size_t>& compressionSizeLimitOpt = std::nullopt) {
  testSetCompressionCodec<RequestRpcMetadata, Config>(
      expectCompression, compressionSizeLimitOpt);
  testSetCompressionCodec<ResponseRpcMetadata, Config>(
      expectCompression, compressionSizeLimitOpt);
  testSetCompressionCodec<StreamPayloadMetadata, Config>(
      expectCompression, compressionSizeLimitOpt);
}

void testSetCompressionCodec(
    const ExpectCompression& expectCompression = ExpectCompression::NotEmpty,
    const std::optional<size_t>& compressionSizeLimitOpt = std::nullopt) {
  testSetCompressionCodec<ZlibCompressionCodecConfig>(
      expectCompression, compressionSizeLimitOpt);
  testSetCompressionCodec<ZstdCompressionCodecConfig>(
      expectCompression, compressionSizeLimitOpt);
  testSetCompressionCodec<Lz4CompressionCodecConfig>(
      expectCompression, compressionSizeLimitOpt);
}

TEST(
    CompressionTest,
    setCompressionCodecWithHighCompressionSizeLimitDoesNotSetCompression) {
  testSetCompressionCodec(
      ExpectCompression::Empty /* expectCompression */,
      basePayloadSize /* compressionSizeLimitOpt */);
}

TEST(
    CompressionTest,
    setCompressionCodecWithLowCompressionSizeLimitSetsCompression) {
  testSetCompressionCodec(
      ExpectCompression::NotEmpty /* expectCompression */,
      basePayloadSize - 1 /* compressionSizeLimitOpt */);
}
