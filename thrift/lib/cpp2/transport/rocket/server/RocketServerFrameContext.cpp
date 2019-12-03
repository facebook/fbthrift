/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

#include <utility>

#include <folly/Likely.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Serializer.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketSinkClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.h>

namespace apache {
namespace thrift {
namespace rocket {

RocketServerFrameContext::RocketServerFrameContext(
    RocketServerConnection& connection,
    StreamId streamId)
    : connection_(&connection), streamId_(streamId) {
  connection_->incInflightRequests();
}

RocketServerFrameContext::RocketServerFrameContext(
    RocketServerFrameContext&& other) noexcept
    : connection_(other.connection_), streamId_(other.streamId_) {
  other.connection_ = nullptr;
}

RocketServerFrameContext::~RocketServerFrameContext() {
  if (connection_) {
    connection_->decInflightRequests();
  }
}

folly::EventBase& RocketServerFrameContext::getEventBase() const {
  DCHECK(connection_);
  return connection_->getEventBase();
}

void RocketServerFrameContext::sendPayload(Payload&& payload, Flags flags) {
  DCHECK(connection_);
  DCHECK(flags.next() || flags.complete());
  connection_->sendPayload(streamId_, std::move(payload), flags);
}

void RocketServerFrameContext::sendError(RocketException&& rex) {
  DCHECK(connection_);
  connection_->sendError(streamId_, std::move(rex));
}

void RocketServerFrameContext::onFullFrame(
    RequestResponseFrame&& fullFrame) && {
  auto& frameHandler = *connection_->frameHandler_;
  frameHandler.handleRequestResponseFrame(
      std::move(fullFrame), std::move(*this));
}

void RocketServerFrameContext::onFullFrame(RequestFnfFrame&& fullFrame) && {
  auto& frameHandler = *connection_->frameHandler_;
  frameHandler.handleRequestFnfFrame(std::move(fullFrame), std::move(*this));
}

void RocketServerFrameContext::onFullFrame(RequestStreamFrame&& fullFrame) && {
  auto& connection = *connection_;
  auto& frameHandler = *connection.frameHandler_;
  auto& clientCallback = connection.createStreamClientCallback(
      streamId_,
      *std::exchange(connection_, nullptr),
      fullFrame.initialRequestN());
  frameHandler.handleRequestStreamFrame(std::move(fullFrame), &clientCallback);
}

void RocketServerFrameContext::onFullFrame(RequestChannelFrame&& fullFrame) && {
  auto& connection = *connection_;
  if (fullFrame.initialRequestN() != 2) {
    connection.close(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "initialRequestN of Sink must be 2"));
  } else {
    auto& clientCallback = connection.createSinkClientCallback(
        streamId_, *std::exchange(connection_, nullptr));
    auto& frameHandler = *connection.frameHandler_;
    frameHandler.handleRequestChannelFrame(
        std::move(fullFrame), &clientCallback);
  }
}

template <class RequestFrame>
void RocketServerFrameContext::onRequestFrame(RequestFrame&& frame) && {
  if (UNLIKELY(frame.hasFollows())) {
    auto streamId = streamId_;
    auto& connection = *connection_;
    connection.partialRequestFrames_.emplace(
        streamId,
        RocketServerPartialFrameContext(
            std::move(*this), std::forward<RequestFrame>(frame)));
    return;
  }
  std::move(*this).onFullFrame(std::forward<RequestFrame>(frame));
}

namespace detail {
class OnPayloadVisitor : public boost::static_visitor<void> {
 public:
  OnPayloadVisitor(PayloadFrame&& payloadFrame, RocketServerFrameContext& ctx)
      : payloadFrame_(std::move(payloadFrame)), ctx_(ctx) {}

  void operator()(RequestResponseFrame& requestFrame) {
    handleNext(requestFrame);
  }
  void operator()(RequestFnfFrame& requestFrame) {
    handleNext(requestFrame);
  }
  void operator()(RequestStreamFrame& requestFrame) {
    handleNext(requestFrame);
  }
  void operator()(RequestChannelFrame& requestFrame) {
    handleNext(requestFrame);
  }

 private:
  PayloadFrame payloadFrame_;
  RocketServerFrameContext& ctx_;

  template <class Frame>
  void handleNext(Frame& requestFrame) {
    const bool hasFollows = payloadFrame_.hasFollows();
    requestFrame.payload().append(std::move(payloadFrame_.payload()));

    if (!hasFollows) {
      requestFrame.setHasFollows(false);
      std::move(ctx_).onFullFrame(std::move(requestFrame));
    }
  }
};
} // namespace detail

void RocketServerPartialFrameContext::onPayloadFrame(
    PayloadFrame&& payloadFrame) && {
  detail::OnPayloadVisitor visitor(std::move(payloadFrame), mainCtx);
  bufferedFragments_.apply_visitor(visitor);
}

// Explicit function template instantiations
template void RocketServerFrameContext::onRequestFrame<RequestResponseFrame>(
    RequestResponseFrame&&) &&;
template void RocketServerFrameContext::onRequestFrame<RequestFnfFrame>(
    RequestFnfFrame&&) &&;
template void RocketServerFrameContext::onRequestFrame<RequestStreamFrame>(
    RequestStreamFrame&&) &&;
template void RocketServerFrameContext::onRequestFrame<RequestChannelFrame>(
    RequestChannelFrame&&) &&;

} // namespace rocket
} // namespace thrift
} // namespace apache
