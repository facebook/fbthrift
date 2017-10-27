/*
 * Copyright 2004-present Facebook, Inc.
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
#pragma once

#include <thrift/lib/cpp2/transport/http2/common/SingleRpcChannel.h>

namespace apache {
namespace thrift {

class MetadataInBodySingleRpcChannel : public SingleRpcChannel {
 public:
  MetadataInBodySingleRpcChannel(
      proxygen::ResponseHandler* toHttp2,
      ThriftProcessor* processor);

  MetadataInBodySingleRpcChannel(
      H2ClientConnection* toHttp2,
      const std::string& httpHost,
      const std::string& httpUrl);

  void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept override;

 protected:
  // The service side handling code for onH2StreamEnd().
  void onThriftRequest() noexcept override;
};

} // namespace thrift
} // namespace apache
