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

#include <fmt/core.h>
#include <folly/Conv.h>
#include <folly/CppAttributes.h>
#include <folly/ExceptionWrapper.h>
#include <folly/GLog.h>
#include <folly/Likely.h>
#include <folly/ScopeGuard.h>
#include <folly/Try.h>
#include <folly/fibers/FiberManager.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <folly/lang/Exception.h>

#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/core/TryUtil.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RequestContext.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClientFlowable.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Util.h>

namespace apache {
namespace thrift {
namespace rocket {

class RocketClientWriteCallback;

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

template <class T>
folly::Try<T> unpack(rocket::Payload&& payload) {
  return folly::makeTryWith([&] {
    T t{std::move(payload).data(), {}};
    if (payload.hasNonemptyMetadata()) {
      CompactProtocolReader reader;
      reader.setInput(payload.metadata().get());
      t.metadata.read(&reader);
    }
    return t;
  });
}
} // namespace

RocketClient::RocketClient(
    folly::EventBase& evb,
    folly::AsyncTransportWrapper::UniquePtr socket,
    std::unique_ptr<SetupFrame> setupFrame)
    : evb_(&evb),
      fm_(&folly::fibers::getFiberManager(*evb_)),
      socket_(std::move(socket)),
      setupFrame_(std::move(setupFrame)),
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
  DCHECK(streamsV2_.empty());
}

std::shared_ptr<RocketClient> RocketClient::create(
    folly::EventBase& evb,
    folly::AsyncTransportWrapper::UniquePtr socket,
    std::unique_ptr<SetupFrame> setupFrame) {
  return std::shared_ptr<RocketClient>(
      new RocketClient(evb, std::move(socket), std::move(setupFrame)),
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

  if (auto* ctx = queue_.getTrackedContext(streamId)) {
    DCHECK(ctx->expectingResponse());
    return handleResponseFrame(*ctx, frameType, std::move(frame));
  }

  if (auto* stream = getStreamById(streamId)) {
    DCHECK(!stream->requestStreamPayload);
    return handleStreamFrame(*stream->flowable, frameType, std::move(frame));
  } else if (auto* serverCallback = getStreamV2ById(streamId)) {
    return handleStreamFrame(*serverCallback, frameType, std::move(frame));
  }
}

// Called when response for REQUEST_RESPONSE frame is received, or when the
// first payload on a stream is received, provided the sender explicitly waited
// on the first payload via ctx.waitForResponse().
void RocketClient::handleResponseFrame(
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

void RocketClient::handleStreamFrame(
    RocketStreamServerCallback& serverCallback,
    FrameType frameType,
    std::unique_ptr<folly::IOBuf> frame) {
  switch (frameType) {
    case FrameType::PAYLOAD: {
      PayloadFrame payloadFrame{std::move(frame)};
      const auto streamId = payloadFrame.streamId();
      // Note that if the payload frame arrives in fragments, we rely on the
      // last fragment having the right next and/or complete flags set.
      const bool next = payloadFrame.hasNext();
      const bool complete = payloadFrame.hasComplete();
      if (auto fullPayload = bufferOrGetFullPayload(std::move(payloadFrame))) {
        auto& clientCallback = serverCallback.getClientCallback();
        if (next) {
          if (firstResponseTimeouts_.count(streamId)) {
            firstResponseTimeouts_.erase(streamId);
            auto firstResponse =
                unpack<FirstResponsePayload>(std::move(*fullPayload));
            if (firstResponse.hasException()) {
              clientCallback.onFirstResponseError(
                  std::move(firstResponse.exception()));
              freeStream(streamId);
              return;
            }
            clientCallback.onFirstResponse(
                std::move(*firstResponse), &serverCallback);
          } else {
            auto streamPayload = unpack<StreamPayload>(std::move(*fullPayload));
            if (streamPayload.hasException()) {
              clientCallback.onStreamError(
                  std::move(streamPayload.exception()));
              freeStream(streamId);
              return;
            }
            clientCallback.onStreamNext(std::move(*streamPayload));
          }
        }

        if (complete) {
          clientCallback.onStreamComplete();
          freeStream(streamId);
        }
      }
    } break;

    case FrameType::ERROR: {
      ErrorFrame errorFrame{std::move(frame)};
      const auto streamId = errorFrame.streamId();
      auto& clientCallback = serverCallback.getClientCallback();
      auto ew = folly::make_exception_wrapper<RocketException>(
          errorFrame.errorCode(), std::move(errorFrame.payload()).data());
      if (firstResponseTimeouts_.count(streamId)) {
        firstResponseTimeouts_.erase(streamId);
        clientCallback.onFirstResponseError(ew);
      } else {
        clientCallback.onStreamError(ew);
      }
      freeStream(streamId);
    } break;

    default:
      close(folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              NETWORK_ERROR,
          folly::to<std::string>(
              "Client attempting to handle unhandleable frame type: ",
              static_cast<uint8_t>(frameType))));
  }
}

folly::Try<Payload> RocketClient::sendRequestResponseSync(
    Payload&& request,
    std::chrono::milliseconds timeout,
    RocketClientWriteCallback* writeCallback) {
  auto g = makeRequestCountGuard();
  auto setupFrame = std::move(setupFrame_);
  RequestContext ctx(
      RequestResponseFrame(makeStreamId(), std::move(request)),
      queue_,
      setupFrame.get(),
      writeCallback);
  auto swr = scheduleWrite(ctx);
  if (swr.hasException()) {
    return folly::Try<Payload>(std::move(swr.exception()));
  }
  return ctx.waitForResponse(timeout);
}

folly::Try<void> RocketClient::sendRequestFnfSync(
    Payload&& request,
    RocketClientWriteCallback* writeCallback) {
  auto g = makeRequestCountGuard();
  auto setupFrame = std::move(setupFrame_);
  RequestContext ctx(
      RequestFnfFrame(makeStreamId(), std::move(request)),
      queue_,
      setupFrame.get(),
      writeCallback);
  auto swr = scheduleWrite(ctx);
  if (swr.hasException()) {
    return folly::Try<void>(std::move(swr.exception()));
  }
  return ctx.waitForWriteToComplete();
}

void RocketClient::sendRequestStream(
    Payload&& request,
    std::chrono::milliseconds firstResponseTimeout,
    StreamClientCallback* clientCallback) {
  DCHECK(folly::fibers::onFiber());
  const auto streamId = makeStreamId();

  auto setupFrame = std::move(setupFrame_);
  RequestContext ctx(
      RequestStreamFrame(streamId, std::move(request), 1 /* initialRequestN */),
      queue_,
      setupFrame.get(),
      nullptr /* RocketClientWriteCallback */);

  streamsV2_.emplace(
      streamId,
      std::make_unique<RocketStreamServerCallback>(
          streamId, *this, *clientCallback));
  auto g = folly::makeGuard([&] { freeStream(streamId); });

  auto writeScheduled = scheduleWrite(ctx);
  if (writeScheduled.hasException()) {
    return clientCallback->onFirstResponseError(
        std::move(writeScheduled.exception()));
  }

  scheduleFirstResponseTimeout(streamId, firstResponseTimeout);
  auto writeCompleted = ctx.waitForWriteToComplete();
  if (writeCompleted.hasException()) {
    // Look up the stream in the map again. It may have already been erased,
    // e.g., by closeNow() or an error frame that arrived before this fiber had
    // a chance to resume, and we must guarantee that onFirstResponseError() is
    // not called more than once.
    if (getStreamV2ById(streamId)) {
      return clientCallback->onFirstResponseError(
          std::move(writeCompleted.exception()));
    }
  }

  g.dismiss();
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

folly::Try<void> RocketClient::sendRequestNSync(StreamId streamId, int32_t n) {
  if (UNLIKELY(n <= 0)) {
    return {};
  }

  if (!getStreamById(streamId) && !getStreamV2ById(streamId)) {
    return {};
  }

  if (auto* stream = getStreamById(streamId)) {
    if (stream->requestStreamPayload) {
      auto setupFrame = std::move(setupFrame_);
      RequestContext ctx(
          RequestStreamFrame(
              streamId, std::move(*stream->requestStreamPayload), n),
          queue_,
          setupFrame.get());
      stream->requestStreamPayload.reset();
      auto swr = scheduleWrite(ctx);
      if (swr.hasException()) {
        return folly::Try<void>(std::move(swr.exception()));
      }
      return ctx.waitForWriteToComplete();
    }
  }

  RequestContext ctx(RequestNFrame(streamId, n), queue_);
  auto swr = scheduleWrite(ctx);
  if (swr.hasException()) {
    return folly::Try<void>(std::move(swr.exception()));
  }
  return ctx.waitForWriteToComplete();
}

folly::Try<void> RocketClient::sendCancelSync(StreamId streamId) {
  RequestContext ctx(CancelFrame(streamId), queue_);
  SCOPE_EXIT {
    freeStream(streamId);
  };
  auto swr = scheduleWrite(ctx);
  if (swr.hasException()) {
    return folly::Try<void>(std::move(swr.exception()));
  }
  return ctx.waitForWriteToComplete();
}

void RocketClient::sendRequestN(StreamId streamId, int32_t n) {
  auto g = makeRequestCountGuard();
  fm_->addTaskFinally(
      [this, g = std::move(g), streamId, n] {
        return sendRequestNSync(streamId, n);
      },
      [this,
       keepAlive = shared_from_this()](folly::Try<folly::Try<void>>&& result) {
        auto resc = collapseTry(std::move(result));
        if (resc.hasException()) {
          FB_LOG_EVERY_MS(ERROR, 1000) << "sendRequestN failed, closing now: "
                                       << resc.exception().what();
          closeNow(std::move(resc.exception()));
        }
      });
}

void RocketClient::cancelStream(StreamId streamId) {
  auto g = makeRequestCountGuard();
  freeStream(streamId);
  fm_->addTaskFinally(
      [this, g = std::move(g), streamId] { return sendCancelSync(streamId); },
      [this,
       keepAlive = shared_from_this()](folly::Try<folly::Try<void>>&& result) {
        auto resc = collapseTry(std::move(result));
        if (resc.hasException()) {
          FB_LOG_EVERY_MS(ERROR, 1000) << "cancelStream failed, closing now: "
                                       << resc.exception().what();
          closeNow(std::move(resc.exception()));
        }
      });
}

folly::Try<void> RocketClient::scheduleWrite(RequestContext& ctx) {
  if (!evb_) {
    folly::throw_exception(transport::TTransportException(
        transport::TTransportException::TTransportExceptionType::INVALID_STATE,
        "Cannot send requests on a detached client"));
  }

  if (state_ != ConnectionState::CONNECTED) {
    return folly::Try<void>(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::NOT_OPEN,
            "Write not scheduled on disconnected client"));
  }

  queue_.enqueueScheduledWrite(ctx);
  if (!writeLoopCallback_.isLoopCallbackScheduled()) {
    evb_->runInLoop(&writeLoopCallback_);
  }
  return {};
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
  req.onWriteSuccess();
  if (req.expectingResponse()) {
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

  close(folly::make_exception_wrapper<std::runtime_error>(fmt::format(
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

  auto streamsV2 = std::move(streamsV2_);
  for (const auto& idAndCallback : streamsV2) {
    const auto streamId = idAndCallback.first;
    auto& clientCallback = idAndCallback.second.get()->getClientCallback();
    if (firstResponseTimeouts_.count(streamId)) {
      clientCallback.onFirstResponseError(ew);
    } else {
      clientCallback.onStreamError(ew);
    }
  }
  firstResponseTimeouts_.clear();
  bufferedFragments_.clear();
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

RocketStreamServerCallback* RocketClient::getStreamV2ById(StreamId streamId) {
  auto it = streamsV2_.find(streamId);
  return it != streamsV2_.end() ? it->second.get() : nullptr;
}

void RocketClient::freeStream(StreamId streamId) {
  streams_.erase(streamId);
  streamsV2_.erase(streamId);
  bufferedFragments_.erase(streamId);
  firstResponseTimeouts_.erase(streamId);
  notifyIfDetachable();
}

folly::Optional<Payload> RocketClient::bufferOrGetFullPayload(
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

void RocketClient::scheduleFirstResponseTimeout(
    StreamId streamId,
    std::chrono::milliseconds timeout) {
  DCHECK(evb_);
  DCHECK(firstResponseTimeouts_.find(streamId) == firstResponseTimeouts_.end());

  auto firstResponseTimeout =
      std::make_unique<FirstResponseTimeout>(*this, streamId);
  evb_->timer().scheduleTimeout(firstResponseTimeout.get(), timeout);
  firstResponseTimeouts_.emplace(streamId, std::move(firstResponseTimeout));
}

void RocketClient::FirstResponseTimeout::timeoutExpired() noexcept {
  const auto streamIt = client_.streamsV2_.find(streamId_);
  CHECK(streamIt != client_.streamsV2_.end());

  auto& clientCallback = streamIt->second->getClientCallback();
  clientCallback.onFirstResponseError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TIMED_OUT));

  client_.freeStream(streamId_);
}

} // namespace rocket
} // namespace thrift
} // namespace apache
