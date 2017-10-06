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
#include <thrift/lib/cpp2/transport/rsocket/server/RSResponder.h>

#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RequestResponseThriftChannel.h>

namespace apache {
namespace thrift {

RSResponder::RSResponder(ThriftProcessor* processor, folly::EventBase* evb)
    : processor_(processor), evb_(evb) {}

yarpl::Reference<yarpl::single::Single<rsocket::Payload>>
RSResponder::handleRequestResponse(
    rsocket::Payload request,
    rsocket::StreamId streamId) {
  // TODO: Where do we use this streamId value
  DCHECK(request.metadata);
  return yarpl::single::Single<rsocket::Payload>::create(
      [this, request = std::move(request), streamId](auto subscriber) mutable {
        VLOG(4) << "RSResponder.handleRequestResponse : " << request;

        auto metadata = RequestResponseThriftChannel::deserializeMetadata(
            *request.metadata);
        if (!metadata->__isset.kind) {
          metadata->kind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
          metadata->__isset.kind = true;
        }
        if (!metadata->__isset.seqId) {
          metadata->seqId = streamId;
          metadata->__isset.seqId = true;
        }

        auto channel =
            std::make_shared<RequestResponseThriftChannel>(evb_, subscriber);
        processor_->onThriftRequest(
            std::move(metadata), std::move(request.data), channel);

      });
}
} // namespace thrift
} // namespace apache
