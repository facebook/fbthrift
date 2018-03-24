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
#include <folly/synchronization/Baton.h>
#include <rsocket/Payload.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSServerThriftChannel.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RequestResponse.h>
#include <yarpl/Flowable.h>
#include <yarpl/flowable/Flowables.h>

namespace apache {
namespace thrift {

class StreamOutput : public RSServerThriftChannel {
 public:
  using Result = std::shared_ptr<yarpl::flowable::Flowable<rsocket::Payload>>;

  StreamOutput(folly::EventBase* evb, int streamId, SubscriberRef subscriber)
      : RSServerThriftChannel(evb),
        streamId_(streamId),
        subscriber_(std::move(subscriber)) {}

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf) noexcept override {
    auto subscriber = std::move(subscriber_);
    subscriber->onSubscribe(yarpl::flowable::Subscription::create());
    subscriber->onNext(rsocket::Payload(
        std::move(buf), RequestResponse::serializeMetadata(*metadata)));
    subscriber->onComplete();
  }

  void sendStreamThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf,
      apache::thrift::SemiStream<std::unique_ptr<folly::IOBuf>>
          stream) noexcept override {
    auto response =
        yarpl::flowable::Flowable<rsocket::Payload>::justOnce(rsocket::Payload(
            std::move(buf), RequestResponse::serializeMetadata(*metadata)));
    auto mappedStream =
        toFlowable(std::move(stream).via(evb_))->map([](auto buf) mutable {
          return rsocket::Payload(std::move(buf));
        });

    // We will not subscribe to the second stream till more than one item is
    // requested from the client side. So we will first send the initial
    // response and wait till the client subscribes to the resultant stream
    response->concatWith(mappedStream)->subscribe(std::move(subscriber_));
  }

 protected:
  int streamId_;
  SubscriberRef subscriber_;
};
} // namespace thrift
} // namespace apache
