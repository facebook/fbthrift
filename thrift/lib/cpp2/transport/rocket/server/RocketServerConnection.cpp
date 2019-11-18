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

#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>

#include <memory>
#include <utility>

#include <fmt/core.h>
#include <folly/ExceptionWrapper.h>
#include <folly/Likely.h>
#include <folly/MapUtil.h>
#include <folly/Overload.h>
#include <folly/ScopeGuard.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>

#include <wangle/acceptor/ConnectionManager.h>

#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Util.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerHandler.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketSinkClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.h>

namespace apache {
namespace thrift {
namespace rocket {

RocketServerConnection::RocketServerConnection(
    folly::AsyncTransportWrapper::UniquePtr socket,
    std::shared_ptr<RocketServerHandler> frameHandler,
    std::chrono::milliseconds streamStarvationTimeout)
    : evb_(*socket->getEventBase()),
      socket_(std::move(socket)),
      frameHandler_(std::move(frameHandler)),
      streamStarvationTimeout_(streamStarvationTimeout) {
  CHECK(socket_);
  CHECK(frameHandler_);
  socket_->setReadCB(&parser_);
}

RocketStreamClientCallback* RocketServerConnection::createStreamClientCallback(
    RocketServerFrameContext&& context,
    uint32_t initialRequestN) {
  // RocketStreamClientCallback get owned by RocketServerConnection
  // in streams_ map after onFirstResponse() get called.
  return new RocketStreamClientCallback(std::move(context), initialRequestN);
}

RocketSinkClientCallback* RocketServerConnection::createSinkClientCallback(
    RocketServerFrameContext&& context) {
  return new RocketSinkClientCallback(std::move(context));
}

void RocketServerConnection::send(std::unique_ptr<folly::IOBuf> data) {
  evb_.dcheckIsInEventBaseThread();

  if (state_ != ConnectionState::ALIVE) {
    return;
  }

  batchWriteLoopCallback_.enqueueWrite(std::move(data));
  if (!batchWriteLoopCallback_.isLoopCallbackScheduled()) {
    evb_.runInLoop(&batchWriteLoopCallback_, true /* thisIteration */);
  }
}

RocketServerConnection::~RocketServerConnection() {
  DCHECK(inflightRequests_ == 0);
  DCHECK(inflightWrites_ == 0);
  DCHECK(batchWriteLoopCallback_.empty());
}

bool RocketServerConnection::closeIfNeeded() {
  if (state_ != ConnectionState::CLOSING ||
      inflightRequests_ != getNumStreams() || inflightWrites_ != 0) {
    return false;
  }

  DestructorGuard dg(this);
  // Update state_ early, as subsequent lines may call recursively into
  // closeIfNeeded(). Such recursive calls should be no-ops.
  state_ = ConnectionState::CLOSED;

  if (auto* manager = getConnectionManager()) {
    manager->removeConnection(this);
  }

  for (auto it = streams_.begin(); it != streams_.end();) {
    folly::variant_match(
        it->second,
        [](const std::unique_ptr<RocketStreamClientCallback>& callback) {
          auto& serverCallback = callback->getStreamServerCallback();
          serverCallback.onStreamCancel();
        },
        [](const std::unique_ptr<RocketSinkClientCallback>& callback) {
          callback->onStreamCancel();
        });
    it = streams_.erase(it);
  }

  if (batchWriteLoopCallback_.isLoopCallbackScheduled()) {
    batchWriteLoopCallback_.cancelLoopCallback();
    flushPendingWrites();
  }

  socket_.reset();
  destroy();
  return true;
}

void RocketServerConnection::handleFrame(std::unique_ptr<folly::IOBuf> frame) {
  DestructorGuard dg(this);

  // Entire payloads may be chained, but the parser ensures each fragment is
  // coalesced.
  DCHECK(!frame->isChained());
  folly::io::Cursor cursor(frame.get());

  const auto streamId = readStreamId(cursor);
  FrameType frameType;
  Flags flags;
  std::tie(frameType, flags) = readFrameTypeAndFlags(cursor);

  if (UNLIKELY(!setupFrameReceived_)) {
    if (frameType != FrameType::SETUP) {
      return close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID_SETUP, "First frame must be SETUP frame"));
    }
    setupFrameReceived_ = true;
  } else {
    if (UNLIKELY(frameType == FrameType::SETUP)) {
      return close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID_SETUP, "More than one SETUP frame received"));
    }
  }

  switch (frameType) {
    case FrameType::SETUP: {
      RocketServerFrameContext frameContext(*this, streamId);
      return frameHandler_->handleSetupFrame(
          SetupFrame(std::move(frame)), std::move(frameContext));
    }

    case FrameType::REQUEST_RESPONSE: {
      RocketServerFrameContext frameContext(*this, streamId);
      return std::move(frameContext)
          .onRequestFrame(
              RequestResponseFrame(streamId, flags, cursor, std::move(frame)));
    }

    case FrameType::REQUEST_FNF: {
      RocketServerFrameContext frameContext(*this, streamId);
      return std::move(frameContext)
          .onRequestFrame(
              RequestFnfFrame(streamId, flags, cursor, std::move(frame)));
    }

    case FrameType::REQUEST_STREAM: {
      RocketServerFrameContext frameContext(*this, streamId);
      return std::move(frameContext)
          .onRequestFrame(
              RequestStreamFrame(streamId, flags, cursor, std::move(frame)));
    }

    case FrameType::REQUEST_CHANNEL: {
      RocketServerFrameContext frameContext(*this, streamId);
      return std::move(frameContext)
          .onRequestFrame(
              RequestChannelFrame(streamId, flags, cursor, std::move(frame)));
    }

    case FrameType::KEEPALIVE: {
      if (streamId == StreamId{0}) {
        KeepAliveFrame keepAliveFrame{std::move(frame)};
        if (keepAliveFrame.hasRespondFlag()) {
          // Echo back data without 'respond' flag
          send(KeepAliveFrame{Flags::none(), std::move(keepAliveFrame).data()}
                   .serialize());
        }
      } else {
        close(folly::make_exception_wrapper<RocketException>(
            ErrorCode::CONNECTION_ERROR,
            fmt::format(
                "Received keepalive frame with non-zero stream ID {}",
                static_cast<uint32_t>(streamId))));
      }
      return;
    }

    // for the rest of frame types, how to deal with them depends on whether
    // they are part of a streaming or sink (or channel)
    default: {
      auto iter = streams_.find(streamId);
      if (iter == streams_.end()) {
        handleUntrackedFrame(
            std::move(frame), streamId, frameType, flags, std::move(cursor));
      } else {
        folly::variant_match(
            iter->second,
            [&](const std::unique_ptr<RocketStreamClientCallback>&
                    clientCallback) {
              handleStreamFrame(
                  std::move(frame),
                  streamId,
                  frameType,
                  flags,
                  std::move(cursor),
                  *clientCallback);
            },
            [&](const std::unique_ptr<RocketSinkClientCallback>&
                    clientCallback) {
              handleSinkFrame(
                  std::move(frame),
                  streamId,
                  frameType,
                  flags,
                  std::move(cursor),
                  *clientCallback);
            });
      }
    }
  }
}

