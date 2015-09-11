/*
 * Copyright 2015 Facebook, Inc.
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
#include <thrift/lib/cpp2/async/HeaderCapableChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp/TApplicationException.h>

namespace apache {
namespace thrift {

using apache::thrift::transport::THeader;

void HeaderCapableChannel::addRpcOptionHeaders(THeader* header,
                                               RpcOptions& rpcOptions) {
  if (!clientSupportHeader()) {
    return;
  }

  if (rpcOptions.getPriority() != apache::thrift::concurrency::N_PRIORITIES) {
    header->setHeader(transport::THeader::PRIORITY_HEADER,
                      folly::to<std::string>(rpcOptions.getPriority()));
  }

  if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
    header->setHeader(transport::THeader::CLIENT_TIMEOUT_HEADER,
                      folly::to<std::string>(rpcOptions.getTimeout().count()));
  }
}
}
} // apache::thrift
