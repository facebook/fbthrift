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

#pragma once

#include <chrono>

#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

class RpcOptions;

namespace detail {

RequestRpcMetadata makeRequestRpcMetadata(
    const RpcOptions& rpcOptions,
    RpcKind kind,
    ProtocolId protocolId,
    folly::StringPiece methodName,
    std::chrono::milliseconds defaultChannelTimeout,
    transport::THeader& header,
    const transport::THeader::StringToStringMap& persistentWriteHeaders);

void fillTHeaderFromResponseRpcMetadata(
    ResponseRpcMetadata& responseMetadata,
    transport::THeader& header);

void fillResponseRpcMetadataFromTHeader(
    transport::THeader& header,
    ResponseRpcMetadata& responseMetadata);

} // namespace detail
} // namespace thrift
} // namespace apache
