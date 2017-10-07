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

void RSResponder::handleFireAndForget(
    rsocket::Payload request,
    rsocket::StreamId streamId) {
  VLOG(3) << "RSResponder.handleFireAndForget : " << request;

  auto metadata = std::make_unique<RequestRpcMetadata>();
  metadata->seqId = streamId;
  metadata->__isset.seqId = true;
  metadata->kind = RpcKind::SINGLE_REQUEST_NO_RESPONSE;
  metadata->__isset.kind = true;

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
             std::unique_ptr<folly::IOBuf>>(
             [this, request = std::move(request), streamId](
                 auto subscriber) mutable {
               VLOG(3) << "PointResponder.handleRequestStream : " << request
                       << ", streamId: " << streamId;

               auto metadata = std::make_unique<RequestRpcMetadata>();
               metadata->seqId = streamId;
               metadata->__isset.seqId = true;
               metadata->kind = RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE;
               metadata->__isset.kind = true;

               auto channel = std::make_shared<StreamingOutput>(
                   evb_, streamId, subscriber);
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
  return yarpl::flowable::Flowables::fromPublisher<
             std::unique_ptr<folly::IOBuf>>(
             [this,
              request = std::move(request),
              requestStream = std::move(requestStream),
              streamId](auto subscriber) mutable {
               VLOG(3) << "PointResponder.handleRequestChannel : " << request
                       << ", streamId: " << streamId;

               auto metadata = std::make_unique<RequestRpcMetadata>();
               metadata->seqId = streamId;
               metadata->__isset.seqId = true;
               metadata->kind = RpcKind::STREAMING_REQUEST_STREAMING_RESPONSE;
               metadata->__isset.kind = true;

               // TODO - STREAMING_REQUEST_NO_RESPONSE?
               // TODO - STREAMING_REQUEST_SINGLE_RESPONSE?

               auto channel = std::make_shared<StreamingInputOutput>(
                   evb_, requestStream, streamId, subscriber);
               processor_->onThriftRequest(
                   std::move(metadata),
                   std::move(request.data),
                   std::move(channel));
             })
      ->map([](auto buff) { return rsocket::Payload(std::move(buff)); });
}
} // namespace thrift
} // namespace apache
