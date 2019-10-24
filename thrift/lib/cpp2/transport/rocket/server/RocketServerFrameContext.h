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

#pragma once

#include <boost/variant.hpp>
#include <folly/io/async/HHWheelTimer.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>

namespace folly {
class EventBase;
class IOBuf;
} // namespace folly

namespace apache {
namespace thrift {

class RocketSinkClientCallback;
class RocketStreamClientCallback;

namespace rocket {

class RocketException;
class RocketServerConnection;

namespace detail {
class OnPayloadVisitor;
} // namespace detail

class RocketServerFrameContext {
 public:
  RocketServerFrameContext(
      RocketServerConnection& connection,
      StreamId streamId);
  RocketServerFrameContext(RocketServerFrameContext&& other) noexcept;
  RocketServerFrameContext& operator=(RocketServerFrameContext&&) = delete;
  ~RocketServerFrameContext();

  void sendPayload(Payload&& payload, Flags flags);
  void sendError(RocketException&& rex);
  void sendRequestN(int32_t n);
  void sendCancel();

  template <class RequestFrame>
  void onRequestFrame(RequestFrame&& frame) &&;

  folly::EventBase& getEventBase() const;

  StreamId streamId() const {
    return streamId_;
  }

  void scheduleStreamTimeout(folly::HHWheelTimer::Callback* timeoutCallback);
  void freeStream();
  void takeOwnership(RocketStreamClientCallback* callback);
  void takeOwnership(RocketSinkClientCallback* callback);

  RocketServerConnection& connection() {
    DCHECK(connection_);
    return *connection_;
  }

 private:
  friend class detail::OnPayloadVisitor;

  RocketServerConnection* connection_{nullptr};
  const StreamId streamId_;

  void onFullFrame(RequestResponseFrame&& fullFrame) &&;
  void onFullFrame(RequestFnfFrame&& fullFrame) &&;
  void onFullFrame(RequestStreamFrame&& fullFrame) &&;
  void onFullFrame(RequestChannelFrame&& fullFrame) &&;
};

class RocketServerPartialFrameContext {
 public:
  template <class RequestFrame>
  RocketServerPartialFrameContext(
      RocketServerFrameContext&& baseCtx,
      RequestFrame&& frame)
      : mainCtx(std::move(baseCtx)),
        bufferedFragments_(std::forward<RequestFrame>(frame)) {}
  void onPayloadFrame(PayloadFrame&& payloadFrame) &&;

 private:
  RocketServerFrameContext mainCtx;
  boost::variant<
      RequestResponseFrame,
      RequestFnfFrame,
      RequestStreamFrame,
      RequestChannelFrame>
      bufferedFragments_;
};

extern template void RocketServerFrameContext::onRequestFrame<
    RequestResponseFrame>(RequestResponseFrame&&) &&;
extern template void RocketServerFrameContext::onRequestFrame<RequestFnfFrame>(
    RequestFnfFrame&&) &&;
extern template void RocketServerFrameContext::onRequestFrame<
    RequestStreamFrame>(RequestStreamFrame&&) &&;
extern template void RocketServerFrameContext::onRequestFrame<
    RequestChannelFrame>(RequestChannelFrame&&) &&;

} // namespace rocket
} // namespace thrift
} // namespace apache
