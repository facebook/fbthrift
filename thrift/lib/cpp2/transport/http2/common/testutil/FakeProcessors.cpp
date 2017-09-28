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

using folly::EventBase;
using folly::IOBuf;

void EchoProcessor::onThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> payload,
    std::shared_ptr<ThriftChannelIf> channel) noexcept {
  evb_->runInEventBaseThread([
    this,
    evbMetadata = std::move(metadata),
    evbPayload = std::move(payload),
    evbChannel = std::move(channel)
  ]() mutable {
    onThriftRequestHelper(
        std::move(evbMetadata), std::move(evbPayload), std::move(evbChannel));
  });
}

void EchoProcessor::cancel(
    int32_t /*seqId*/,
    ThriftChannelIf* /*channel*/) noexcept {
  LOG(ERROR) << "cancel() unused in this fake object.";
}

void EchoProcessor::onThriftRequestHelper(
    std::unique_ptr<RequestRpcMetadata> requestMetadata,
    std::unique_ptr<folly::IOBuf> payload,
    std::shared_ptr<ThriftChannelIf> channel) noexcept {
  CHECK(requestMetadata);
  CHECK(payload);
  CHECK(channel);
  auto responseMetadata = std::make_unique<ResponseRpcMetadata>();
  if (requestMetadata->__isset.seqId) {
    responseMetadata->seqId = requestMetadata->seqId;
    responseMetadata->__isset.seqId = true;
  }
  if (requestMetadata->__isset.otherMetadata) {
    responseMetadata->otherMetadata = std::move(requestMetadata->otherMetadata);
  }
  (responseMetadata->otherMetadata)[key_] = value_;
  responseMetadata->__isset.otherMetadata = true;
  auto iobuf = IOBuf::copyBuffer(trailer_);
  payload->prependChain(std::move(iobuf));
  channel->sendThriftResponse(std::move(responseMetadata), std::move(payload));
}

} // namespace thrift
} // namespace apache