void RocketServerConnection::handleUntrackedFrame(
    std::unique_ptr<folly::IOBuf> frame,
    StreamId streamId,
    FrameType frameType,
    Flags flags,
    folly::io::Cursor cursor) {
  switch (frameType) {
    case FrameType::PAYLOAD: {
      auto it = partialRequestFrames_.find(streamId);
      if (it == partialRequestFrames_.end()) {
        return;
      }

      auto& frameContext = it->second;
      PayloadFrame payloadFrame(streamId, flags, cursor, std::move(frame));

      const bool hasFollows = payloadFrame.hasFollows();
      SCOPE_EXIT {
        if (!hasFollows) {
          partialRequestFrames_.erase(streamId);
        }
      };

      // Note that frameContext is only moved out of if we're handling the
      // final fragment.
      return std::move(frameContext).onPayloadFrame(std::move(payloadFrame));
    }

    case FrameType::CANCEL:
      FOLLY_FALLTHROUGH;
    case FrameType::REQUEST_N:
      FOLLY_FALLTHROUGH;
    case FrameType::ERROR:
      return;

    default:
      close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID,
          fmt::format(
              "Received unhandleable frame type ({})",
              static_cast<uint8_t>(frameType))));
  }
}

void RocketServerConnection::handleStreamFrame(
    std::unique_ptr<folly::IOBuf>,
    StreamId streamId,
    FrameType frameType,
    Flags flags,
    folly::io::Cursor cursor,
    RocketStreamClientCallback& clientCallback) {
  switch (frameType) {
    case FrameType::REQUEST_N: {
      RequestNFrame requestNFrame(streamId, flags, cursor);
      clientCallback.request(requestNFrame.requestN());
      return;
    }

    case FrameType::CANCEL: {
      auto& serverCallback = clientCallback.getStreamServerCallback();
      serverCallback.onStreamCancel();
      freeStream(streamId);
      return;
    }

    default:
      close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID,
          fmt::format(
              "Received unhandleable frame type ({}) for stream (id {})",
              static_cast<uint8_t>(frameType),
              static_cast<uint32_t>(streamId))));
  }
}

