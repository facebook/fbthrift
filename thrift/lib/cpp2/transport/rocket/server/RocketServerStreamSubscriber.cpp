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

#include <thrift/lib/cpp2/transport/rocket/server/RocketServerStreamSubscriber.h>

#include <memory>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/Range.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

#include <yarpl/flowable/Subscription.h>

namespace apache {
namespace thrift {
namespace rocket {

RocketServerStreamSubscriber::RocketServerStreamSubscriber(
    RocketServerFrameContext&& context,
    uint32_t initialRequestN)
    : context_(std::make_unique<RocketServerFrameContext>(std::move(context))),
      initialRequestN_(initialRequestN) {}

RocketServerStreamSubscriber::~RocketServerStreamSubscriber() {
  if (auto subscription = std::move(subscription_)) {
    subscription->cancel();
  }
}

void RocketServerStreamSubscriber::onSubscribe(
    std::shared_ptr<yarpl::flowable::Subscription> subscription) {
  if (canceledBeforeSubscribed_) {
    return subscription->cancel();
  }

  subscription_ = std::move(subscription);
  if (initialRequestN_ != 0) {
    subscription_->request(initialRequestN_);
  }
}

void RocketServerStreamSubscriber::onNext(Payload payload) {
  context_->sendPayload(std::move(payload), Flags::none().next(true));
}

void RocketServerStreamSubscriber::onComplete() {
  context_->sendPayload(
      Payload::makeFromData(std::unique_ptr<folly::IOBuf>{}),
      Flags::none().complete(true));
}

void RocketServerStreamSubscriber::onError(folly::exception_wrapper ew) {
  if (!ew.with_exception<RocketException>([this](auto&& rex) {
        context_->sendError(
            RocketException(ErrorCode::APPLICATION_ERROR, rex.moveErrorData()));
      })) {
    context_->sendError(
        RocketException(ErrorCode::APPLICATION_ERROR, ew.what()));
  }
}

void RocketServerStreamSubscriber::request(uint32_t n) {
  if (subscription_) {
    subscription_->request(n);
  } else {
    initialRequestN_ += n;
  }
}

void RocketServerStreamSubscriber::cancel() {
  if (auto subscription = std::move(subscription_)) {
    subscription->cancel();
    context_.reset();
  } else {
    canceledBeforeSubscribed_ = true;
  }
}

} // namespace rocket
} // namespace thrift
} // namespace apache
