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

#include <thrift/lib/cpp2/transport/rocket/client/RocketClientFlowable.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include <glog/logging.h>

#include <folly/Likely.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>

namespace apache {
namespace thrift {
namespace rocket {

void RocketClientFlowable::RocketClientSubscription::request(int64_t n) {
  if (UNLIKELY(n <= 0)) {
    return;
  }

  // Clamp if necessary. rsocket protocol requires n <= 2^31 - 1.
  n = std::min<int64_t>(n, std::numeric_limits<int32_t>::max());
  if (client_) {
    client_->sendRequestN(streamId_, static_cast<int32_t>(n));
  }
}

void RocketClientFlowable::RocketClientSubscription::cancel() {
  if (auto* client = std::exchange(client_, nullptr)) {
    client->cancelStream(streamId_);
  }
}

void RocketClientFlowable::onPayloadFrame(PayloadFrame&& frame) {
  if (frame.hasFollows()) {
    if (!bufferedFragments_) {
      bufferedFragments_ =
          std::make_unique<Payload>(std::move(frame.payload()));
    } else {
      bufferedFragments_->append(std::move(frame.payload()));
    }
    return;
  }

  // Note that if the payload frame arrives in fragments, we rely on the last
  // fragment having the right next and/or complete flags set.
  if (frame.hasNext()) {
    if (bufferedFragments_) {
      bufferedFragments_->append(std::move(frame.payload()));
      onNext(std::move(*bufferedFragments_));
      bufferedFragments_.reset();
    } else {
      onNext(std::move(frame.payload()));
    }
  }

  if (frame.hasComplete()) {
    onComplete();
  }
}

void RocketClientFlowable::onErrorFrame(ErrorFrame&& frame) {
  bufferedFragments_.reset();

  onError(folly::make_exception_wrapper<RocketException>(
      frame.errorCode(), std::move(frame.payload()).data()));
}

} // namespace rocket
} // namespace thrift
} // namespace apache
