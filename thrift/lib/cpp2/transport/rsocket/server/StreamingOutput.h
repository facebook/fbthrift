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
#include <yarpl/Flowable.h>

#include <yarpl/flowable/Flowables.h>

#include <thrift/lib/cpp2/transport/rsocket/server/RSThriftChannelBase.h>

namespace apache {
namespace thrift {

class StreamingOutput : public RSThriftChannelBase {
 public:
  using Result = yarpl::Reference<yarpl::flowable::Flowable<rsocket::Payload>>;

  StreamingOutput(folly::EventBase* evb, int streamId, SubscriberRef subscriber)
      : RSThriftChannelBase(evb),
        streamId_(streamId),
        subscriber_(std::move(subscriber)) {}

  void sendThriftResponse(
      std::unique_ptr<ResponseRpcMetadata> metadata,
      std::unique_ptr<folly::IOBuf> buf) noexcept override {
    // This method gets called only in error cases, so we should
    // redirect it to onError of the Subscriber
    VLOG(3) << "sendThriftResponse: "
            << buf->cloneCoalescedAsValue().moveToFbString().toStdString();

    auto subscriber = getOutput(metadata->seqId);
    subscriber->onSubscribe(yarpl::flowable::Subscription::empty());
    auto str = folly::humanify(buf->cloneCoalescedAsValue().moveToFbString())
                   .toStdString();
    subscriber->onError(std::runtime_error(str));
  }

  SubscriberRef getOutput(int32_t) noexcept override {
    return subscriber_;
  }

 protected:
  int streamId_;
  SubscriberRef subscriber_;
};
} // namespace thrift
} // namespace apache
