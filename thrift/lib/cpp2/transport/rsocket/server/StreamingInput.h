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

#include <thrift/lib/cpp2/transport/rsocket/server/RSThriftChannelBase.h>

namespace apache {
namespace thrift {

class StreamingInput : public RSThriftChannelBase {
 public:
  using Input = yarpl::Reference<yarpl::flowable::Flowable<rsocket::Payload>>;

  StreamingInput(
      folly::EventBase* evb,
      Input input,
      int streamId,
      std::unique_ptr<RSThriftChannelBase> inner)
      : RSThriftChannelBase(evb),
        input_(input),
        streamId_(streamId),
        inner_(std::move(inner)) {}

  virtual void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf) noexcept override {
    inner_->sendThriftResponse(std::move(metadata), std::move(buf));
  }

  void sendErrorWrapped(
      folly::exception_wrapper ex,
      std::string exCode,
      apache::thrift::MessageChannel::SendCallback* cb =
          nullptr) noexcept override {
    inner_->sendErrorWrapped(ex, exCode, cb);
  }

  void setInput(int32_t, SubscriberRef sink) noexcept override {
    input_->map([](auto payload) { return std::move(payload.data); })
        ->subscribe(std::move(sink));
  }

 private:
  Input input_;
  int streamId_;
  std::unique_ptr<RSThriftChannelBase> inner_;
};
} // namespace thrift
} // namespace apache
