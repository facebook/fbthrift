/*
 * Copyright 2017-present Facebook, Inc.
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
#include <thrift/lib/cpp2/transport/http2/common/H2ChannelFactory.h>

#include <thrift/lib/cpp2/transport/http2/common/MetadataInBodySingleRpcChannel.h>
#include <thrift/lib/cpp2/transport/http2/common/SingleRpcChannel.h>

namespace apache {
namespace thrift {

std::shared_ptr<H2Channel> H2ChannelFactory::createChannel(
    int32_t version,
    proxygen::ResponseHandler* toHttp2,
    ThriftProcessor* processor) {
  if (version == 2) {
    return std::make_shared<MetadataInBodySingleRpcChannel>(toHttp2, processor);
  } else {
    DCHECK(version == 0 || version == 1);
    return std::make_shared<SingleRpcChannel>(toHttp2, processor);
  }
}

std::shared_ptr<H2Channel> H2ChannelFactory::getChannel(
    int32_t version,
    H2ClientConnection* toHttp2,
    const std::string& httpHost,
    const std::string& httpUrl) {
  if (version == 2) {
    return std::make_shared<MetadataInBodySingleRpcChannel>(
        toHttp2, httpHost, httpUrl);
  } else {
    DCHECK(version == 0 || version == 1);
    auto ch = std::make_shared<SingleRpcChannel>(toHttp2, httpHost, httpUrl);
    if (version == 0) {
      ch->setNotYetStable();
    }
    return ch;
  }
}

} // namespace thrift
} // namespace apache
