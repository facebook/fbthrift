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
#include <thrift/lib/cpp2/transport/rsocket/server/StreamInput.h>
#include <thrift/lib/cpp2/transport/rsocket/server/StreamOutput.h>
#include <yarpl/Flowable.h>
#include <yarpl/flowable/Flowables.h>

namespace apache {
namespace thrift {

class Channel : public RSServerThriftChannel {
 public:
  Channel(
      folly::EventBase* evb,
      StreamInput::Input input,
      int streamId,
      SubscriberRef subscriber)
      : RSServerThriftChannel(evb), streamId_(streamId) {
    input_ = std::make_unique<StreamInput>(
        evb_, input, streamId, std::make_unique<RSServerThriftChannel>(evb_));
    output_ = std::make_unique<StreamOutput>(evb_, streamId, subscriber);
  }

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf) noexcept override {
    output_->sendThriftResponse(std::move(metadata), std::move(buf));
  }

  void sendStreamThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf,
      apache::thrift::SemiStream<std::unique_ptr<folly::IOBuf>>
          stream) noexcept override {
    output_->sendStreamThriftResponse(
        std::move(metadata), std::move(buf), std::move(stream));
  }

  apache::thrift::Stream<std::unique_ptr<folly::IOBuf>>
  extractStream() noexcept override {
    return input_->extractStream();
  }

 private:
  std::unique_ptr<StreamInput> input_;
  std::unique_ptr<StreamOutput> output_;
  int streamId_;
};
} // namespace thrift
} // namespace apache
