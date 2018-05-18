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

#include <thrift/lib/cpp2/transport/rsocket/server/RSThriftRequests.h>

#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <yarpl/flowable/Flowable.h>
#include <yarpl/single/SingleSubscriptions.h>

namespace apache {
namespace thrift {

namespace detail {
std::unique_ptr<folly::IOBuf> serializeMetadata(
    const ResponseRpcMetadata& responseMetadata) {
  CompactProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  responseMetadata.write(&writer);
  return queue.move();
}

std::unique_ptr<RequestRpcMetadata> deserializeMetadata(
    const folly::IOBuf& buffer) {
  CompactProtocolReader reader;
  auto metadata = std::make_unique<RequestRpcMetadata>();
  reader.setInput(&buffer);
  metadata->read(&reader);
  return metadata;
}
} // namespace detail

RSOneWayRequest::RSOneWayRequest(
    const apache::thrift::server::ServerConfigs& serverConfigs,
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<Cpp2ConnContext> connContext,
    folly::EventBase* evb,
    folly::Function<void(RSOneWayRequest*)> onDestroy)
    : ThriftRequestCore(
          serverConfigs,
          std::move(metadata),
          std::move(connContext)),
      evb_(evb),
      onDestroy_(std::move(onDestroy)) {
  scheduleTimeouts();
}

RSOneWayRequest::~RSOneWayRequest() {
  if (auto onDestroy = std::move(onDestroy_)) {
    onDestroy(this);
  }
}

void RSOneWayRequest::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata>,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "No response is allowed";
}

void RSOneWayRequest::sendStreamThriftResponse(
    std::unique_ptr<ResponseRpcMetadata>,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept {
  LOG(FATAL) << "Server should not call this function.";
}

folly::EventBase* RSOneWayRequest::getEventBase() noexcept {
  return evb_;
}

void RSOneWayRequest::cancel() {
  ThriftRequestCore::cancel();
  if (auto onDestroy = std::move(onDestroy_)) {
    onDestroy(this);
  }
}

RSSingleRequest::RSSingleRequest(
    const apache::thrift::server::ServerConfigs& serverConfigs,
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<Cpp2ConnContext> connContext,
    folly::EventBase* evb,
    std::shared_ptr<yarpl::single::SingleObserver<rsocket::Payload>>
        singleObserver)
    : ThriftRequestCore(
          serverConfigs,
          std::move(metadata),
          std::move(connContext)),
      evb_(evb),
      singleObserver_(singleObserver) {
  scheduleTimeouts();
}

void RSSingleRequest::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf) noexcept {
  DCHECK(evb_->isInEventBaseThread()) << "Should be called in IO thread";
  DCHECK(metadata);

  singleObserver_->onSubscribe(yarpl::single::SingleSubscriptions::empty());
  singleObserver_->onSuccess(
      rsocket::Payload(std::move(buf), detail::serializeMetadata(*metadata)));
  singleObserver_ = nullptr;
}

void RSSingleRequest::sendStreamThriftResponse(
    std::unique_ptr<ResponseRpcMetadata>,
    std::unique_ptr<folly::IOBuf>,
    apache::thrift::SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept {
  LOG(FATAL) << "Server should not call this function.";
}

folly::EventBase* RSSingleRequest::getEventBase() noexcept {
  return evb_;
}

RSStreamRequest::RSStreamRequest(
    const apache::thrift::server::ServerConfigs& serverConfigs,
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<Cpp2ConnContext> connContext,
    folly::EventBase* evb,
    std::shared_ptr<yarpl::flowable::Subscriber<rsocket::Payload>> subscriber)
    : ThriftRequestCore(
          serverConfigs,
          std::move(metadata),
          std::move(connContext)),
      evb_(evb),
      subscriber_(std::move(subscriber)) {
  scheduleTimeouts();
}

void RSStreamRequest::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata>,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "Server should not call this function.";
}

void RSStreamRequest::sendStreamThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> buf,
    apache::thrift::SemiStream<std::unique_ptr<folly::IOBuf>> stream) noexcept {
  auto response = yarpl::flowable::Flowable<rsocket::Payload>::justOnce(
      rsocket::Payload(std::move(buf), detail::serializeMetadata(*metadata)));
  if (stream) {
    auto mappedStream =
        toFlowable(std::move(stream).via(evb_))->map([](auto buf) mutable {
          return rsocket::Payload(std::move(buf));
        });

    // We will not subscribe to the second stream till more than one item is
    // requested from the client side. So we will first send the initial
    // response and wait till the client subscribes to the resultant stream
    response->concatWith(mappedStream)->subscribe(std::move(subscriber_));
  } else {
    response->subscribe(std::move(subscriber_));
  }
}

folly::EventBase* RSStreamRequest::getEventBase() noexcept {
  return evb_;
}

} // namespace thrift
} // namespace apache
