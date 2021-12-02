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

#include <folly/Optional.h>
#include <folly/dynamic.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/PluggableFunction.h>
#include <thrift/lib/cpp2/util/ManagedStringView.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

class RpcOptions;

namespace detail {

inline constexpr std::string_view kHeaderUex = "uex";
inline constexpr std::string_view kHeaderUexw = "uexw";
inline constexpr std::string_view kHeaderEx = "ex";
inline constexpr std::string_view kHeaderExMeta = "exm";

THRIFT_PLUGGABLE_FUNC_DECLARE(
    std::unique_ptr<folly::IOBuf>,
    makeFrameworkMetadata,
    const RpcOptions&,
    folly::dynamic& logMessages);

RequestRpcMetadata makeRequestRpcMetadata(
    const RpcOptions& rpcOptions,
    RpcKind kind,
    ProtocolId protocolId,
    ManagedStringView&& methodName,
    std::chrono::milliseconds defaultChannelTimeout,
    transport::THeader& header);

void fillTHeaderFromResponseRpcMetadata(
    ResponseRpcMetadata& responseMetadata, transport::THeader& header);

void fillResponseRpcMetadataFromTHeader(
    transport::THeader& header, ResponseRpcMetadata& responseMetadata);

std::string serializeErrorClassification(ErrorClassification ec);
ErrorClassification deserializeErrorClassification(std::string_view str);

folly::Optional<std::string> errorKindToString(ErrorKind);
folly::Optional<std::string> errorBlameToString(ErrorBlame);
folly::Optional<std::string> errorSafetyToString(ErrorSafety);

} // namespace detail
} // namespace thrift
} // namespace apache
