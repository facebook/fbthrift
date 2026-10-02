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

#include <thrift/lib/cpp2/transport/rocket/compression/CompressionManager.h>

#include <folly/compression/Utils.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/IOBufChainCursor.h>
#include <thrift/lib/cpp2/transport/rocket/compression/CompressionAlgorithmSelector.h>

namespace apache::thrift::rocket {
namespace detail {

template <typename Metadata>
static void setCompressionCodec(
    CompressionConfig compressionConfig,
    Metadata& metadata,
    size_t payloadSize) {
  if (const auto& codecConfig = compressionConfig.codecConfig()) {
    if (payloadSize >
        static_cast<size_t>(
            compressionConfig.compressionSizeLimit().value_or(0))) {
      metadata.compression() =
          CompressionAlgorithmSelector::fromCodecConfig(*codecConfig);
    }
  }
}

template void setCompressionCodec<>(
    CompressionConfig compressionConfig,
    RequestRpcMetadata& metadata,
    size_t payloadSize);
template void setCompressionCodec<>(
    CompressionConfig compressionConfig,
    ResponseRpcMetadata& metadata,
    size_t payloadSize);
template void setCompressionCodec<>(
    CompressionConfig compressionConfig,
    StreamPayloadMetadata& metadata,
    size_t payloadSize);
} // namespace detail

static std::unique_ptr<folly::IOBuf> compressBuffer(
    std::unique_ptr<folly::IOBuf>&& buffer,
    CompressionAlgorithm compressionAlgorithm) {
  auto [codecType, level] =
      CompressionAlgorithmSelector::toCodecTypeAndLevel(compressionAlgorithm);
  try {
    return folly::compression::getCodec(codecType, level)
        ->compress(buffer.get());
  } catch (const std::exception& e) {
    throw TApplicationException(
        TApplicationException::INVALID_TRANSFORM,
        fmt::format("compression failure: {}", e.what()));
  }
}

static std::unique_ptr<folly::IOBuf> uncompressBufferImpl(
    std::unique_ptr<folly::IOBuf>&& buffer,
    folly::compression::CodecType codecType,
    int level) {
  return folly::compression::getCodec(codecType, level)
      ->uncompress(buffer.get());
}

static IOBufChain uncompressBufferImpl(
    IOBufChain&& buffer, folly::compression::CodecType codecType, int level) {
  if (codecType == folly::compression::CodecType::NO_COMPRESSION) {
    return std::move(buffer);
  }
  if (codecType == folly::compression::CodecType::LZ4_VARINT_SIZE) {
    throw std::invalid_argument(
        "IOBufChain LZ4 decompression is not supported");
  }

  auto codec = folly::compression::getStreamCodec(codecType, level);
  io::IOBufChainCursor cursor(buffer);
  const auto uncompressedLength = codec->getUncompressedLength(
      folly::StringPiece{cursor.peekBytes()}, folly::none);
  IOBufChain result;
  folly::compression::detail::uncompressStream(
      *codec, cursor, buffer.chainLength(), uncompressedLength, result);
  if (uncompressedLength && *uncompressedLength != result.chainLength()) {
    throw std::runtime_error("Codec: invalid uncompressed length");
  }
  return result;
}

template <typename Buffer>
static Buffer uncompressBuffer(
    Buffer&& buffer, CompressionAlgorithm compressionAlgorithm) {
  auto [codecType, level] =
      CompressionAlgorithmSelector::toCodecTypeAndLevel(compressionAlgorithm);
  try {
    return uncompressBufferImpl(std::forward<Buffer>(buffer), codecType, level);
  } catch (const std::exception& e) {
    throw TApplicationException(
        TApplicationException::INVALID_TRANSFORM,
        fmt::format("decompression failure: {}", e.what()));
  }
}

CompressionAlgorithm CompressionManager::fromCodecConfig(
    const CodecConfig& codecConfig) {
  return CompressionAlgorithmSelector::fromCodecConfig(codecConfig);
}

std::pair<folly::compression::CodecType, int>
CompressionManager::toCodecTypeAndLevel(
    const CompressionAlgorithm& compressionAlgorithm) {
  return CompressionAlgorithmSelector::toCodecTypeAndLevel(
      compressionAlgorithm);
}

void CompressionManager::setCompressionCodec(
    CompressionConfig compressionConfig,
    RequestRpcMetadata& metadata,
    size_t payloadSize) {
  detail::setCompressionCodec(compressionConfig, metadata, payloadSize);
}

void CompressionManager::setCompressionCodec(
    CompressionConfig compressionConfig,
    ResponseRpcMetadata& metadata,
    size_t payloadSize) {
  detail::setCompressionCodec(compressionConfig, metadata, payloadSize);
}

void CompressionManager::setCompressionCodec(
    CompressionConfig compressionConfig,
    StreamPayloadMetadata& metadata,
    size_t payloadSize) {
  detail::setCompressionCodec(compressionConfig, metadata, payloadSize);
}

std::unique_ptr<folly::IOBuf> CompressionManager::compressBuffer(
    std::unique_ptr<folly::IOBuf>&& buffer,
    CompressionAlgorithm compressionAlgorithm) {
  return rocket::compressBuffer(std::move(buffer), compressionAlgorithm);
}

std::unique_ptr<folly::IOBuf> CompressionManager::uncompressBuffer(
    std::unique_ptr<folly::IOBuf>&& buffer,
    CompressionAlgorithm compressionAlgorithm) {
  return rocket::uncompressBuffer(std::move(buffer), compressionAlgorithm);
}

IOBufChain CompressionManager::uncompressBuffer(
    IOBufChain&& buffer, CompressionAlgorithm compressionAlgorithm) {
  return rocket::uncompressBuffer(std::move(buffer), compressionAlgorithm);
}

bool isLz4Supported() {
  // Derived from the selector rather than naming a CodecType directly, so this
  // cannot drift from the algorithm actually used on the wire.
  static const bool supported = folly::compression::hasCodec(
      CompressionAlgorithmSelector::toCodecTypeAndLevel(
          CompressionAlgorithm::LZ4)
          .first);
  return supported;
}

} // namespace apache::thrift::rocket
