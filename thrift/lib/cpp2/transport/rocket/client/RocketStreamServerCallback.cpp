/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/transport/rocket/client/RocketStreamServerCallback.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
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
bool RocketStreamServerCallback::onStreamRequestN(uint64_t tokens) {
  return client_.sendRequestN(streamId_, tokens);
}
void RocketStreamServerCallback::onStreamCancel() {
  client_.cancelStream(streamId_);
}
bool RocketStreamServerCallback::onSinkHeaders(HeadersPayload&& payload) {
  return client_.sendExtHeaders(streamId_, std::move(payload));
}

void RocketStreamServerCallback::onInitialPayload(
    FirstResponsePayload&& payload,
    folly::EventBase* evb) {
  std::ignore = clientCallback_->onFirstResponse(std::move(payload), evb, this);
}
void RocketStreamServerCallback::onInitialError(folly::exception_wrapper ew) {
  clientCallback_->onFirstResponseError(std::move(ew));
  client_.cancelStream(streamId_);
}
void RocketStreamServerCallback::onStreamTransportError(
    folly::exception_wrapper ew) {
  clientCallback_->onStreamError(std::move(ew));
}
StreamChannelStatus RocketStreamServerCallback::onStreamPayload(
    StreamPayload&& payload) {
  std::ignore = clientCallback_->onStreamNext(std::move(payload));
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
  ew.handle(
      [&](RocketException& ex) {
        clientCallback_->onStreamError(
            thrift::detail::EncodedError(ex.moveErrorData()));
      },
      [&](...) {
        clientCallback_->onStreamError(std::move(ew));
        return false;
      });
  return StreamChannelStatus::Complete;
}
void RocketStreamServerCallback::onStreamHeaders(HeadersPayload&& payload) {
  std::ignore = clientCallback_->onStreamHeaders(std::move(payload));
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

// RocketStreamServerCallbackWithChunkTimeout
bool RocketStreamServerCallbackWithChunkTimeout::onStreamRequestN(
    uint64_t tokens) {
  if (credits_ == 0) {
    scheduleTimeout();
  }
  credits_ += tokens;
  return RocketStreamServerCallback::onStreamRequestN(tokens);
}

void RocketStreamServerCallbackWithChunkTimeout::onInitialPayload(
    FirstResponsePayload&& payload,
    folly::EventBase* evb) {
  if (credits_ > 0) {
    scheduleTimeout();
  }
  RocketStreamServerCallback::onInitialPayload(std::move(payload), evb);
}

StreamChannelStatus RocketStreamServerCallbackWithChunkTimeout::onStreamPayload(
    StreamPayload&& payload) {
  DCHECK(credits_ != 0);
  if (--credits_ != 0) {
    scheduleTimeout();
  } else {
    cancelTimeout();
  }
  return RocketStreamServerCallback::onStreamPayload(std::move(payload));
}
void RocketStreamServerCallbackWithChunkTimeout::timeoutExpired() noexcept {
  onStreamTransportError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::TIMED_OUT,
          "stream chunk timeout"));
  onStreamCancel();
}
void RocketStreamServerCallbackWithChunkTimeout::scheduleTimeout() {
  if (!timeout_) {
    timeout_ = std::make_unique<
        TimeoutCallback<RocketStreamServerCallbackWithChunkTimeout>>(*this);
  }
  client_.scheduleTimeout(timeout_.get(), chunkTimeout_);
}
void RocketStreamServerCallbackWithChunkTimeout::cancelTimeout() {
  timeout_.reset();
}

