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
namespace rocket {

namespace {
template <typename ServerCallback>
class TimeoutCallback : public folly::HHWheelTimer::Callback {
 public:
  explicit TimeoutCallback(ServerCallback& parent) : parent_(parent) {}
  void timeoutExpired() noexcept override {
    parent_.timeoutExpired();
  }

 private:
  ServerCallback& parent_;
};
} // namespace

// RocketStreamServerCallback
void RocketStreamServerCallback::onStreamRequestN(uint64_t tokens) {
  if (credits_ == 0) {
    scheduleTimeout();
  }
  credits_ += tokens;
  client_.sendRequestN(streamId_, tokens);
}
void RocketStreamServerCallback::onStreamCancel() {
  client_.cancelStream(streamId_);
}

void RocketStreamServerCallback::onInitialPayload(
    FirstResponsePayload&& payload,
    folly::EventBase* evb) {
  if (credits_ > 0) {
    scheduleTimeout();
  }
  clientCallback_->onFirstResponse(std::move(payload), evb, this);
}
void RocketStreamServerCallback::onInitialError(folly::exception_wrapper ew) {
  clientCallback_->onFirstResponseError(std::move(ew));
}
void RocketStreamServerCallback::onStreamTransportError(
    folly::exception_wrapper ew) {
  clientCallback_->onStreamError(std::move(ew));
}
StreamChannelStatus RocketStreamServerCallback::onStreamPayload(
    StreamPayload&& payload) {
  DCHECK(credits_ != 0);
  if (--credits_ != 0) {
    scheduleTimeout();
  } else {
    cancelTimeout();
  }
  clientCallback_->onStreamNext(std::move(payload));
  return StreamChannelStatus::Alive;
}
StreamChannelStatus RocketStreamServerCallback::onStreamFinalPayload(
    StreamPayload&& payload) {
  auto& client = client_;
  auto streamId = streamId_;
  onStreamPayload(std::move(payload));
  // It's possible that stream was canceled from the client callback. This
  // object may be already destroyed.
  if (client.streamExists(streamId)) {
    return onStreamComplete();
  }
  return StreamChannelStatus::Alive;
}
StreamChannelStatus RocketStreamServerCallback::onStreamComplete() {
  clientCallback_->onStreamComplete();
  return StreamChannelStatus::Complete;
}
StreamChannelStatus RocketStreamServerCallback::onStreamError(
    folly::exception_wrapper ew) {
  clientCallback_->onStreamError(std::move(ew));
  return StreamChannelStatus::Complete;
}
StreamChannelStatus RocketStreamServerCallback::onSinkRequestN(uint64_t) {
  clientCallback_->onStreamError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              STREAMING_CONTRACT_VIOLATION,
          "onSinkRequestN called for a stream"));
  return StreamChannelStatus::ContractViolation;
}
StreamChannelStatus RocketStreamServerCallback::onSinkCancel() {
  clientCallback_->onStreamError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              STREAMING_CONTRACT_VIOLATION,
          "onSinkCancel called for a stream"));
  return StreamChannelStatus::ContractViolation;
}
void RocketStreamServerCallback::timeoutExpired() noexcept {
  clientCallback_->onStreamError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::TIMED_OUT,
          "stream chunk timeout"));
  onStreamCancel();
}
void RocketStreamServerCallback::scheduleTimeout() {
  if (chunkTimeout_ == std::chrono::milliseconds::zero()) {
    return;
  }
  if (!timeout_) {
    timeout_ =
        std::make_unique<TimeoutCallback<RocketStreamServerCallback>>(*this);
  }
  client_.scheduleTimeout(timeout_.get(), chunkTimeout_);
}
void RocketStreamServerCallback::cancelTimeout() {
  timeout_.reset();
}

// RocketChannelServerCallback
void RocketChannelServerCallback::onStreamRequestN(uint64_t tokens) {
  DCHECK(state_ == State::BothOpen || state_ == State::StreamOpen);
  client_.sendRequestN(streamId_, tokens);
}
void RocketChannelServerCallback::onStreamCancel() {
  DCHECK(state_ == State::BothOpen || state_ == State::StreamOpen);
  client_.cancelStream(streamId_);
}
void RocketChannelServerCallback::onSinkNext(StreamPayload&& payload) {
  DCHECK(state_ == State::BothOpen || state_ == State::SinkOpen);
  client_.sendPayload(
      streamId_, std::move(payload), rocket::Flags::none().next(true));
}
void RocketChannelServerCallback::onSinkError(folly::exception_wrapper ew) {
  DCHECK(state_ == State::BothOpen || state_ == State::SinkOpen);
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
  DCHECK(state_ == State::BothOpen || state_ == State::SinkOpen);
  if (state_ == State::BothOpen) {
    client_.sendComplete(streamId_, false);
    state_ = State::StreamOpen;
    return;
  }
  client_.sendComplete(streamId_, true);
}

