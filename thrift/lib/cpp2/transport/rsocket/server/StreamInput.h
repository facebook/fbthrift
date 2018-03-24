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
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSServerThriftChannel.h>
#include <yarpl/Flowable.h>
#include <yarpl/flowable/Flowables.h>

namespace apache {
namespace thrift {

class StreamInput : public RSServerThriftChannel {
 public:
  using Input = std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>>;

  StreamInput(
      folly::EventBase* evb,
      Input input,
      int streamId,
      std::unique_ptr<RSServerThriftChannel> inner)
      : RSServerThriftChannel(evb),
        input_(input),
        streamId_(streamId),
        inner_(std::move(inner)) {}

  virtual void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf) noexcept override {
    inner_->sendThriftResponse(std::move(metadata), std::move(buf));
  }

  apache::thrift::Stream<std::unique_ptr<folly::IOBuf>>
  extractStream() noexcept override {
    return toStream(
        input_->map([](auto payload) { return std::move(payload.data); }),
        evb_);
  }

 private:
  Input input_;
  int streamId_;
  std::unique_ptr<RSServerThriftChannel> inner_;
};
} // namespace thrift
} // namespace apache
