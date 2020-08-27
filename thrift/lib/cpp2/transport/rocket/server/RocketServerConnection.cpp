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
#include <folly/Utility.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>

#include <wangle/acceptor/ConnectionManager.h>

#include <thrift/lib/cpp/TApplicationException.h>
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

constexpr std::chrono::milliseconds
    RocketServerConnection::SocketDrainer::kRetryInterval;
constexpr std::chrono::seconds RocketServerConnection::SocketDrainer::kTimeout;

RocketServerConnection::RocketServerConnection(
    folly::AsyncTransport::UniquePtr socket,
    std::unique_ptr<RocketServerHandler> frameHandler,
    std::chrono::milliseconds streamStarvationTimeout,
    std::chrono::milliseconds writeBatchingInterval,
    size_t writeBatchingSize)
    : evb_(*socket->getEventBase()),
      socket_(std::move(socket)),
      frameHandler_(std::move(frameHandler)),
      streamStarvationTimeout_(streamStarvationTimeout),
      writeBatcher_(*this, writeBatchingInterval, writeBatchingSize),
      socketDrainer_(*this) {
  CHECK(socket_);
  CHECK(frameHandler_);
  socket_->setReadCB(&parser_);
}

RocketStreamClientCallback& RocketServerConnection::createStreamClientCallback(
    StreamId streamId,
    RocketServerConnection& connection,
    uint32_t initialRequestN) {
  auto callback = std::make_unique<RocketStreamClientCallback>(
      streamId, connection, initialRequestN);
  auto& callbackRef = *callback;
  streams_.emplace(streamId, std::move(callback));
  return callbackRef;
}

RocketSinkClientCallback& RocketServerConnection::createSinkClientCallback(
    StreamId streamId,
    RocketServerConnection& connection) {
  auto callback =
      std::make_unique<RocketSinkClientCallback>(streamId, connection);
  auto& callbackRef = *callback;
  streams_.emplace(streamId, std::move(callback));
  return callbackRef;
}

void RocketServerConnection::send(
    std::unique_ptr<folly::IOBuf> data,
    apache::thrift::MessageChannel::SendCallbackPtr cb) {
  evb_.dcheckIsInEventBaseThread();

  if (state_ != ConnectionState::ALIVE && state_ != ConnectionState::DRAINING) {
    return;
  }

  writeBatcher_.enqueueWrite(std::move(data), std::move(cb));
}

RocketServerConnection::~RocketServerConnection() {
  DCHECK(inflightRequests_ == 0);
  DCHECK(inflightWritesQueue_.empty());
  DCHECK(inflightSinkFinalResponses_ == 0);
  DCHECK(writeBatcher_.empty());
  socket_.reset();
}

void RocketServerConnection::closeIfNeeded() {
  if (state_ == ConnectionState::DRAINING && inflightRequests_ == 0 &&
      inflightSinkFinalResponses_ == 0) {
    DestructorGuard dg(this);
    // Immediately stop processing new requests
    socket_->setReadCB(nullptr);

    sendError(
        StreamId{0}, RocketException(ErrorCode::CONNECTION_DRAIN_COMPLETE));

    state_ = ConnectionState::CLOSING;
    closeIfNeeded();
  }

  if (state_ != ConnectionState::CLOSING || isBusy()) {
    return;
  }

  DestructorGuard dg(this);
  // Update state_ early, as subsequent lines may call recursively into
  // closeIfNeeded(). Such recursive calls should be no-ops.
  state_ = ConnectionState::CLOSED;

  if (auto* manager = getConnectionManager()) {
    manager->removeConnection(this);
  }

  while (!streams_.empty()) {
    auto callback = std::move(streams_.begin()->second);
    streams_.erase(streams_.begin());
    // Calling application callback may trigger rehashing.
    folly::variant_match(
        callback,
        [](const std::unique_ptr<RocketStreamClientCallback>& callback) {
          callback->onStreamCancel();
        },
        [](const std::unique_ptr<RocketSinkClientCallback>& callback) {
          bool state = callback->onSinkError(TApplicationException(
              TApplicationException::TApplicationExceptionType::INTERRUPTION));
          DCHECK(state) << "onSinkError called after sink complete!";
        });
    requestComplete();
  }

  writeBatcher_.drain();

  socketDrainer_.activate();
}

