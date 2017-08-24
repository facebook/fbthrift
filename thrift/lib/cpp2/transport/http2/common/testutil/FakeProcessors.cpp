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

#include <thrift/lib/cpp2/transport/http2/common/testutil/FakeProcessors.h>

#include <glog/logging.h>
#include <thrift/lib/cpp2/transport/http2/common/H2ChannelIf.h>

namespace apache {
namespace thrift {

using std::map;
using std::string;
using folly::EventBase;
using folly::IOBuf;

void EchoProcessor::onThriftRequest(
    std::unique_ptr<FunctionInfo> functionInfo,
    std::unique_ptr<std::map<std::string, std::string>> headers,
    std::unique_ptr<folly::IOBuf> payload,
    std::shared_ptr<ThriftChannelIf> channel) noexcept {
  evb_->runInEventBaseThread([
    this,
    evbFunctionInfo = std::move(functionInfo),
    evbHeaders = std::move(headers),
    evbPayload = std::move(payload),
    evbChannel = std::move(channel)
  ]() mutable {
    onThriftRequestHelper(
        std::move(evbFunctionInfo),
        std::move(evbHeaders),
        std::move(evbPayload),
        std::move(evbChannel));
  });
}

void EchoProcessor::cancel(
    uint32_t /*seqId*/,
    ThriftChannelIf* /*channel*/) noexcept {
  LOG(ERROR) << "cancel() unused in this fake object.";
}

void EchoProcessor::onThriftRequestHelper(
    std::unique_ptr<FunctionInfo> functionInfo,
    std::unique_ptr<std::map<std::string, std::string>> headers,
    std::unique_ptr<folly::IOBuf> payload,
    std::shared_ptr<ThriftChannelIf> channel) noexcept {
  CHECK(headers);
  CHECK(payload);
  CHECK(channel);
  (*headers)[key_] = value_;
  auto iobuf = IOBuf::copyBuffer(trailer_);
  payload->prependChain(std::move(iobuf));
  channel->sendThriftResponse(
      functionInfo->seqId, std::move(headers), std::move(payload));
}

} // namespace thrift
} // namespace apache
