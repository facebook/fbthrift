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

// RocketStreamServerCallback
void RocketStreamServerCallback::onStreamRequestN(uint64_t tokens) {
  client_.sendRequestN(streamId_, tokens);
}
void RocketStreamServerCallback::onStreamCancel() {
  client_.cancelStream(streamId_);
}
void RocketStreamServerCallback::onStreamNext(StreamPayload&& payload) {
  clientCallback_.onStreamNext(std::move(payload));
}
void RocketStreamServerCallback::onStreamError(folly::exception_wrapper ew) {
  clientCallback_.onStreamError(std::move(ew));
}
void RocketStreamServerCallback::onStreamComplete() {
  clientCallback_.onStreamComplete();
  client_.freeStream(streamId_);
}

// RocketChannelServerCallback
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
void RocketChannelServerCallback::onStreamNext(StreamPayload&& payload) {
  clientCallback_.onStreamNext(std::move(payload));
}
void RocketChannelServerCallback::onStreamError(folly::exception_wrapper ew) {
  clientCallback_.onStreamError(std::move(ew));
}
void RocketChannelServerCallback::onStreamComplete() {
  clientCallback_.onStreamComplete();
  state_.onStreamComplete();
  if (state_.isComplete()) {
    client_.freeStream(streamId_);
  }
}
void RocketChannelServerCallback::onSinkComplete() {
  client_.sendComplete(streamId_);
  state_.onSinkComplete();
  if (state_.isComplete()) {
    client_.freeStream(streamId_);
  }
}

// RocketSinkServerCallback
void RocketSinkServerCallback::onSinkNext(StreamPayload&& payload) {
  client_.sendPayload(
      streamId_, std::move(payload), rocket::Flags::none().next(true));
}
void RocketSinkServerCallback::onSinkError(folly::exception_wrapper ew) {
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
void RocketSinkServerCallback::onStreamNext(StreamPayload&& payload) {
  if (!receivedFinalResponse_) {
    clientCallback_.onFinalResponse(std::move(payload));
    receivedFinalResponse_ = true;
  } else {
    client_.closeNow(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Sink client shouldn't receive any further payload frame"
            " after the first payload frame."));
  }
}

void RocketSinkServerCallback::onStreamError(folly::exception_wrapper ew) {
  if (!receivedFinalResponse_) {
    clientCallback_.onFinalResponseError(std::move(ew));
  } else {
    client_.closeNow(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Sink client shouldn't receive any further error frame"
            " after the first payload frame."));
  }
}

void RocketSinkServerCallback::onStreamComplete() {
  if (receivedFinalResponse_) {
    client_.freeStream(streamId_);
  } else {
    client_.closeNow(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Didn't received final response's payload before compelete"));
  }
}

void RocketSinkServerCallback::onSinkComplete() {
  client_.sendComplete(streamId_);
}

} // namespace thrift
} // namespace apache