// RocketChannelServerCallback
void RocketChannelServerCallback::onStreamRequestN(uint64_t tokens) {
  DCHECK(state_ == State::BothOpen || state_ == State::StreamOpen);
  std::ignore = client_.sendRequestN(streamId_, tokens);
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
  ew.handle(
      [&](rocket::RocketException& rex) {
        client_.sendError(
            streamId_,
            rocket::RocketException(
                rocket::ErrorCode::APPLICATION_ERROR, rex.moveErrorData()));
      },
      [&](...) {
        client_.sendError(
            streamId_,
            rocket::RocketException(
                rocket::ErrorCode::APPLICATION_ERROR, ew.what()));
      });
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
  client_.cancelStream(streamId_);
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
void RocketChannelServerCallback::onStreamHeaders(HeadersPayload&&) {}
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
bool RocketSinkServerCallback::onSinkNext(StreamPayload&& payload) {
  DCHECK(state_ == State::BothOpen);
  if (LIKELY(!pageAligned_)) {
    // apply compression if client has specified compression codec
    if (compressionConfig_) {
      detail::setCompressionCodec(
          *compressionConfig_,
          payload.metadata,
          payload.payload->computeChainDataLength());
    }
    client_.sendPayload(
        streamId_, std::move(payload), rocket::Flags::none().next(true));
  } else {
    client_.sendExtAlignedPage(
        streamId_,
        std::move(payload).payload,
        rocket::Flags::none().next(true));
  }
  return true;
}
void RocketSinkServerCallback::onSinkError(folly::exception_wrapper ew) {
  DCHECK(state_ == State::BothOpen);
  ew.handle(
      [&](rocket::RocketException& rex) {
        client_.sendError(
            streamId_,
            rocket::RocketException(
                rocket::ErrorCode::APPLICATION_ERROR, rex.moveErrorData()));
      },
      [&](...) {
        client_.sendError(
            streamId_,
            rocket::RocketException(
                rocket::ErrorCode::APPLICATION_ERROR, ew.what()));
      });
}
bool RocketSinkServerCallback::onSinkComplete() {
  DCHECK(state_ == State::BothOpen);
  state_ = State::StreamOpen;
  client_.sendComplete(streamId_, false);
  return true;
}

void RocketSinkServerCallback::onInitialPayload(
    FirstResponsePayload&& payload,
    folly::EventBase* evb) {
  std::ignore = clientCallback_->onFirstResponse(std::move(payload), evb, this);
}
void RocketSinkServerCallback::onInitialError(folly::exception_wrapper ew) {
  clientCallback_->onFirstResponseError(std::move(ew));
  client_.sendError(
      streamId_, rocket::RocketException(rocket::ErrorCode::CANCELED));
}
void RocketSinkServerCallback::onStreamTransportError(
    folly::exception_wrapper ew) {
  clientCallback_->onFinalResponseError(std::move(ew));
}
StreamChannelStatus RocketSinkServerCallback::onStreamPayload(StreamPayload&&) {
  clientCallback_->onFinalResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              STREAMING_CONTRACT_VIOLATION,
          "onStreamPayload called for sink"));
  return StreamChannelStatus::ContractViolation;
}
StreamChannelStatus RocketSinkServerCallback::onStreamFinalPayload(
    StreamPayload&& payload) {
  clientCallback_->onFinalResponse(std::move(payload));
  return StreamChannelStatus::Complete;
}
StreamChannelStatus RocketSinkServerCallback::onStreamComplete() {
  clientCallback_->onFinalResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              STREAMING_CONTRACT_VIOLATION,
          "onStreamComplete called for sink"));
  return StreamChannelStatus::ContractViolation;
}
StreamChannelStatus RocketSinkServerCallback::onStreamError(
    folly::exception_wrapper ew) {
  ew.handle(
      [&](RocketException& ex) {
        clientCallback_->onFinalResponseError(
            thrift::detail::EncodedError(ex.moveErrorData()));
      },
      [&](...) { clientCallback_->onFinalResponseError(std::move(ew)); });
  return StreamChannelStatus::Complete;
}
void RocketSinkServerCallback::onStreamHeaders(HeadersPayload&&) {}
StreamChannelStatus RocketSinkServerCallback::onSinkRequestN(uint64_t tokens) {
  switch (state_) {
    case State::BothOpen:
      std::ignore = clientCallback_->onSinkRequestN(tokens);
      return StreamChannelStatus::Alive;
    case State::StreamOpen:
      return StreamChannelStatus::Alive;
    default:
      folly::assume_unreachable();
  }
}
StreamChannelStatus RocketSinkServerCallback::onSinkCancel() {
  clientCallback_->onFinalResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::INTERRUPTED,
          "sink closed via onSinkCancel"));
  return StreamChannelStatus::ContractViolation;
}
} // namespace rocket
} // namespace thrift
} // namespace apache
