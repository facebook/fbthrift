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
#include <thrift/lib/cpp2/transport/rsocket/server/StreamingInput.h>
#include <thrift/lib/cpp2/transport/rsocket/server/StreamingInputOutput.h>
#include <thrift/lib/cpp2/transport/rsocket/server/StreamingOutput.h>

#include <rsocket/internal/ScheduledSubscriber.h>

namespace apache {
namespace thrift {

RSResponder::RSResponder(ThriftProcessor* processor, folly::EventBase* evb)
    : processor_(processor), evb_(evb) {}

yarpl::Reference<yarpl::single::Single<rsocket::Payload>>
RSResponder::handleRequestResponse(
    rsocket::Payload request,
    rsocket::StreamId) {
  // TODO: Where do we use this streamId value -> might make batching possible!
  DCHECK(request.metadata);
  return yarpl::single::Single<rsocket::Payload>::create(
      [this, request = std::move(request)](auto subscriber) mutable {
        VLOG(4) << "RSResponder.handleRequestResponse : " << request;

        auto metadata = RequestResponseThriftChannel::deserializeMetadata(
            *request.metadata);
        DCHECK(metadata->__isset.kind);
        DCHECK(metadata->__isset.seqId);

        auto channel =
            std::make_shared<RequestResponseThriftChannel>(evb_, subscriber);
        processor_->onThriftRequest(
            std::move(metadata), std::move(request.data), channel);

      });
}

void RSResponder::handleFireAndForget(
    rsocket::Payload request,
    rsocket::StreamId) {
  VLOG(3) << "RSResponder.handleFireAndForget : " << request;

  auto metadata =
      RequestResponseThriftChannel::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);

  auto channel = std::make_shared<RSThriftChannelBase>(evb_);
  processor_->onThriftRequest(
      std::move(metadata), std::move(request.data), std::move(channel));
}

RSResponder::FlowableRef RSResponder::handleRequestStream(
    rsocket::Payload request,
    rsocket::StreamId streamId) {
  (void)streamId;
  // TODO This id is internally used by RSocketStateMachine but we need one more
  // id that represents each RPC call - seqId.

  return yarpl::flowable::Flowables::fromPublisher<
             std::unique_ptr<folly::IOBuf>>([this,
                                             request = std::move(request),
                                             streamId](
                                                auto subscriber) mutable {
           auto metadata = RequestResponseThriftChannel::deserializeMetadata(
               *request.metadata);
           DCHECK(metadata->__isset.kind);
           DCHECK(metadata->__isset.seqId);

           auto channel = std::make_shared<StreamingOutput>(
               evb_,
               streamId,
               yarpl::make_ref<
                   rsocket::ScheduledSubscriber<std::unique_ptr<folly::IOBuf>>>(
                   std::move(subscriber), *evb_));
           processor_->onThriftRequest(
               std::move(metadata),
               std::move(request.data),
               std::move(channel));
         })
      ->map([](auto buff) {
        VLOG(3) << "Sending mapped output buffer";
        return rsocket::Payload(std::move(buff));
      });
}

RSResponder::FlowableRef RSResponder::handleRequestChannel(
    rsocket::Payload request,
    FlowableRef requestStream,
    rsocket::StreamId streamId) {
  VLOG(3) << "RSResponder::handleRequestChannel";

  // The execution of the user supplied service function will happen on the
  // worker thread. So we use ScheduledSubscriber to perform the IO operations
  // on the IO thread.

  auto requestStreamFlowable =
      yarpl::flowable::Flowables::fromPublisher<rsocket::Payload>(
          [requestStream = std::move(requestStream), evb = evb_](
              yarpl::Reference<yarpl::flowable::Subscriber<rsocket::Payload>>
                  subscriber) {
            requestStream->subscribe(
                yarpl::make_ref<
                    rsocket::ScheduledSubscriptionSubscriber<rsocket::Payload>>(
                    std::move(subscriber), *evb));
          });

  return yarpl::flowable::Flowables::fromPublisher<
             std::unique_ptr<folly::IOBuf>>([this,
                                             request = std::move(request),
                                             requestStream = std::move(
                                                 requestStreamFlowable),
                                             streamId](
                                                auto subscriber) mutable {
           auto metadata = RequestResponseThriftChannel::deserializeMetadata(
               *request.metadata);
           DCHECK(metadata->__isset.kind);
           DCHECK(metadata->__isset.seqId);

           // TODO - STREAMING_REQUEST_NO_RESPONSE?
           // TODO - STREAMING_REQUEST_SINGLE_RESPONSE?
           CHECK(metadata->kind != RpcKind::STREAMING_REQUEST_SINGLE_RESPONSE);
           CHECK(metadata->kind != RpcKind::STREAMING_REQUEST_NO_RESPONSE);

           auto channel = std::make_shared<StreamingInputOutput>(
               evb_,
               requestStream,
               streamId,
               yarpl::make_ref<
                   rsocket::ScheduledSubscriber<std::unique_ptr<folly::IOBuf>>>(
                   std::move(subscriber), *evb_));
           processor_->onThriftRequest(
               std::move(metadata),
               std::move(request.data),
               std::move(channel));
         })
      ->map([](auto buff) { return rsocket::Payload(std::move(buff)); });
}
} // namespace thrift
} // namespace apache
