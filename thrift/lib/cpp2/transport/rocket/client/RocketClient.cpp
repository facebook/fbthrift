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

#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>

#include <chrono>
#include <functional>
#include <string>
#include <utility>

#include <folly/Conv.h>
#include <folly/CppAttributes.h>
#include <folly/ExceptionWrapper.h>
#include <folly/Format.h>
#include <folly/GLog.h>
#include <folly/Likely.h>
#include <folly/Try.h>
#include <folly/fibers/FiberManager.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <folly/lang/Exception.h>

#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RequestContext.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClientFlowable.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Util.h>

namespace apache {
namespace thrift {
namespace rocket {

namespace {
class OnEventBaseDestructionCallback
    : public folly::EventBase::OnDestructionCallback {
 public:
  explicit OnEventBaseDestructionCallback(RocketClient& client)
      : client_(client) {}

  void onEventBaseDestruction() noexcept final {
    client_.closeNow(folly::make_exception_wrapper<std::runtime_error>(
        "Destroying EventBase"));
  }

 private:
  RocketClient& client_;
};
} // namespace

RocketClient::RocketClient(
    folly::EventBase& evb,
    folly::AsyncTransportWrapper::UniquePtr socket)
    : evb_(&evb),
      fm_(&folly::fibers::getFiberManager(*evb_)),
      socket_(std::move(socket)),
      parser_(*this),
      writeLoopCallback_(*this),
      eventBaseDestructionCallback_(
          std::make_unique<OnEventBaseDestructionCallback>(*this)) {
  DCHECK(socket_ != nullptr);
  socket_->setReadCB(&parser_);
  evb_->runOnDestruction(*eventBaseDestructionCallback_);
}

RocketClient::~RocketClient() {
  closeNow(folly::make_exception_wrapper<std::runtime_error>(
      "Destroying RocketClient"));
  eventBaseDestructionCallback_->cancel();

  // All outstanding request contexts should have been cleaned up in closeNow()
  DCHECK(!queue_.hasInflightRequests());
  DCHECK(streams_.empty());
}

std::shared_ptr<RocketClient> RocketClient::create(
    folly::EventBase& evb,
    folly::AsyncTransportWrapper::UniquePtr socket) {
  return std::shared_ptr<RocketClient>(
      new RocketClient(evb, std::move(socket)),
      DelayedDestruction::Destructor());
}

void RocketClient::handleFrame(std::unique_ptr<folly::IOBuf> frame) {
  DestructorGuard dg(this);

  // Entire payloads may be chained, but the parser ensures each fragment is
  // coalesced.
  DCHECK(!frame->isChained());
  folly::io::Cursor cursor(frame.get());

  const auto streamId = readStreamId(cursor);
  FrameType frameType;
  std::tie(frameType, std::ignore) = readFrameTypeAndFlags(cursor);

  if (UNLIKELY(frameType == FrameType::ERROR && streamId == StreamId{0})) {
    ErrorFrame errorFrame(std::move(frame));
    return close(folly::make_exception_wrapper<RocketException>(
        errorFrame.errorCode(), std::move(errorFrame.payload()).data()));
  }

  if (auto* ctx = queue_.getRequestResponseContext(streamId)) {
    DCHECK(ctx->isRequestResponse());
    return handleRequestResponseFrame(*ctx, frameType, std::move(frame));
  }

  if (auto* stream = getStreamById(streamId)) {
    DCHECK(!stream->requestStreamPayload);
    return handleStreamFrame(*stream->flowable, frameType, std::move(frame));
  }
}

void RocketClient::handleRequestResponseFrame(
    RequestContext& ctx,
    FrameType frameType,
    std::unique_ptr<folly::IOBuf> frame) {
  switch (frameType) {
    case FrameType::PAYLOAD:
      return ctx.onPayloadFrame(PayloadFrame(std::move(frame)));

    case FrameType::ERROR:
      return ctx.onErrorFrame(ErrorFrame(std::move(frame)));

    default:
      close(folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              NETWORK_ERROR,
          folly::to<std::string>(
              "Client attempting to handle unhandleable frame type: ",
              static_cast<uint8_t>(frameType))));
  }
}

void RocketClient::handleStreamFrame(
    RocketClientFlowable& stream,
    FrameType frameType,
    std::unique_ptr<folly::IOBuf> frame) {
  switch (frameType) {
    case FrameType::PAYLOAD:
      stream.onPayloadFrame(PayloadFrame(std::move(frame)));
      break;

    case FrameType::ERROR:
      stream.onErrorFrame(ErrorFrame(std::move(frame)));
      break;

    default:
      close(folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              NETWORK_ERROR,
          folly::to<std::string>(
              "Client attempting to handle unhandleable frame type: ",
              static_cast<uint8_t>(frameType))));
  }
}

Payload RocketClient::sendRequestResponseSync(
    Payload&& request,
    std::chrono::milliseconds timeout) {
  RequestContext ctx(
      RequestResponseFrame(makeStreamId(), std::move(request)),
      queue_,
      !std::exchange(setupFrameSent_, true) /* setupFrameNeeded */);
  scheduleWrite(ctx);
  return ctx.waitForResponse(timeout);
}

void RocketClient::sendRequestFnfSync(Payload&& request) {
  RequestContext ctx(
      RequestFnfFrame(makeStreamId(), std::move(request)),
      queue_,
      !std::exchange(setupFrameSent_, true) /* setupFrameNeeded */
  );
  scheduleWrite(ctx);
  return ctx.waitForWriteToComplete();
}

std::shared_ptr<RocketClientFlowable> RocketClient::createStream(
    Payload&& request) {
  const auto streamId = makeStreamId();
  auto flowable = std::make_shared<RocketClientFlowable>(streamId, *this);
  streams_.emplace(
      streamId,
      StreamWrapper{std::make_unique<Payload>(std::move(request)), flowable});
  return flowable;
}

void RocketClient::sendRequestNSync(StreamId streamId, int32_t n) {
  if (UNLIKELY(n <= 0)) {
    return;
  }

  auto* stream = getStreamById(streamId);
  if (!stream) {
    return;
  }

  if (stream->requestStreamPayload) {
    RequestContext ctx(
        RequestStreamFrame(
            streamId, std::move(*stream->requestStreamPayload), n),
        queue_,
        !std::exchange(setupFrameSent_, true) /* setupFrameNeeded */);
    stream->requestStreamPayload.reset();
    scheduleWrite(ctx);
    return ctx.waitForWriteToComplete();
  }

  RequestContext ctx(
      RequestNFrame(streamId, n), queue_, false /* setupFrameNeeded */);
  scheduleWrite(ctx);
  return ctx.waitForWriteToComplete();
}

void RocketClient::sendCancelSync(StreamId streamId) {
  RequestContext ctx(
      CancelFrame(streamId), queue_, false /* setupFrameNeeded */);
  SCOPE_EXIT {
    freeStream(streamId);
  };
  scheduleWrite(ctx);
  ctx.waitForWriteToComplete();
}

void RocketClient::sendRequestN(StreamId streamId, int32_t n) {
  fm_->addTaskFinally(
      [this, streamId, n] { sendRequestNSync(streamId, n); },
      [this, keepAlive = shared_from_this()](folly::Try<void>&& result) {
        if (result.hasException()) {
          FB_LOG_EVERY_MS(ERROR, 1000) << "sendRequestN failed, closing now: "
                                       << result.exception().what();
          closeNow(std::move(result.exception()));
        }
      });
}

void RocketClient::cancelStream(StreamId streamId) {
  fm_->addTaskFinally(
      [this, streamId] { sendCancelSync(streamId); },
      [this, keepAlive = shared_from_this()](folly::Try<void>&& result) {
        if (result.hasException()) {
          FB_LOG_EVERY_MS(ERROR, 1000) << "cancelStream failed, closing now: "
                                       << result.exception().what();
          closeNow(std::move(result.exception()));
        }
      });
}

void RocketClient::scheduleWrite(RequestContext& ctx) {
  if (!evb_) {
    folly::throw_exception(transport::TTransportException(
        transport::TTransportException::TTransportExceptionType::INVALID_STATE,
        "Cannot send requests on a detached client"));
  }

  if (state_ != ConnectionState::CONNECTED) {
    folly::throw_exception(transport::TTransportException(
        transport::TTransportException::TTransportExceptionType::NOT_OPEN,
        "Write not scheduled on disconnected client"));
  }

  queue_.enqueueScheduledWrite(ctx);
  if (!writeLoopCallback_.isLoopCallbackScheduled()) {
    evb_->runInLoop(&writeLoopCallback_);
  }
}

void RocketClient::WriteLoopCallback::runLoopCallback() noexcept {
  if (!std::exchange(rescheduled_, true)) {
    client_.evb_->runInLoop(this, true /* thisIteration */);
    return;
  }
  rescheduled_ = false;
  client_.writeScheduledRequestsToSocket();
}

void RocketClient::writeScheduledRequestsToSocket() noexcept {
  DestructorGuard dg(this);

  for (size_t reqsToWrite = queue_.scheduledWriteQueueSize();
       reqsToWrite != 0 && state_ == ConnectionState::CONNECTED;) {
    auto& req = queue_.markNextScheduledWriteAsSending();
    auto iobufChain = req.serializedChain();
    socket_->writeChain(
        this,
        std::move(iobufChain),
        --reqsToWrite ? folly::WriteFlags::CORK : folly::WriteFlags::NONE);
  }

  notifyIfDetachable();
}

void RocketClient::writeSuccess() noexcept {
  DestructorGuard dg(this);
  DCHECK(state_ != ConnectionState::CLOSED);

  auto& req = queue_.markNextSendingAsSent();
  if (req.isRequestResponse()) {
    req.scheduleTimeoutForResponse();
  } else {
    queue_.markAsResponded(req);
  }

  // In some cases, a successful write may happen after writeErr() has been
  // called on a preceding request.
  if (state_ == ConnectionState::ERROR) {
    close(folly::make_exception_wrapper<transport::TTransportException>(
        transport::TTransportException::TTransportExceptionType::INTERRUPTED,
        "Already processing error"));
  }
}

void RocketClient::writeErr(
    size_t bytesWritten,
    const folly::AsyncSocketException& ex) noexcept {
  DestructorGuard dg(this);
  DCHECK(state_ != ConnectionState::CLOSED);

  queue_.markNextSendingAsSent();

  close(folly::make_exception_wrapper<std::runtime_error>(folly::sformat(
      "Failed to write to remote endpoint. Wrote {} bytes."
      " AsyncSocketException: {}",
      bytesWritten,
      ex.what())));
}

void RocketClient::closeNow(folly::exception_wrapper ew) noexcept {
  DestructorGuard dg(this);

  if (auto closeCallback = std::move(closeCallback_)) {
    closeCallback();
  }

  if (socket_) {
    socket_->closeNow();
    socket_.reset();
  }

  // Move streams_ into a local copy before iterating and erasing. Note that
  // flowable->onError() may itself attempt to erase an element of streams_,
  // invalidating any outstanding iterators.
  auto streams = std::move(streams_);
  for (auto it = streams.begin(); it != streams.end();) {
    auto& flowable = it->second.flowable;
    flowable->onError(ew);
    // Since the client is shutting down now, we don't bother with
    // notifyIfDetachable().
    it = streams.erase(it);
  }
}

void RocketClient::close(folly::exception_wrapper ew) noexcept {
  DestructorGuard dg(this);
  DCHECK(ew);

  switch (state_) {
    case ConnectionState::CONNECTED:
      writeLoopCallback_.cancelLoopCallback();

      state_ = ConnectionState::ERROR;
      socket_->setReadCB(nullptr);
      // We'll still wait for any pending writes buffered up within
      // AsyncSocket to complete (with either writeSuccess() or writeErr()).
      socket_->close();

      FOLLY_FALLTHROUGH;

    case ConnectionState::ERROR:
      queue_.failAllSentWrites(ew);
      // Once there are no more inflight requests, we can safely fail any
      // remaining scheduled requests.
      if (!queue_.hasInflightRequests()) {
        queue_.failAllScheduledWrites(ew);
        state_ = ConnectionState::CLOSED;
        closeNow(std::move(ew));
      }
      return;

    case ConnectionState::CLOSED:
      LOG(FATAL) << "close() called on an already CLOSED connection";
  };
}

RocketClient::StreamWrapper* RocketClient::getStreamById(StreamId streamId) {
  auto it = streams_.find(streamId);
  return it != streams_.end() ? &it->second : nullptr;
}

void RocketClient::freeStream(StreamId streamId) {
  streams_.erase(streamId);
  notifyIfDetachable();
}

} // namespace rocket
} // namespace thrift
} // namespace apache
