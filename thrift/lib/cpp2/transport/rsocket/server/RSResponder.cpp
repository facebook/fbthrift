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
#include <thrift/lib/cpp2/transport/rsocket/server/RequestResponse.h>
#include <thrift/lib/cpp2/transport/rsocket/server/StreamOutput.h>

#include <rsocket/internal/ScheduledSubscriber.h>

namespace apache {
namespace thrift {

using namespace rsocket;

RSResponder::RSResponder(
    ThriftProcessor* processor,
    folly::EventBase* evb,
    std::shared_ptr<apache::thrift::server::TServerObserver> observer)
    : processor_(processor), evb_(evb), observer_(std::move(observer)) {}

void RSResponder::handleRequestResponse(
    Payload request,
    StreamId,
    std::shared_ptr<yarpl::single::SingleObserver<Payload>> response) noexcept {
  DCHECK(request.metadata);
  auto metadata = RequestResponse::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);

  auto channel = std::make_shared<RequestResponse>(evb_, std::move(response));
  processor_->onThriftRequest(
      std::move(metadata), std::move(request.data), channel);
}

void RSResponder::handleFireAndForget(Payload request, StreamId) {
  auto metadata = RequestResponse::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);

  auto channel = std::make_shared<RSServerThriftChannel>(evb_);
  processor_->onThriftRequest(
      std::move(metadata), std::move(request.data), std::move(channel));
}

void RSResponder::handleRequestStream(
    Payload request,
    StreamId streamId,
    std::shared_ptr<yarpl::flowable::Subscriber<Payload>> response) noexcept {
  auto metadata = RequestResponse::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);
  request.metadata.reset();

  auto channel =
      std::make_shared<StreamOutput>(evb_, streamId, std::move(response));
  processor_->onThriftRequest(
      std::move(metadata), std::move(request.data), std::move(channel));
}

} // namespace thrift
} // namespace apache