void RocketChannelServerCallback::onInitialPayload(
    FirstResponsePayload&& payload,
    folly::EventBase* evb) {
  clientCallback_.onFirstResponse(std::move(payload), evb, this);
}
void RocketChannelServerCallback::onInitialError(folly::exception_wrapper ew) {
  clientCallback_.onFirstResponseError(std::move(ew));
}
void RocketChannelServerCallback::onStreamTransportError(
    folly::exception_wrapper ew) {
  switch (state_) {
    case State::BothOpen:
    case State::StreamOpen:
      clientCallback_.onStreamError(std::move(ew));
      return;
    case State::SinkOpen:
      clientCallback_.onSinkCancel();
      return;
    default:
      folly::assume_unreachable();
  };
}
StreamChannelStatus RocketChannelServerCallback::onStreamPayload(
    StreamPayload&& payload) {
  switch (state_) {
    case State::BothOpen:
    case State::StreamOpen:
      clientCallback_.onStreamNext(std::move(payload));
      return StreamChannelStatus::Alive;
    case State::SinkOpen:
      clientCallback_.onSinkCancel();
      return StreamChannelStatus::ContractViolation;
    default:
      folly::assume_unreachable();
  }
}
StreamChannelStatus RocketChannelServerCallback::onStreamFinalPayload(
    StreamPayload&& payload) {
  auto& client = client_;
  auto streamId = streamId_;
  onStreamPayload(std::move(payload));
  // It's possible that stream was canceled from the client callback. This
  // object may be already destroyed.
  if (client.streamExists(streamId)) {
    return onStreamComplete();
  }
  return StreamChannelStatus::Alive;
}
StreamChannelStatus RocketChannelServerCallback::onStreamComplete() {
  switch (state_) {
    case State::BothOpen:
      state_ = State::SinkOpen;
      clientCallback_.onStreamComplete();
      return StreamChannelStatus::Alive;
    case State::StreamOpen:
      clientCallback_.onStreamComplete();
      return StreamChannelStatus::Complete;
    case State::SinkOpen:
      clientCallback_.onSinkCancel();
      return StreamChannelStatus::ContractViolation;
    default:
      folly::assume_unreachable();
  }
}
StreamChannelStatus RocketChannelServerCallback::onStreamError(
    folly::exception_wrapper ew) {
  clientCallback_.onStreamError(std::move(ew));
  return StreamChannelStatus::Complete;
}
StreamChannelStatus RocketChannelServerCallback::onSinkRequestN(
    uint64_t tokens) {
  switch (state_) {
    case State::BothOpen:
    case State::SinkOpen:
      clientCallback_.onSinkRequestN(tokens);
      return StreamChannelStatus::Alive;
    case State::StreamOpen:
      return StreamChannelStatus::Alive;
    default:
      folly::assume_unreachable();
  }
}
StreamChannelStatus RocketChannelServerCallback::onSinkCancel() {
  switch (state_) {
    case State::BothOpen:
    case State::SinkOpen:
      clientCallback_.onSinkCancel();
      return StreamChannelStatus::Complete;
    case State::StreamOpen:
      clientCallback_.onStreamError(folly::make_exception_wrapper<
                                    transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::INTERRUPTED,
          "channel closed via onSinkCancel"));
      return StreamChannelStatus::Complete;
    default:
      folly::assume_unreachable();
  }
}

// RocketSinkServerCallback
void RocketSinkServerCallback::onSinkNext(StreamPayload&& payload) {
  DCHECK(state_ == State::BothOpen);
  client_.sendPayload(
      streamId_, std::move(payload), rocket::Flags::none().next(true));
}
void RocketSinkServerCallback::onSinkError(folly::exception_wrapper ew) {
  DCHECK(state_ == State::BothOpen);
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
void RocketSinkServerCallback::onSinkComplete() {
  DCHECK(state_ == State::BothOpen);
  state_ = State::StreamOpen;
  client_.sendComplete(streamId_, false);
}

void RocketSinkServerCallback::onStreamCancel() {
  DCHECK(state_ == State::BothOpen || state_ == State::StreamOpen);
  client_.cancelStream(streamId_);
}

void RocketSinkServerCallback::onInitialPayload(
    FirstResponsePayload&& payload,
    folly::EventBase* evb) {
  clientCallback_.onFirstResponse(std::move(payload), evb, this);
}
void RocketSinkServerCallback::onInitialError(folly::exception_wrapper ew) {
  clientCallback_.onFirstResponseError(std::move(ew));
}
void RocketSinkServerCallback::onStreamTransportError(
    folly::exception_wrapper ew) {
  clientCallback_.onFinalResponseError(std::move(ew));
}
StreamChannelStatus RocketSinkServerCallback::onStreamPayload(StreamPayload&&) {
  clientCallback_.onFinalResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              STREAMING_CONTRACT_VIOLATION,
          "onStreamPayload called for sink"));
  return StreamChannelStatus::ContractViolation;
}
StreamChannelStatus RocketSinkServerCallback::onStreamFinalPayload(
    StreamPayload&& payload) {
  clientCallback_.onFinalResponse(std::move(payload));
  return StreamChannelStatus::Complete;
}
StreamChannelStatus RocketSinkServerCallback::onStreamComplete() {
  clientCallback_.onFinalResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              STREAMING_CONTRACT_VIOLATION,
          "onStreamComplete called for sink"));
  return StreamChannelStatus::ContractViolation;
}
StreamChannelStatus RocketSinkServerCallback::onStreamError(
    folly::exception_wrapper ew) {
  clientCallback_.onFinalResponseError(std::move(ew));
  return StreamChannelStatus::Complete;
}
StreamChannelStatus RocketSinkServerCallback::onSinkRequestN(uint64_t tokens) {
  switch (state_) {
    case State::BothOpen:
      clientCallback_.onSinkRequestN(tokens);
      return StreamChannelStatus::Alive;
    case State::StreamOpen:
      return StreamChannelStatus::Alive;
    default:
      folly::assume_unreachable();
  }
}
StreamChannelStatus RocketSinkServerCallback::onSinkCancel() {
  clientCallback_.onFinalResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::INTERRUPTED,
          "sink closed via onSinkCancel"));
  return StreamChannelStatus::ContractViolation;
}
} // namespace rocket
} // namespace thrift
} // namespace apache
