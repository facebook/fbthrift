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
  connection_->onContextConstructed();
}

RocketServerFrameContext::RocketServerFrameContext(
    RocketServerFrameContext&& other) noexcept
    : connection_(other.connection_), streamId_(other.streamId_) {
  other.connection_ = nullptr;
}

RocketServerFrameContext::~RocketServerFrameContext() {
  if (connection_) {
    connection_->onContextDestroyed();
  }
}

folly::EventBase& RocketServerFrameContext::getEventBase() const {
  DCHECK(connection_);
  return connection_->getEventBase();
}

void RocketServerFrameContext::sendPayload(Payload&& payload, Flags flags) {
  DCHECK(connection_);
  DCHECK(flags.next() || flags.complete());

  auto buf = PayloadFrame(streamId_, std::move(payload), flags).serialize();
  connection_->send(std::move(buf));
}

void RocketServerFrameContext::sendError(RocketException&& rex) {
  DCHECK(connection_);

  Serializer writer;
  ErrorFrame(streamId_, std::move(rex)).serialize(writer);
  connection_->send(std::move(writer).move());
}

void RocketServerFrameContext::sendRequestN(int32_t n) {
  DCHECK(connection_);

  Serializer writer;
  RequestNFrame(streamId_, n).serialize(writer);
  connection_->send(std::move(writer).move());
}

void RocketServerFrameContext::sendCancel() {
  DCHECK(connection_);

  Serializer writer;
  CancelFrame(streamId_).serialize(writer);
  connection_->send(std::move(writer).move());
}

void RocketServerFrameContext::sendExt(
    Payload&& payload,
    Flags flags,
    ExtFrameType extFrameType) {
  DCHECK(connection_);

  auto buf =
      ExtFrame(streamId_, std::move(payload), flags, extFrameType).serialize();
  connection_->send(std::move(buf));
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
  auto* clientCallback = connection.createStreamClientCallback(
      std::move(*this), fullFrame.initialRequestN());
  frameHandler.handleRequestStreamFrame(std::move(fullFrame), clientCallback);
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
    auto* clientCallback =
        connection.createSinkClientCallback(std::move(*this));
    auto& frameHandler = *connection.frameHandler_;
    frameHandler.handleRequestChannelFrame(
        std::move(fullFrame), clientCallback);
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

void RocketServerFrameContext::scheduleStreamTimeout(
    folly::HHWheelTimer::Callback* timeoutCallback) {
  connection_->scheduleStreamTimeout(timeoutCallback);
}

void RocketServerFrameContext::scheduleSinkTimeout(
    folly::HHWheelTimer::Callback* timeoutCallback,
    std::chrono::milliseconds timeout) {
  connection_->scheduleSinkTimeout(timeoutCallback, timeout);
}

bool RocketServerFrameContext::takeOwnership(
    RocketStreamClientCallback* callback) {
  connection_->streams_.emplace(
      streamId_, std::unique_ptr<RocketStreamClientCallback>(callback));
  // Client may have disconnected before onFirstResponse; in some cases, we need
  // to trigger connection cleanup.
  return !connection_->closeIfNeeded();
}

bool RocketServerFrameContext::takeOwnership(
    RocketSinkClientCallback* callback) {
  connection_->streams_.emplace(
      streamId_, std::unique_ptr<RocketSinkClientCallback>(callback));
  // Client may have disconnected before onFirstResponse; in some cases, we need
  // to trigger connection cleanup.
  return !connection_->closeIfNeeded();
}

void RocketServerFrameContext::freeStream() {
  connection_->freeStream(streamId_);
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
