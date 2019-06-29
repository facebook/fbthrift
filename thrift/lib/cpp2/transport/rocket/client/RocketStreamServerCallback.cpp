/*
 * Copyright 2019-present Facebook, Inc.
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
#include <thrift/lib/cpp2/transport/rocket/client/RocketStreamServerCallback.h>

#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>

namespace apache {
namespace thrift {

void RocketStreamServerCallback::onStreamRequestN(uint64_t tokens) {
  client_.sendRequestN(streamId_, tokens);
}
void RocketStreamServerCallback::onStreamCancel() {
  client_.cancelStream(streamId_);
}
void RocketStreamServerCallback::onStreamComplete() {
  clientCallback_.onStreamComplete();
  client_.freeStream(streamId_);
}

void RocketChannelServerCallback::onStreamRequestN(uint64_t tokens) {
  client_.sendRequestN(streamId_, tokens);
}
void RocketChannelServerCallback::onStreamCancel() {
  client_.cancelStream(streamId_);
}

void RocketChannelServerCallback::onSinkNext(StreamPayload&& payload) {
  client_.sendPayload(
      streamId_, std::move(payload), rocket::Flags::none().next(true));
}

void RocketChannelServerCallback::onSinkError(folly::exception_wrapper ew) {
  if (!ew.with_exception<rocket::RocketException>([this](auto&& rex) {
        client_.sendError(
            streamId_,
            rocket::RocketException(
                rocket::ErrorCode::APPLICATION_ERROR, rex.moveErrorData()));
      })) {
    client_.sendError(
        streamId_,
        rocket::RocketException(
            rocket::ErrorCode::APPLICATION_ERROR, ew.what()));
  }
}

void RocketChannelServerCallback::onSinkComplete() {
  client_.sendComplete(streamId_);
  state_.onSinkComplete();
  if (state_.isComplete()) {
    client_.freeStream(streamId_);
  }
}

void RocketChannelServerCallback::onStreamComplete() {
  clientCallback_.onStreamComplete();
  state_.onStreamComplete();
  if (state_.isComplete()) {
    client_.freeStream(streamId_);
  }
}

} // namespace thrift
} // namespace apache
