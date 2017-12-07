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

#pragma once

#include <folly/ExceptionWrapper.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <yarpl/flowable/Subscriber.h>

namespace apache {
namespace thrift {

/**
 * The channel object supports streaming as follows:
 * - Server and client each keep an instance of the channel as duals of
 *   each other. The input in one side is perceived as output in the other
 *   side, and vice versa.
 *
 * - For RPCs with streaming requests, the channel provides "setInput(stream)"
 *   method for the server to register a sink that is to receive these messages.
 *
 * - For RPCs with streaming responses, the operations are reversed.
 *   The server uses the channel's "getOutput()" method to obtain a stream and
 *   to use it for streaming messages.
 */
class StreamThriftChannelBase : public ThriftChannelIf {
 public:
  explicit StreamThriftChannelBase(folly::EventBase* evb) : evb_(evb) {}

  virtual ~StreamThriftChannelBase() = default;

  folly::EventBase* getEventBase() noexcept override {
    return evb_;
  }

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata>,
      std::unique_ptr<folly::IOBuf>) noexcept override {
    LOG(FATAL) << "No response is allowed";
  }

  void sendThriftRequest(
      std::unique_ptr<RequestRpcMetadata>,
      std::unique_ptr<folly::IOBuf>,
      std::unique_ptr<ThriftClientCallback>) noexcept override {
    LOG(FATAL) << "Server should not call this function.";
  }

  virtual void setInput(int32_t, SubscriberRef) noexcept {
    LOG(FATAL) << "Use StreamingInput/StreamingInputOutput";
  }

  virtual SubscriberRef getOutput(int32_t) noexcept {
    LOG(FATAL) << "Use StreamingOutput/StreamingInputOutput";
  }

 protected:
  folly::EventBase* evb_;
};

} // namespace thrift
} // namespace apache
