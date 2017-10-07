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

#include <thrift/lib/cpp2/transport/rsocket/server/RequestResponseThriftChannel.h>

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace apache {
namespace thrift {

std::unique_ptr<folly::IOBuf> RequestResponseThriftChannel::serializeMetadata(
    const ResponseRpcMetadata& responseMetadata) {
  CompactProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  responseMetadata.write(&writer);
  return queue.move();
}

std::unique_ptr<RequestRpcMetadata>
RequestResponseThriftChannel::deserializeMetadata(const folly::IOBuf& buffer) {
  CompactProtocolReader reader;
  auto metadata = std::make_unique<RequestRpcMetadata>();
  reader.setInput(&buffer);
  metadata->read(&reader);
  return metadata;
}

void RequestResponseThriftChannel::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf) noexcept {
  DCHECK(evb_->isInEventBaseThread()) << "Should be called in IO thread";
  DCHECK(metadata);

  VLOG(3) << "sendThriftResponse";
  subscriber_->onSubscribe(yarpl::single::SingleSubscriptions::empty());

  VLOG(3)
      << "Metadata: "
      << folly::humanify(
             serializeMetadata(*metadata)->cloneCoalesced()->moveToFbString());

  subscriber_->onSuccess(
      rsocket::Payload(std::move(buf), serializeMetadata(*metadata)));
  subscriber_ = nullptr;
}
} // namespace thrift
} // namespace apache
