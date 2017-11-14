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

#include <folly/String.h>
#include <folly/futures/Future.h>
#include <folly/io/IOBuf.h>
#include <rsocket/Payload.h>
#include <yarpl/Flowable.h>

#include <yarpl/flowable/Flowables.h>

#include <thrift/lib/cpp2/transport/rsocket/server/StreamingInput.h>
#include <thrift/lib/cpp2/transport/rsocket/server/StreamingInputOutput.h>
#include <thrift/lib/cpp2/transport/rsocket/server/StreamingOutput.h>

namespace apache {
namespace thrift {

// TODO - sending multiple single request&response rpc calls
// over a single stream?

class StreamingInputOutput : public RSThriftChannelBase {
 public:
  StreamingInputOutput(
      folly::EventBase* evb,
      StreamingInput::Input input,
      int streamId,
      SubscriberRef subscriber)
      : RSThriftChannelBase(evb), streamId_(streamId) {
    VLOG(3) << "StreamingInputOutput::ctor";
    input_ = std::make_unique<StreamingInput>(
        evb_, input, streamId, std::make_unique<RSThriftChannelBase>(evb_));
    output_ = std::make_unique<StreamingOutput>(evb_, streamId, subscriber);
  }

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf) noexcept override {
    // This method gets called only in error cases, so we should
    // redirect it to onError of the Subscriber
    output_->sendThriftResponse(std::move(metadata), std::move(buf));
  }

  void sendErrorWrapped(
      folly::exception_wrapper /*ex*/,
      std::string exCode,
      apache::thrift::MessageChannel::SendCallback* /*cb*/ =
          nullptr) noexcept override {
    VLOG(3) << "sendErrorWrapped";
    getOutput(0)->onError(std::runtime_error(exCode));
  }

  void setInput(int32_t seqId, SubscriberRef sink) noexcept override {
    input_->setInput(seqId, sink);
  }

  SubscriberRef getOutput(int32_t seqId) noexcept override {
    return output_->getOutput(seqId);
  }

 private:
  std::unique_ptr<StreamingInput> input_;
  std::unique_ptr<StreamingOutput> output_;
  int streamId_;
};
} // namespace thrift
} // namespace apache
