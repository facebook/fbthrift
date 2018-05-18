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

#include <rsocket/internal/ScheduledSubscriber.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RequestResponse.h>
#include <thrift/lib/cpp2/transport/rsocket/server/StreamOutput.h>

namespace apache {
namespace thrift {

using namespace rsocket;

RSResponder::RSResponder(std::shared_ptr<Cpp2Worker> worker)
    : worker_(std::move(worker)),
      cpp2Processor_(worker_->getServer()->getCpp2Processor()),
      threadManager_(worker_->getServer()->getThreadManager()),
      observer_(worker_->getServer()->getObserver()),
      serverConfigs_(worker_->getServer()) {
  DCHECK(threadManager_);
}

void RSResponder::onThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload,
    std::shared_ptr<ThriftChannelIf> channel,
    std::unique_ptr<Cpp2ConnContext> connContext) noexcept {
  DCHECK(metadata);
  DCHECK(payload);
  DCHECK(channel);
  DCHECK(cpp2Processor_);

  bool invalidMetadata =
      !(metadata->__isset.protocol && metadata->__isset.name &&
        metadata->__isset.kind && metadata->__isset.seqId);

  auto request = std::make_unique<ThriftRequest>(
      *serverConfigs_,
      channel,
      std::move(metadata),
      std::move(connContext),
      [keep = cpp2Processor_](ThriftRequest*) {
        // keep the processor as for OneWay requests, even though the client
        // disconnects, which destroys the RSResponder, the oneway request will
        // still execute, which will require cpp2Processor_ to be alive
        // @see AsyncProcessor::processInThread function for more details
        // D1006482 for furhter details.
      });

  auto evb = channel->getEventBase();
  if (UNLIKELY(invalidMetadata)) {
    LOG(ERROR) << "Invalid metadata object";
    evb->runInEventBaseThread([req = std::move(request)]() {
      req->sendErrorWrapped(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::UNSUPPORTED_CLIENT_TYPE,
              "invalid metadata object"),
          "corrupted metadata");
    });
    return;
  }

  auto protoId = request->getProtoId();
  auto reqContext = request->getRequestContext();
  cpp2Processor_->process(
      std::move(request),
      std::move(payload),
      protoId,
      reqContext,
      evb,
      threadManager_.get());
}

void RSResponder::handleRequestResponse(
    Payload request,
    StreamId,
    std::shared_ptr<yarpl::single::SingleObserver<Payload>> response) noexcept {
  DCHECK(request.metadata);
  auto metadata = RequestResponse::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);

  auto channel = std::make_shared<RequestResponse>(
      worker_->getEventBase(), std::move(response));
  onThriftRequest(std::move(metadata), std::move(request.data), channel);
}

void RSResponder::handleFireAndForget(Payload request, StreamId) {
  auto metadata = RequestResponse::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);

  auto channel =
      std::make_shared<RSServerThriftChannel>(worker_->getEventBase());
  onThriftRequest(
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

  auto channel = std::make_shared<StreamOutput>(
      worker_->getEventBase(), streamId, std::move(response));
  onThriftRequest(
      std::move(metadata), std::move(request.data), std::move(channel));
}

} // namespace thrift
} // namespace apache
