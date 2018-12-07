/*
 * Copyright 2018-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

#include <utility>

#include <folly/Likely.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Serializer.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>

namespace apache {
namespace thrift {
namespace rocket {

namespace {
class FragmentAppender : public boost::static_visitor<void> {
 public:
  explicit FragmentAppender(PayloadFrame&& payloadFrame)
      : payloadFrame_(std::move(payloadFrame)) {}

  void operator()(RequestResponseFrame& requestFrame) {
    appendPayload(requestFrame);
  }
  void operator()(RequestFnfFrame& requestFrame) {
    appendPayload(requestFrame);
  }
  void operator()(RequestStreamFrame& requestFrame) {
    appendPayload(requestFrame);
  }

 private:
  PayloadFrame payloadFrame_;

  template <class Frame>
  void appendPayload(Frame& requestFrame) {
    requestFrame.payload().append(std::move(payloadFrame_.payload()));
  }
};
} // namespace

RocketServerFrameContext::RocketServerFrameContext(
    RocketServerConnection& connection,
    StreamId streamId)
    : connection_(&connection), streamId_(streamId) {
  connection_->onContextConstructed();
}

RocketServerFrameContext::RocketServerFrameContext(
    RocketServerFrameContext&& other) noexcept
    : connection_(other.connection_),
      streamId_(other.streamId_),
      bufferedFragments_(std::move(other.bufferedFragments_)) {
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

  Serializer writer;
  PayloadFrame(streamId_, std::move(payload), flags).serialize(writer);
  connection_->send(std::move(writer).move());
}

void RocketServerFrameContext::sendError(RocketException&& rex) {
  DCHECK(connection_);

  Serializer writer;
  ErrorFrame(streamId_, std::move(rex)).serialize(writer);
  connection_->send(std::move(writer).move());
}

void RocketServerFrameContext::onPayloadFrame(PayloadFrame&& payloadFrame) && {
  // Until RequestChannel is supported, client may not send non-fragment payload
  // frames to server
  DCHECK(bufferedFragments_);
  const bool hasFollows = payloadFrame.hasFollows();

  FragmentAppender fragmentAppender{std::move(payloadFrame)};
  bufferedFragments_->apply_visitor(fragmentAppender);

  if (!hasFollows) {
    std::move(*this).onFullFrame();
  }
}

template <class RequestFrame>
void RocketServerFrameContext::onRequestFrame(RequestFrame&& frame) && {
  // Request* frames may only be the first in a sequence of fragments.
  DCHECK(!bufferedFragments_);
  const bool hasFollows = frame.hasFollows();
  bufferedFragments_.emplace(std::forward<RequestFrame>(frame));

  if (UNLIKELY(hasFollows)) {
    connection_->partialFrames_.emplace(streamId_, std::move(*this));
    return;
  }
  std::move(*this).onFullFrame();
}

void RocketServerFrameContext::onFullFrame() && {
  auto fullFrame = std::move(bufferedFragments_);
  OnFullFrame onFullFrame{std::move(*this)};
  std::move(*fullFrame).apply_visitor(onFullFrame);
}

// Implementation of nested OnFullFrame visitor
RocketServerFrameContext::OnFullFrame::OnFullFrame(
    RocketServerFrameContext&& context)
    : parent_(std::move(context)) {}

void RocketServerFrameContext::OnFullFrame::operator()(
    RequestResponseFrame&& fullFrame) {
  fullFrame.setHasFollows(false);
  auto& frameHandler = *parent_.connection_->frameHandler_;
  frameHandler.handleRequestResponseFrame(
      std::move(fullFrame), std::move(parent_));
}

void RocketServerFrameContext::OnFullFrame::operator()(
    RequestFnfFrame&& fullFrame) {
  fullFrame.setHasFollows(false);
  auto& frameHandler = *parent_.connection_->frameHandler_;
  frameHandler.handleRequestFnfFrame(std::move(fullFrame), std::move(parent_));
}

void RocketServerFrameContext::OnFullFrame::operator()(
    RequestStreamFrame&& fullFrame) {
  fullFrame.setHasFollows(false);
  auto& connection = *parent_.connection_;
  auto& frameHandler = *connection.frameHandler_;
  auto subscriber = connection.createStreamSubscriber(
      std::move(parent_), fullFrame.initialRequestN());
  frameHandler.handleRequestStreamFrame(
      std::move(fullFrame), std::move(subscriber));
}

// Explicit function template instantiations
template void RocketServerFrameContext::onRequestFrame<RequestResponseFrame>(
    RequestResponseFrame&&) &&;
template void RocketServerFrameContext::onRequestFrame<RequestFnfFrame>(
    RequestFnfFrame&&) &&;
template void RocketServerFrameContext::onRequestFrame<RequestStreamFrame>(
    RequestStreamFrame&&) &&;

} // namespace rocket
} // namespace thrift
} // namespace apache
