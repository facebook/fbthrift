/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <chrono>
#include <map>
#include <string>
#include <utility>

#include <folly/Conv.h>

#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/RequestCallback.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {
namespace detail {

RequestRpcMetadata makeRequestRpcMetadata(
    const RpcOptions& rpcOptions,
    RpcKind kind,
    ProtocolId protocolId,
    folly::StringPiece methodName,
    std::chrono::milliseconds defaultChannelTimeout,
    transport::THeader& header,
    const transport::THeader::StringToStringMap& persistentWriteHeaders) {
  RequestRpcMetadata metadata;
  uint64_t flags = 0;
  metadata.protocol_ref() = protocolId;
  metadata.kind_ref() = kind;
  metadata.name_ref() = methodName.str();
  if (rpcOptions.getTimeout() > std::chrono::milliseconds::zero()) {
    metadata.clientTimeoutMs_ref() = rpcOptions.getTimeout().count();
  } else if (defaultChannelTimeout > std::chrono::milliseconds::zero()) {
    metadata.clientTimeoutMs_ref() = defaultChannelTimeout.count();
  }
  if (rpcOptions.getQueueTimeout() > std::chrono::milliseconds::zero()) {
    metadata.queueTimeoutMs_ref() = rpcOptions.getQueueTimeout().count();
  }
  if (rpcOptions.getPriority() < concurrency::N_PRIORITIES) {
    metadata.priority_ref() =
        static_cast<RpcPriority>(rpcOptions.getPriority());
  }
  if (header.getCrc32c().has_value()) {
    metadata.crc32c_ref() = header.getCrc32c().value();
  }
  // add user specified compression settings to metadata
  if (auto compressionConfig = header.getDesiredCompressionConfig()) {
    metadata.compressionConfig_ref() = *compressionConfig;
  }

  auto writeHeaders = header.releaseWriteHeaders();
  if (auto* eh = header.getExtraWriteHeaders()) {
    // Extra write headers always take precedence over write headers (see
    // THeader.cpp). We must copy here since we don't own the extra write
    // headers.
    for (const auto& entry : *eh) {
      writeHeaders[entry.first] = entry.second;
    }
  }
  writeHeaders.insert(
      persistentWriteHeaders.begin(), persistentWriteHeaders.end());

  // If server load was requested via THeader, use QUERY_SERVER_LOAD flag
  // instead.
  auto loadIt = writeHeaders.find(transport::THeader::QUERY_LOAD_HEADER);
  if (loadIt != writeHeaders.end()) {
    flags |= static_cast<uint64_t>(RequestRpcMetadataFlags::QUERY_SERVER_LOAD);
    metadata.loadMetric_ref() = std::move(loadIt->second);
    writeHeaders.erase(loadIt);
  }

  if (!writeHeaders.empty()) {
    metadata.otherMetadata_ref() = std::move(writeHeaders);
  }

  if (flags) {
    metadata.flags_ref() = flags;
  }

  return metadata;
}

void fillTHeaderFromResponseRpcMetadata(
    ResponseRpcMetadata& responseMetadata,
    transport::THeader& header) {
  std::map<std::string, std::string> otherMetadata;
  if (responseMetadata.otherMetadata_ref()) {
    otherMetadata = std::move(*responseMetadata.otherMetadata_ref());
  }
  if (auto load = responseMetadata.load_ref()) {
    otherMetadata[transport::THeader::QUERY_LOAD_HEADER] =
        folly::to<std::string>(*load);
  }
  if (auto crc32c = responseMetadata.crc32c_ref()) {
    header.setCrc32c(*crc32c);
  }
  if (auto compression = responseMetadata.compression_ref()) {
    // for fb internal logging purpose only; does not actually do transformation
    // based on THeader
    transport::THeader::TRANSFORMS transform;
    switch (*compression) {
      case apache::thrift::CompressionAlgorithm::ZSTD:
        transform = transport::THeader::ZSTD_TRANSFORM;
        break;
      case apache::thrift::CompressionAlgorithm::ZLIB:
        transform = transport::THeader::ZLIB_TRANSFORM;
        break;
      default:
        transform = transport::THeader::NONE;
        break;
    }
    if (transform != transport::THeader::NONE) {
      header.setReadTransform(static_cast<uint16_t>(transform));
    }
  }
  header.setReadHeaders(std::move(otherMetadata));
}

void fillResponseRpcMetadataFromTHeader(
    transport::THeader& header,
    ResponseRpcMetadata& responseMetadata) {
  std::map<std::string, std::string> otherMetadata = header.releaseHeaders();
  {
    auto loadIt = otherMetadata.find(transport::THeader::QUERY_LOAD_HEADER);
    if (loadIt != otherMetadata.end()) {
      responseMetadata.load_ref() = folly::to<int64_t>(loadIt->second);
      otherMetadata.erase(loadIt);
    }
  }
  if (auto crc32c = header.getCrc32c()) {
    responseMetadata.crc32c_ref() = *crc32c;
  }
  responseMetadata.otherMetadata_ref() = std::move(otherMetadata);
}
} // namespace detail
} // namespace thrift
} // namespace apache
