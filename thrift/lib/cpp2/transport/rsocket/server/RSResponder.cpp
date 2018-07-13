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
#include <thrift/lib/cpp2/transport/rsocket/server/RSThriftRequests.h>

namespace apache {
namespace thrift {

using namespace rsocket;

RSResponder::RSResponder(
    std::shared_ptr<Cpp2Worker> worker,
    const folly::SocketAddress& clientAddress)
    : worker_(std::move(worker)),
      cpp2Processor_(worker_->getServer()->getCpp2Processor()),
      threadManager_(worker_->getServer()->getThreadManager()),
      observer_(worker_->getServer()->getObserver()),
      serverConfigs_(worker_->getServer()),
      clientAddress_(clientAddress) {
  DCHECK(threadManager_);
  DCHECK(cpp2Processor_);
}

std::unique_ptr<Cpp2ConnContext> RSResponder::createConnContext() const {
  return std::make_unique<Cpp2ConnContext>(&clientAddress_);
}

void RSResponder::onThriftRequest(
    std::unique_ptr<ThriftRequestCore> request,
    std::unique_ptr<folly::IOBuf> buf,
    bool invalidMetadata) {
  if (UNLIKELY(invalidMetadata)) {
    LOG(ERROR) << "Invalid metadata object";
    worker_->getEventBase()->runInEventBaseThread([req = std::move(request)]() {
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
      std::move(buf),
      protoId,
      reqContext,
      worker_->getEventBase(),
      threadManager_.get());
}

void RSResponder::handleRequestResponse(
    Payload request,
    StreamId,
    std::shared_ptr<yarpl::single::SingleObserver<Payload>> response) noexcept {
  DCHECK(request.metadata);
  auto metadata = detail::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);

  bool invalidMetadata =
      !(metadata->__isset.protocol && metadata->__isset.name &&
        metadata->__isset.kind && metadata->__isset.seqId);

  auto singleRequest = std::make_unique<RSSingleRequest>(
      *serverConfigs_,
      std::move(metadata),
      createConnContext(),
      worker_->getEventBase(),
      std::move(response));

  onThriftRequest(
      std::move(singleRequest), std::move(request.data), invalidMetadata);
}

void RSResponder::handleFireAndForget(Payload request, StreamId) {
  DCHECK(request.metadata);
  auto metadata = detail::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);

  bool invalidMetadata =
      !(metadata->__isset.protocol && metadata->__isset.name &&
        metadata->__isset.kind && metadata->__isset.seqId);

  auto onewayRequest = std::make_unique<RSOneWayRequest>(
      *serverConfigs_,
      std::move(metadata),
      createConnContext(),
      worker_->getEventBase(),
      [keep = cpp2Processor_](RSOneWayRequest*) {
        // keep the processor as for OneWay requests, even though the client
        // disconnects, which destroys the RSResponder, the oneway request will
        // still execute, which will require cpp2Processor_ to be alive
        // @see AsyncProcessor::processInThread function for more details
        // D1006482 for furhter details.
      });

  onThriftRequest(
      std::move(onewayRequest), std::move(request.data), invalidMetadata);
}

void RSResponder::handleRequestStream(
    Payload request,
    StreamId,
    std::shared_ptr<yarpl::flowable::Subscriber<Payload>> response) noexcept {
  DCHECK(request.metadata);
  auto metadata = detail::deserializeMetadata(*request.metadata);
  DCHECK(metadata->__isset.kind);
  DCHECK(metadata->__isset.seqId);
  request.metadata.reset();

  bool invalidMetadata =
      !(metadata->__isset.protocol && metadata->__isset.name &&
        metadata->__isset.kind && metadata->__isset.seqId);

  auto streamRequest = std::make_unique<RSStreamRequest>(
      *serverConfigs_,
      std::move(metadata),
      createConnContext(),
      worker_->getEventBase(),
      std::move(response));

  onThriftRequest(
      std::move(streamRequest), std::move(request.data), invalidMetadata);
}

} // namespace thrift
} // namespace apache