void RocketServerConnection::handleSinkFrame(
    std::unique_ptr<folly::IOBuf> frame,
    StreamId streamId,
    FrameType frameType,
    Flags flags,
    folly::io::Cursor cursor,
    RocketSinkClientCallback& clientCallback) {
  switch (frameType) {
    case FrameType::PAYLOAD: {
      PayloadFrame payloadFrame(streamId, flags, cursor, std::move(frame));
      const bool next = payloadFrame.hasNext();
      const bool complete = payloadFrame.hasComplete();
      if (auto fullPayload = bufferOrGetFullPayload(std::move(payloadFrame))) {
        bool notViolateContract = true;
        if (next) {
          auto streamPayload =
              rocket::unpack<StreamPayload>(std::move(*fullPayload));
          if (streamPayload.hasException()) {
            notViolateContract = clientCallback.onSinkError(
                std::move(streamPayload.exception()));
            if (notViolateContract) {
              freeStream(streamId);
            }
          } else {
            notViolateContract =
                clientCallback.onSinkNext(std::move(*streamPayload));
          }
        }

        if (complete) {
          // it is possible final repsonse(error) sent from serverCallback,
          // serverCallback may be already destoryed.
          if (streams_.find(streamId) != streams_.end()) {
            notViolateContract = clientCallback.onSinkComplete();
          }
        }

        if (!notViolateContract) {
          close(folly::make_exception_wrapper<transport::TTransportException>(
              transport::TTransportException::TTransportExceptionType::
                  STREAMING_CONTRACT_VIOLATION,
              "receiving sink payload frame after sink completion"));
        }
      }
    } break;

    case FrameType::ERROR: {
      ErrorFrame errorFrame{std::move(frame)};
      auto ew = folly::make_exception_wrapper<RocketException>(
          errorFrame.errorCode(), std::move(errorFrame.payload()).data());
      bool notViolateContract = clientCallback.onSinkError(std::move(ew));
      if (notViolateContract) {
        freeStream(streamId);
      } else {
        close(folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "receiving sink error frame after sink completion"));
      }
    } break;

    case FrameType::CANCEL: {
      clientCallback.onStreamCancel();
      return freeStream(streamId);
    } break;

    default:
      close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID,
          fmt::format(
              "Received unhandleable frame type ({}) for sink (id {})",
              static_cast<uint8_t>(frameType),
              static_cast<uint32_t>(streamId))));
  }
}

void RocketServerConnection::close(folly::exception_wrapper ew) {
  if (state_ == ConnectionState::CLOSING || state_ == ConnectionState::CLOSED) {
    return;
  }

  DestructorGuard dg(this);
  // Immediately stop processing new requests
  socket_->setReadCB(nullptr);

  auto rex = ew
      ? RocketException(ErrorCode::CONNECTION_ERROR, ew.what())
      : RocketException(ErrorCode::CONNECTION_CLOSE, "Closing connection");
  RocketServerFrameContext(*this, StreamId{0}).sendError(std::move(rex));

  state_ = ConnectionState::CLOSING;
  closeIfNeeded();
}

