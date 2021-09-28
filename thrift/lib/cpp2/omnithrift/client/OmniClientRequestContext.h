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

#include <folly/Memory.h>

#include <thrift/lib/cpp2/gen/client_cpp.h>

namespace apache {
namespace thrift {
namespace omniclient {

inline std::pair<
    std::unique_ptr<apache::thrift::ContextStack>,
    std::shared_ptr<apache::thrift::transport::THeader>>
makeOmniClientRequestContext(
    uint16_t protocolId,
    apache::thrift::transport::THeader::StringToStringMap headers,
    std::shared_ptr<
        std::vector<std::shared_ptr<apache::thrift::TProcessorEventHandler>>>
        handlers,
    const char* serviceName,
    const char* functionName) {
  auto header = std::make_shared<apache::thrift::transport::THeader>(
      apache::thrift::transport::THeader::ALLOW_BIG_FRAMES);
  header->setProtocolId(protocolId);
  header->setHeaders(std::move(headers));
  auto ctx = apache::thrift::ContextStack::createWithClientContext(
      handlers, serviceName, functionName, *header);

  return {std::move(ctx), std::move(header)};
}

} // namespace omniclient
} // namespace thrift
} // namespace apache
