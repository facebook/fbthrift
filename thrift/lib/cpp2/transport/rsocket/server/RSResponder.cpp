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
#include <thrift/lib/cpp2/transport/rsocket/server/Channel.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RequestResponse.h>
#include <thrift/lib/cpp2/transport/rsocket/server/StreamInput.h>
#include <thrift/lib/cpp2/transport/rsocket/server/StreamOutput.h>

#include <rsocket/internal/ScheduledSubscriber.h>

namespace apache {
namespace thrift {

RSResponder::RSResponder(
    ThriftProcessor* processor,
    folly::EventBase* evb,
    std::shared_ptr<apache::thrift::server::TServerObserver> observer)
    : processor_(processor), evb_(evb), observer_(std::move(observer)) {}

std::shared_ptr<yarpl::single::Single<rsocket::Payload>>
RSResponder::handleRequestResponse(
    rsocket::Payload request,
    rsocket::StreamId) {
  // TODO: Where do we use this streamId value -> might make batching possible!
  DCHECK(request.metadata);
  return yarpl::single::Single<rsocket::Payload>::create(
      [this, request = std::move(request)](auto subscriber) mutable {
        auto metadata = RequestResponse::deserializeMetadata(*request.metadata);
        DCHECK(metadata->__isset.kind);
        DCHECK(metadata->__isset.seqId);

        auto channel = std::make_shared<RequestResponse>(evb_, subscriber);
        processor_->onThriftRequest(
            std::move(metadata), std::move(request.data), channel);
      });
}

void RSResponder::handleFireAndForget(
    rsocket::Payload request,
    rsocket::StreamId) {
  auto metadata = RequestResponse::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);

  auto channel = std::make_shared<RSServerThriftChannel>(evb_);
  processor_->onThriftRequest(
      std::move(metadata), std::move(request.data), std::move(channel));
}

RSResponder::FlowableRef RSResponder::handleRequestStream(
    rsocket::Payload request,
    rsocket::StreamId streamId) {
  (void)streamId;
  // TODO This id is internally used by RSocketStateMachine but we need one more
  // id that represents each RPC call - seqId.

  return yarpl::flowable::internal::flowableFromSubscriber<rsocket::Payload>(
      [this, request = std::move(request), streamId](auto subscriber) mutable {
        auto metadata = RequestResponse::deserializeMetadata(*request.metadata);
        DCHECK(metadata->__isset.kind);
        DCHECK(metadata->__isset.seqId);
        request.metadata.reset();

        auto channel = std::make_shared<StreamOutput>(
            evb_,
            streamId,
            std::make_shared<rsocket::ScheduledSubscriber<rsocket::Payload>>(
                std::move(subscriber), *evb_));
        processor_->onThriftRequest(
            std::move(metadata), std::move(request.data), std::move(channel));
      });
}

RSResponder::FlowableRef RSResponder::handleRequestChannel(
    rsocket::Payload request,
    FlowableRef requestStream,
    rsocket::StreamId streamId) {
  // TODO We might not need the ScheduledSubscriber/Subscription anymore!

  auto requestStreamFlowable =
      yarpl::flowable::internal::flowableFromSubscriber<rsocket::Payload>(
          [requestStream = std::move(requestStream), evb = evb_](
              std::shared_ptr<yarpl::flowable::Subscriber<rsocket::Payload>>
                  subscriber) {
            requestStream->subscribe(
                std::make_shared<
                    rsocket::ScheduledSubscriptionSubscriber<rsocket::Payload>>(
                    std::move(subscriber), *evb));
          });

  return yarpl::flowable::internal::flowableFromSubscriber<rsocket::Payload>(
      [this,
       request = std::move(request),
       requestStream = std::move(requestStreamFlowable),
       streamId](auto subscriber) mutable {
        auto metadata = RequestResponse::deserializeMetadata(*request.metadata);
        DCHECK(metadata->__isset.kind);
        DCHECK(metadata->__isset.seqId);

        // TODO - STREAMING_REQUEST_NO_RESPONSE?
        // TODO - STREAMING_REQUEST_SINGLE_RESPONSE?
        CHECK(metadata->kind != RpcKind::STREAMING_REQUEST_SINGLE_RESPONSE);
        CHECK(metadata->kind != RpcKind::STREAMING_REQUEST_NO_RESPONSE);

        auto channel = std::make_shared<Channel>(
            evb_, std::move(requestStream), streamId, subscriber);
        processor_->onThriftRequest(
            std::move(metadata), std::move(request.data), std::move(channel));
      });
}
} // namespace thrift
} // namespace apache
