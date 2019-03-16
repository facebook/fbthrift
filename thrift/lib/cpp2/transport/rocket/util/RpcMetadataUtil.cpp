/*
 * Copyright 2019-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rocket/util/RpcMetadataUtil.h>

#include <chrono>

#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {
namespace detail {

RequestRpcMetadata makeRequestRpcMetadata(
    const RpcOptions& rpcOptions,
    RpcKind kind,
    ProtocolId protocolId,
    std::chrono::milliseconds defaultChannelTimeout,
    transport::THeader& header,
    const transport::THeader::StringToStringMap& persistentWriteHeaders) {
  RequestRpcMetadata metadata;
  metadata.protocol_ref() = protocolId;
  metadata.kind_ref() = kind;
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
  if (header.getCrc32c().hasValue()) {
    metadata.crc32c_ref() = header.getCrc32c().value();
  }

  auto writeHeaders = header.releaseWriteHeaders();
  if (auto* eh = header.getExtraWriteHeaders()) {
    writeHeaders.insert(eh->begin(), eh->end());
  }
  writeHeaders.insert(
      persistentWriteHeaders.begin(), persistentWriteHeaders.end());
  if (!writeHeaders.empty()) {
    metadata.otherMetadata_ref() = std::move(writeHeaders);
  }

  return metadata;
}

} // namespace detail
} // namespace thrift
} // namespace apache
