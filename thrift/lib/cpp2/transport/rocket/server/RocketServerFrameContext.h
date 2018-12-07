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

#pragma once

#include <boost/variant.hpp>

#include <folly/Optional.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>

namespace folly {
class EventBase;
class IOBuf;
} // namespace folly

namespace apache {
namespace thrift {
namespace rocket {

class RocketException;

class RequestResponseFrame;
class RocketServerConnection;

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

  template <class RequestFrame>
  void onRequestFrame(RequestFrame&& frame) &&;
  void onPayloadFrame(PayloadFrame&& frame) &&;

  folly::EventBase& getEventBase() const;

  StreamId streamId() const {
    return streamId_;
  }

  RocketServerConnection& connection() {
    DCHECK(connection_);
    return *connection_;
  }

 private:
  RocketServerConnection* connection_{nullptr};
  const StreamId streamId_;
  folly::Optional<boost::variant<RequestResponseFrame, RequestFnfFrame>>
      bufferedFragments_;

  void onFullFrame() &&;
  class OnFullFrame;
};

class RocketServerFrameContext::OnFullFrame
    : public boost::static_visitor<void> {
 public:
  explicit OnFullFrame(RocketServerFrameContext&& context);

  void operator()(RequestResponseFrame&& fullFrame);
  void operator()(RequestFnfFrame&& fullFrame);

 private:
  RocketServerFrameContext parent_;
};

extern template void RocketServerFrameContext::onRequestFrame<
    RequestResponseFrame>(RequestResponseFrame&&) &&;
extern template void RocketServerFrameContext::onRequestFrame<RequestFnfFrame>(
    RequestFnfFrame&&) &&;

} // namespace rocket
} // namespace thrift
} // namespace apache
