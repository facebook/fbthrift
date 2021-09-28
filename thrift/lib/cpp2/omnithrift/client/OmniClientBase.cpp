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

#include <thrift/lib/cpp2/omnithrift/client/OmniClientBase.h>

#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>

namespace apache {
namespace thrift {
namespace omniclient {

folly::SemiFuture<folly::IOBuf> OmniClientBase::semifuture_send(
    const std::string& functionName,
    const std::string& encodedArgs,
    const std::unordered_map<std::string, std::string>& headers) {
  return semifuture_sendWrapped(functionName, encodedArgs, headers)
      .deferValue([](OmniClientWrappedResponse&& response) {
        if (response.buf.hasValue()) {
          return response.buf.value();
        } else {
          response.buf.error().throw_exception();
        }
      });
}

} // namespace omniclient
} // namespace thrift
} // namespace apache