void RocketServerConnection::handleFrame(std::unique_ptr<folly::IOBuf> frame) {
  DestructorGuard dg(this);

  if (state_ != ConnectionState::ALIVE && state_ != ConnectionState::DRAINING) {
    return;
  }

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
      return frameHandler_->handleSetupFrame(
          SetupFrame(std::move(frame)), *this);
    }

    case FrameType::REQUEST_RESPONSE: {
      return handleRequestFrame(
          RequestResponseFrame(streamId, flags, cursor, std::move(frame)));
    }

    case FrameType::REQUEST_FNF: {
      return handleRequestFrame(
          RequestFnfFrame(streamId, flags, cursor, std::move(frame)));
    }

    case FrameType::REQUEST_STREAM: {
      return handleRequestFrame(
          RequestStreamFrame(streamId, flags, cursor, std::move(frame)));
    }

    case FrameType::REQUEST_CHANNEL: {
      return handleRequestFrame(
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
      if (UNLIKELY(iter == streams_.end())) {
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

      PayloadFrame payloadFrame(streamId, flags, cursor, std::move(frame));
      folly::variant_match(it->second, [&](auto& requestFrame) {
        const bool hasFollows = payloadFrame.hasFollows();
        requestFrame.payload().append(std::move(payloadFrame.payload()));
        if (!hasFollows) {
          RocketServerFrameContext(*this, streamId)
              .onFullFrame(std::move(requestFrame));
          partialRequestFrames_.erase(streamId);
        }
      });
      return;
    }
    case FrameType::CANCEL:
      FOLLY_FALLTHROUGH;
    case FrameType::REQUEST_N:
      FOLLY_FALLTHROUGH;
    case FrameType::ERROR:
      return;

    case FrameType::EXT: {
      ExtFrame extFrame(streamId, flags, cursor, std::move(frame));
      switch (extFrame.extFrameType()) {
        case ExtFrameType::INTERACTION_TERMINATE: {
          InteractionTerminate term;
          unpackCompact(term, extFrame.payload().buffer());
          frameHandler_->terminateInteraction(term.get_interactionId());
          return;
        }
        default:
          return;
      }
    }

    default:
      close(folly::make_exception_wrapper<RocketException>(
          ErrorCode::INVALID,
          fmt::format(
              "Received unhandleable frame type ({})",
              static_cast<uint8_t>(frameType))));
  }
}