void RocketServerConnection::timeoutExpired() noexcept {
  DestructorGuard dg(this);

  if (!isBusy()) {
    closeWhenIdle();
  }
}

bool RocketServerConnection::isBusy() const {
  return inflightRequests_ != 0 || inflightWrites_ != 0 ||
      batchWriteLoopCallback_.isLoopCallbackScheduled();
}

// On graceful shutdown, ConnectionManager will first fire the
// notifyPendingShutdown() callback for each connection. Then, after the drain
// period has elapsed, closeWhenIdle() will be called for each connection. Note
// that ConnectionManager waits for a connection to become un-busy before
// calling closeWhenIdle().
void RocketServerConnection::notifyPendingShutdown() {}

void RocketServerConnection::dropConnection() {
  close(folly::make_exception_wrapper<transport::TTransportException>(
      transport::TTransportException::TTransportExceptionType::INTERRUPTED,
      "Dropping connection"));
}

void RocketServerConnection::closeWhenIdle() {
  close(folly::make_exception_wrapper<transport::TTransportException>(
      transport::TTransportException::TTransportExceptionType::INTERRUPTED,
      "Closing due to imminent shutdown"));
}

void RocketServerConnection::writeSuccess() noexcept {
  DCHECK(inflightWrites_ != 0);
  --inflightWrites_;
  closeIfNeeded();
}

void RocketServerConnection::writeErr(
    size_t bytesWritten,
    const folly::AsyncSocketException& ex) noexcept {
  DestructorGuard dg(this);
  DCHECK(inflightWrites_ != 0);
  --inflightWrites_;
  close(folly::make_exception_wrapper<std::runtime_error>(fmt::format(
      "Failed to write to remote endpoint. Wrote {} bytes."
      " AsyncSocketException: {}",
      bytesWritten,
      ex.what())));
}

void RocketServerConnection::scheduleStreamTimeout(
    folly::HHWheelTimer::Callback* timeoutCallback) {
  if (streamStarvationTimeout_ != std::chrono::milliseconds::zero()) {
    evb_.timer().scheduleTimeout(timeoutCallback, streamStarvationTimeout_);
  }
}

void RocketServerConnection::scheduleSinkTimeout(
    folly::HHWheelTimer::Callback* timeoutCallback,
    std::chrono::milliseconds timeout) {
  if (timeout != std::chrono::milliseconds::zero()) {
    evb_.timer().scheduleTimeout(timeoutCallback, timeout);
  }
}

folly::Optional<Payload> RocketServerConnection::bufferOrGetFullPayload(
    PayloadFrame&& payloadFrame) {
  folly::Optional<Payload> fullPayload;

  const auto streamId = payloadFrame.streamId();
  const bool hasFollows = payloadFrame.hasFollows();
  const auto it = bufferedFragments_.find(streamId);

  if (hasFollows) {
    if (it != bufferedFragments_.end()) {
      auto& firstFragments = it->second;
      firstFragments.append(std::move(payloadFrame.payload()));
    } else {
      bufferedFragments_.emplace(streamId, std::move(payloadFrame.payload()));
    }
  } else {
    if (it != bufferedFragments_.end()) {
      auto firstFragments = std::move(it->second);
      bufferedFragments_.erase(it);
      firstFragments.append(std::move(payloadFrame.payload()));
      fullPayload = std::move(firstFragments);
    } else {
      fullPayload = std::move(payloadFrame.payload());
    }
  }

  return fullPayload;
}

void RocketServerConnection::freeStream(StreamId streamId) {
  DestructorGuard dg(this);

  bufferedFragments_.erase(streamId);

  auto it = streams_.find(streamId);
  if (it != streams_.end()) {
    // Avoid potentially erasing from streams_ map recursively via closeIfNeeded
    auto ctx = std::move(*it);
    streams_.erase(it);
  }
}

} // namespace rocket
} // namespace thrift
} // namespace apache