void RocketServerConnection::handleStreamFrame(
    std::unique_ptr<folly::IOBuf> frame,
    StreamId streamId,
    FrameType frameType,
    Flags flags,
    folly::io::Cursor cursor,
    RocketStreamClientCallback& clientCallback) {
  if (!clientCallback.serverCallbackReady()) {
    switch (frameType) {
      case FrameType::CANCEL: {
        return clientCallback.earlyCancelled();
      }
      default:
        return close(folly::make_exception_wrapper<RocketException>(
            ErrorCode::INVALID,
            fmt::format(
                "Received unexpected early frame, stream id ({}) type ({})",
                static_cast<uint32_t>(streamId),
                static_cast<uint8_t>(frameType))));
    }
  }

  switch (frameType) {
    case FrameType::REQUEST_N: {
      RequestNFrame requestNFrame(streamId, flags, cursor);
      clientCallback.request(requestNFrame.requestN());
      return;
    }

    case FrameType::CANCEL: {
      clientCallback.onStreamCancel();
      freeStream(streamId, true);
      return;
    }

    case FrameType::EXT: {
      ExtFrame extFrame(streamId, flags, cursor, std::move(frame));
      switch (extFrame.extFrameType()) {
        case ExtFrameType::HEADERS_PUSH: {
          auto& serverCallback = clientCallback.getStreamServerCallback();
          auto headers = unpack<HeadersPayload>(std::move(extFrame.payload()));
          if (headers.hasException()) {
            serverCallback.onStreamCancel();
            freeStream(streamId, true);
            return;
          }
          std::ignore = serverCallback.onSinkHeaders(std::move(*headers));
          return;
        }
        case ExtFrameType::ALIGNED_PAGE:
        case ExtFrameType::INTERACTION_TERMINATE:
        case ExtFrameType::UNKNOWN:
          if (extFrame.hasIgnore()) {
            return;
          }
          close(folly::make_exception_wrapper<RocketException>(
              ErrorCode::INVALID,
              fmt::format(
                  "Received unhandleable EXT frame type ({}) for stream (id {})",
                  static_cast<uint32_t>(extFrame.extFrameType()),
                  static_cast<uint32_t>(streamId))));
          return;
      }
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
  if (!clientCallback.serverCallbackReady()) {
    switch (frameType) {
      case FrameType::ERROR: {
        ErrorFrame errorFrame{std::move(frame)};
        if (errorFrame.errorCode() == ErrorCode::CANCELED) {
          return clientCallback.earlyCancelled();
        }
      }
      default:
        return close(folly::make_exception_wrapper<RocketException>(
            ErrorCode::INVALID,
            fmt::format(
                "Received unexpected early frame, stream id ({}) type ({})",
                static_cast<uint32_t>(streamId),
                static_cast<uint8_t>(frameType))));
    }
  }

  auto handleSinkPayload = [&](PayloadFrame&& payloadFrame) {
    const bool next = payloadFrame.hasNext();
    const bool complete = payloadFrame.hasComplete();
    if (auto fullPayload = bufferOrGetFullPayload(std::move(payloadFrame))) {
      bool notViolateContract = true;
      if (next) {
        auto streamPayload =
            rocket::unpack<StreamPayload>(std::move(*fullPayload));
        if (streamPayload.hasException()) {
          notViolateContract =
              clientCallback.onSinkError(std::move(streamPayload.exception()));
          if (notViolateContract) {
            freeStream(streamId, true);
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
  };

  switch (frameType) {
    case FrameType::PAYLOAD: {
      PayloadFrame payloadFrame(streamId, flags, cursor, std::move(frame));
      handleSinkPayload(std::move(payloadFrame));
    } break;

    case FrameType::ERROR: {
      ErrorFrame errorFrame{std::move(frame)};
      auto ew = [&] {
        if (errorFrame.errorCode() == ErrorCode::CANCELED) {
          return folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::TApplicationExceptionType::INTERRUPTION);
        } else {
          return folly::make_exception_wrapper<RocketException>(
              errorFrame.errorCode(), std::move(errorFrame.payload()).data());
        }
      }();

      bool notViolateContract = clientCallback.onSinkError(std::move(ew));
      if (notViolateContract) {
        freeStream(streamId, true);
      } else {
        close(folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "receiving sink error frame after sink completion"));
      }
    } break;

    case FrameType::EXT: {
      ExtFrame extFrame(streamId, flags, cursor, std::move(frame));
      if (extFrame.extFrameType() == ExtFrameType::ALIGNED_PAGE) {
        PayloadFrame payloadFrame(
            streamId, std::move(extFrame.payload()), flags);
        handleSinkPayload(std::move(payloadFrame));
        break;
      }
    }

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
    closeIfNeeded();
    return;
  }

  DestructorGuard dg(this);
  // Immediately stop processing new requests
  socket_->setReadCB(nullptr);

  auto rex = ew
      ? RocketException(ErrorCode::CONNECTION_ERROR, ew.what())
      : RocketException(ErrorCode::CONNECTION_CLOSE, "Closing connection");
  sendError(StreamId{0}, std::move(rex));

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
  return inflightRequests_ != 0 || !inflightWritesQueue_.empty() ||
      inflightSinkFinalResponses_ != 0 || !writeBatcher_.empty();
}

// On graceful shutdown, ConnectionManager will first fire the
// notifyPendingShutdown() callback for each connection. Then, after the drain
// period has elapsed, closeWhenIdle() will be called for each connection.
// Note that ConnectionManager waits for a connection to become un-busy before
// calling closeWhenIdle().
void RocketServerConnection::notifyPendingShutdown() {
  if (state_ != ConnectionState::ALIVE) {
    return;
  }

  state_ = ConnectionState::DRAINING;
  closeIfNeeded();
}

void RocketServerConnection::dropConnection(const std::string& /* errorMsg */) {
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
  DCHECK(!inflightWritesQueue_.empty());
  auto& context = inflightWritesQueue_.front();
  for (auto processingCompleteCount = context.requestCompleteCount;
       processingCompleteCount > 0;
       --processingCompleteCount) {
    frameHandler_->requestComplete();
  }

  for (auto& cb : context.sendCallbacks) {
    cb.release()->messageSent();
  }

  inflightWritesQueue_.pop();
  closeIfNeeded();
}

void RocketServerConnection::writeErr(
    size_t /* bytesWritten */,
    const folly::AsyncSocketException& ex) noexcept {
  DestructorGuard dg(this);
  DCHECK(!inflightWritesQueue_.empty());
  auto& context = inflightWritesQueue_.front();
  for (auto processingCompleteCount = context.requestCompleteCount;
       processingCompleteCount > 0;
       --processingCompleteCount) {
    frameHandler_->requestComplete();
  }

  auto ew = folly::make_exception_wrapper<transport::TTransportException>(ex);

  for (auto& cb : context.sendCallbacks) {
    cb.release()->messageSendError(folly::copy(ew));
  }

  inflightWritesQueue_.pop();
  close(std::move(ew));
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

void RocketServerConnection::sendPayload(
    StreamId streamId,
    Payload&& payload,
    Flags flags,
    apache::thrift::MessageChannel::SendCallbackPtr cb) {
  send(
      PayloadFrame(streamId, std::move(payload), flags).serialize(),
      std::move(cb));
}

void RocketServerConnection::sendError(
    StreamId streamId,
    RocketException&& rex,
    apache::thrift::MessageChannel::SendCallbackPtr cb) {
  send(ErrorFrame(streamId, std::move(rex)).serialize(), std::move(cb));
}

void RocketServerConnection::sendRequestN(StreamId streamId, int32_t n) {
  send(RequestNFrame(streamId, n).serialize());
}

void RocketServerConnection::sendCancel(StreamId streamId) {
  send(CancelFrame(streamId).serialize());
}

void RocketServerConnection::sendExt(
    StreamId streamId,
    Payload&& payload,
    Flags flags,
    ExtFrameType extFrameType) {
  send(ExtFrame(streamId, std::move(payload), flags, extFrameType).serialize());
}

void RocketServerConnection::freeStream(
    StreamId streamId,
    bool markRequestComplete) {
  DestructorGuard dg(this);

  bufferedFragments_.erase(streamId);

  DCHECK(streams_.find(streamId) != streams_.end());
  streams_.erase(streamId);
  if (markRequestComplete) {
    requestComplete();
  }
}

} // namespace rocket
} // namespace thrift
} // namespace apache
