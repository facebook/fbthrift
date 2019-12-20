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
#include <folly/Overload.h>
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
#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/core/TryUtil.h>
#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RequestContext.h>
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
  if (frameType == FrameType::METADATA_PUSH && streamId == StreamId{0}) {
    // constructing the METADATA_PUSH frame for validation
    MetadataPushFrame mdPushFrame(std::move(frame));
    LOG(WARNING) << "Dropping METADATA_PUSH frame";
    return;
  }

  if (auto* ctx = queue_.getRequestResponseContext(streamId)) {
    DCHECK(ctx->isRequestResponse());
    return handleRequestResponseFrame(*ctx, frameType, std::move(frame));
  }

  handleStreamChannelFrame(streamId, frameType, std::move(frame));
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

void RocketClient::handleStreamChannelFrame(
    StreamId streamId,
    FrameType frameType,
    std::unique_ptr<folly::IOBuf> frame) {
  auto it = streams_.find(streamId);
  if (it == streams_.end()) {
    notifyIfDetachable();
    return;
  }
  StreamChannelStatus status =
      folly::variant_match(*it, [&](const auto& serverCallbackUniquePtr) {
        auto& serverCallback = *serverCallbackUniquePtr;
        switch (frameType) {
          case FrameType::PAYLOAD:
            return this->handlePayloadFrame(serverCallback, std::move(frame));
          case FrameType::ERROR:
            return this->handleErrorFrame(serverCallback, std::move(frame));
          case FrameType::REQUEST_N:
            return this->handleRequestNFrame(serverCallback, std::move(frame));
          case FrameType::CANCEL:
            return this->handleCancelFrame(serverCallback, std::move(frame));
          case FrameType::EXT:
            return this->handleExtFrame(serverCallback, std::move(frame));
          default:
            this->close(
                folly::make_exception_wrapper<transport::TTransportException>(
                    transport::TTransportException::TTransportExceptionType::
                        NETWORK_ERROR,
                    folly::to<std::string>(
                        "Client attempting to handle unhandleable frame type: ",
                        static_cast<uint8_t>(frameType))));
            return StreamChannelStatus::Alive;
        }
      });

  switch (status) {
    case StreamChannelStatus::Alive:
      break;
    case StreamChannelStatus::Complete:
      freeStream(streamId);
      break;
    case StreamChannelStatus::ContractViolation:
      freeStream(streamId);
      close(folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              STREAMING_CONTRACT_VIOLATION,
          "Streaming contract violation. Closing the connection."));
      break;
  };
}

template <typename CallbackType>
StreamChannelStatus RocketClient::handlePayloadFrame(
    CallbackType& serverCallback,
    std::unique_ptr<folly::IOBuf> frame) {
  PayloadFrame payloadFrame{std::move(frame)};
  const auto streamId = payloadFrame.streamId();
  // Note that if the payload frame arrives in fragments, we rely on the
  // last fragment having the right next and/or complete flags set.
  const bool next = payloadFrame.hasNext();
  const bool complete = payloadFrame.hasComplete();
  if (auto fullPayload = bufferOrGetFullPayload(std::move(payloadFrame))) {
    if (firstResponseTimeouts_.count(streamId)) {
      firstResponseTimeouts_.erase(streamId);
      if (!next) {
        serverCallback.onInitialError(
            folly::make_exception_wrapper<transport::TTransportException>(
                transport::TTransportException::TTransportExceptionType::
                    STREAMING_CONTRACT_VIOLATION,
                "Missing initial response"));
        return StreamChannelStatus::ContractViolation;
      }
      auto firstResponse =
          unpack<FirstResponsePayload>(std::move(*fullPayload));
      if (firstResponse.hasException()) {
        serverCallback.onInitialError(std::move(firstResponse.exception()));
        return StreamChannelStatus::Complete;
      }
      serverCallback.onInitialPayload(std::move(*firstResponse), evb_);
      if (complete) {
        // onInitialPayload could have resulted in canceling the stream.
        if (streams_.find(streamId) != streams_.end()) {
          return serverCallback.onStreamComplete();
        }
      }
      return StreamChannelStatus::Alive;
    }
    if (next) {
      auto streamPayload = unpack<StreamPayload>(std::move(*fullPayload));
      if (streamPayload.hasException()) {
        return serverCallback.onStreamError(
            std::move(streamPayload.exception()));
      }
      if (complete) {
        return serverCallback.onStreamFinalPayload(std::move(*streamPayload));
      }
      return serverCallback.onStreamPayload(std::move(*streamPayload));
    }
    if (complete) {
      return serverCallback.onStreamComplete();
    }
    serverCallback.onStreamError(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Both next and complete flags not set"));
    return StreamChannelStatus::ContractViolation;
  }
  return StreamChannelStatus::Alive;
}

template <typename CallbackType>
StreamChannelStatus RocketClient::handleErrorFrame(
    CallbackType& serverCallback,
    std::unique_ptr<folly::IOBuf> frame) {
  ErrorFrame errorFrame{std::move(frame)};
  const auto streamId = errorFrame.streamId();
  auto ew = folly::make_exception_wrapper<thrift::detail::EncodedError>(
      std::move(errorFrame.payload()).data());
  if (firstResponseTimeouts_.count(streamId)) {
    firstResponseTimeouts_.erase(streamId);
    serverCallback.onInitialError(std::move(ew));
    return StreamChannelStatus::Complete;
  }
  return serverCallback.onStreamError(std::move(ew));
}

template <typename CallbackType>
StreamChannelStatus RocketClient::handleRequestNFrame(
    CallbackType& serverCallback,
    std::unique_ptr<folly::IOBuf> frame) {
  RequestNFrame requestNFrame{std::move(frame)};
  if (firstResponseTimeouts_.count(requestNFrame.streamId())) {
    serverCallback.onInitialError(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Missing initial response: handleRequestNFrame"));
    return StreamChannelStatus::ContractViolation;
  }
  return serverCallback.onSinkRequestN(std::move(requestNFrame).requestN());
}

template <typename CallbackType>
StreamChannelStatus RocketClient::handleCancelFrame(
    CallbackType& serverCallback,
    std::unique_ptr<folly::IOBuf> frame) {
  CancelFrame cancelFrame{std::move(frame)};
  if (firstResponseTimeouts_.count(cancelFrame.streamId())) {
    serverCallback.onInitialError(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Missing initial response: handleCancelFrame"));
    return StreamChannelStatus::ContractViolation;
  }
  return serverCallback.onSinkCancel();
}

template <typename CallbackType>
StreamChannelStatus RocketClient::handleExtFrame(
    CallbackType& serverCallback,
    std::unique_ptr<folly::IOBuf> frame) {
  ExtFrame extFrame{std::move(frame)};
  if (firstResponseTimeouts_.count(extFrame.streamId())) {
    serverCallback.onInitialError(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Missing initial response: handleExtFrame"));
    return StreamChannelStatus::ContractViolation;
  }

  if (extFrame.extFrameType() == ExtFrameType::HEADERS_PUSH) {
    auto headersPayload = unpack<HeadersPayload>(std::move(extFrame.payload()));
    if (headersPayload.hasException()) {
      return serverCallback.onStreamError(
          std::move(headersPayload.exception()));
    }
    serverCallback.onStreamHeaders(std::move(*headersPayload));
    return StreamChannelStatus::Alive;
  }

  if (extFrame.hasIgnore()) {
    return StreamChannelStatus::Alive;
  }

  serverCallback.onStreamError(
      folly::make_exception_wrapper<transport::TTransportException>(
          transport::TTransportException::TTransportExceptionType::
              STREAMING_CONTRACT_VIOLATION,
          "Unsupported frame type: handleExtFrame"));
  return StreamChannelStatus::ContractViolation;
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
    std::chrono::milliseconds chunkTimeout,
    int32_t initialRequestN,
    StreamClientCallback* clientCallback) {
  const auto streamId = makeStreamId();
  if (chunkTimeout != std::chrono::milliseconds::zero()) {
    auto serverCallback =
        std::make_unique<RocketStreamServerCallbackWithChunkTimeout>(
            streamId, *this, *clientCallback, chunkTimeout, initialRequestN);
    sendRequestStreamChannel(
        streamId,
        std::move(request),
        firstResponseTimeout,
        initialRequestN,
        std::move(serverCallback));
  } else {
    auto serverCallback = std::make_unique<RocketStreamServerCallback>(
        streamId, *this, *clientCallback);
    sendRequestStreamChannel(
        streamId,
        std::move(request),
        firstResponseTimeout,
        initialRequestN,
        std::move(serverCallback));
  }
}

void RocketClient::sendRequestChannel(
    Payload&& request,
    std::chrono::milliseconds firstResponseTimeout,
    ChannelClientCallback* clientCallback) {
  const auto streamId = makeStreamId();
  auto serverCallback = std::make_unique<RocketChannelServerCallback>(
      streamId, *this, *clientCallback);
  sendRequestStreamChannel(
      streamId,
      std::move(request),
      firstResponseTimeout,
      0,
      std::move(serverCallback));
}

void RocketClient::sendRequestSink(
    Payload&& request,
    std::chrono::milliseconds firstResponseTimeout,
    SinkClientCallback* clientCallback) {
  const auto streamId = makeStreamId();
  auto serverCallback = std::make_unique<RocketSinkServerCallback>(
      streamId, *this, *clientCallback);
  sendRequestStreamChannel(
      streamId,
      std::move(request),
      firstResponseTimeout,
      1,
      std::move(serverCallback));
}

template <typename ServerCallback>
void RocketClient::sendRequestStreamChannel(
    const StreamId& streamId,
    Payload&& request,
    std::chrono::milliseconds firstResponseTimeout,
    int32_t initialRequestN,
    std::unique_ptr<ServerCallback> serverCallback) {
  using Frame = std::conditional_t<
      std::is_same<RocketStreamServerCallback, ServerCallback>::value ||
          std::is_same<
              RocketStreamServerCallbackWithChunkTimeout,
              ServerCallback>::value,
      RequestStreamFrame,
      RequestChannelFrame>;

  // One extra credit for initial response.
  if (initialRequestN < std::numeric_limits<int32_t>::max()) {
    initialRequestN += 1;
  }

  class Context : public folly::fibers::Baton::Waiter {
   public:
    Context(
        RocketClient& client,
        StreamId streamId,
        std::unique_ptr<SetupFrame> setupFrame,
        Frame&& frame,
        RequestContextQueue& queue,
        ServerCallback& serverCallback)
        : client_(client),
          streamId_(streamId),
          setupFrame_(std::move(setupFrame)),
          ctx_(
              std::move(frame),
              queue,
              setupFrame_.get(),
              nullptr /* RocketClientWriteCallback */),
          serverCallback_(serverCallback) {}

    ~Context() {
      if (!complete_) {
        client_.freeStream(streamId_);
      }
    }

    static void run(
        std::unique_ptr<Context> self,
        std::chrono::milliseconds firstResponseTimeout) {
      auto writeScheduled = self->client_.scheduleWrite(self->ctx_);
      if (writeScheduled.hasException()) {
        return self->serverCallback_.onInitialError(
            std::move(writeScheduled.exception()));
      }

      self->client_.scheduleFirstResponseTimeout(
          self->streamId_, firstResponseTimeout);
      auto& ctx = self->ctx_;
      ctx.waitForWriteToCompleteSchedule(self.release());
    }

   private:
    void post() override {
      SCOPE_EXIT {
        delete this;
      };

      auto writeCompleted = ctx_.waitForWriteToCompleteResult();
      if (writeCompleted.hasException()) {
        // Look up the stream in the map again. It may have already been erased,
        // e.g., by closeNow() or an error frame that arrived before this fiber
        // had a chance to resume, and we must guarantee that onInitialError()
        // is not called more than once.
        if (client_.streamExists(streamId_)) {
          return serverCallback_.onInitialError(
              std::move(writeCompleted.exception()));
        }
      }

      complete_ = true;
    }

    RocketClient& client_;
    StreamId streamId_;
    std::unique_ptr<SetupFrame> setupFrame_;
    RequestContext ctx_;
    ServerCallback& serverCallback_;
    bool complete_{false};
  };

  auto serverCallbackPtr = serverCallback.get();
  streams_.emplace(ServerCallbackUniquePtr(std::move(serverCallback)));

  Context::run(
      std::make_unique<Context>(
          *this,
          streamId,
          std::move(setupFrame_),
          Frame(streamId, std::move(request), initialRequestN),
          queue_,
          *serverCallbackPtr),
      firstResponseTimeout);
}

folly::Try<void> RocketClient::sendPayloadSync(
    StreamId streamId,
    Payload&& payload,
    Flags flags) {
  RequestContext ctx(PayloadFrame(streamId, std::move(payload), flags), queue_);
  auto swr = scheduleWrite(ctx);
  if (swr.hasException()) {
    return folly::Try<void>(std::move(swr.exception()));
  }
  return ctx.waitForWriteToComplete();
}

folly::Try<void> RocketClient::sendErrorSync(
    StreamId streamId,
    RocketException&& rex) {
  RequestContext ctx(ErrorFrame(streamId, std::move(rex)), queue_);
  auto swr = scheduleWrite(ctx);
  if (swr.hasException()) {
    return folly::Try<void>(std::move(swr.exception()));
  }
  return ctx.waitForWriteToComplete();
}

template <typename Frame, typename OnError>
bool RocketClient::sendFrame(Frame&& frame, OnError&& onError) {
  using RequestCountGuard = decltype(makeRequestCountGuard());

  class Context : public folly::fibers::Baton::Waiter {
   public:
    Context(RocketClient& client, Frame&& frame, OnError&& onError)
        : client_(client),
          ctx_(std::forward<Frame>(frame), client_.queue_),
          onError_(std::forward<OnError>(onError)),
          g_(client_.makeRequestCountGuard()) {}

    FOLLY_NODISCARD static bool run(std::unique_ptr<Context> self) {
      auto writeScheduled = self->client_.scheduleWrite(self->ctx_);
      if (writeScheduled.hasException()) {
        self->onError_(std::move(writeScheduled.exception()));
        return false;
      }

      auto& ctx = self->ctx_;
      ctx.waitForWriteToCompleteSchedule(self.release());

      return true;
    }

   private:
    void post() override {
      std::unique_ptr<Context> self(this);

      auto writeCompleted = ctx_.waitForWriteToCompleteResult();
      if (writeCompleted.hasException()) {
        onError_(std::move(writeCompleted.exception()));
      }
    }

    RocketClient& client_;
    RequestContext ctx_;
    std::decay_t<OnError> onError_;
    RequestCountGuard g_;
  };

  auto ctx = std::make_unique<Context>(
      *this, std::forward<Frame>(frame), std::forward<OnError>(onError));
  return Context::run(std::move(ctx));
}

bool RocketClient::sendRequestN(StreamId streamId, int32_t n) {
  if (UNLIKELY(n <= 0)) {
    return true;
  }

  DCHECK(streamExists(streamId));

  return sendFrame(
      RequestNFrame(streamId, n),
      [self = shared_from_this()](folly::exception_wrapper ew) {
        FB_LOG_EVERY_MS(ERROR, 1000)
            << "sendRequestN failed, closing now: " << ew.what();
        self->closeNow(std::move(ew));
      });
}

void RocketClient::cancelStream(StreamId streamId) {
  freeStream(streamId);
  std::ignore = sendFrame(
      CancelFrame(streamId),
      [self = shared_from_this()](folly::exception_wrapper ew) {
        FB_LOG_EVERY_MS(ERROR, 1000)
            << "cancelStream failed, closing now: " << ew.what();
        self->closeNow(std::move(ew));
      });
}

void RocketClient::sendPayload(
    StreamId streamId,
    StreamPayload&& payload,
    Flags flags) {
  auto g = makeRequestCountGuard();
  fm_->addTaskFinally(
      [this,
       g = std::move(g),
       streamId,
       payload = pack(std::move(payload)),
       flags]() mutable {
        if (payload.hasException()) {
          return folly::Try<void>(std::move(payload).exception());
        }

        return sendPayloadSync(streamId, std::move(payload.value()), flags);
      },
      [this,
       keepAlive = shared_from_this()](folly::Try<folly::Try<void>>&& result) {
        auto resc = collapseTry(std::move(result));
        if (resc.hasException()) {
          FB_LOG_EVERY_MS(ERROR, 1000)
              << "sendPayload failed, closing now: " << resc.exception().what();
          closeNow(std::move(resc.exception()));
        }
      });
}

void RocketClient::sendError(StreamId streamId, RocketException&& rex) {
  auto g = makeRequestCountGuard();
  freeStream(streamId);
  fm_->addTaskFinally(
      [this, g = std::move(g), streamId, rex = std::move(rex)]() mutable {
        return sendErrorSync(streamId, std::move(rex));
      },
      [this,
       keepAlive = shared_from_this()](folly::Try<folly::Try<void>>&& result) {
        auto resc = collapseTry(std::move(result));
        if (resc.hasException()) {
          FB_LOG_EVERY_MS(ERROR, 1000)
              << "sendError failed, closing now: " << resc.exception().what();
          closeNow(std::move(resc.exception()));
        }
      });
}

void RocketClient::sendComplete(StreamId streamId, bool closeStream) {
  auto g = makeRequestCountGuard();
  if (closeStream) {
    freeStream(streamId);
  }
  sendPayload(
      streamId,
      StreamPayload(std::unique_ptr<folly::IOBuf>{}, {}),
      rocket::Flags::none().complete(true));
}

bool RocketClient::sendExtHeaders(StreamId streamId, HeadersPayload&& payload) {
  auto onError = [self = shared_from_this()](folly::exception_wrapper ew) {
    FB_LOG_EVERY_MS(ERROR, 1000)
        << "sendExtHeaders failed, closing now: " << ew.what();
    self->closeNow(std::move(ew));
  };
  auto packedPayload = pack(std::move(payload));
  if (packedPayload.hasException()) {
    onError(std::move(packedPayload.exception()));
    return false;
  }

  return sendFrame(
      ExtFrame(
          streamId,
          std::move(packedPayload.value()),
          rocket::Flags::none().ignore(true),
          ExtFrameType::HEADERS_PUSH),
      std::move(onError));
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
    if (flushList_) {
      flushList_->push_back(writeLoopCallback_);
    } else {
      evb_->runInLoop(&writeLoopCallback_);
    }
  }
  return {};
}

void RocketClient::WriteLoopCallback::runLoopCallback() noexcept {
  if (!client_.flushList_ && !std::exchange(rescheduled_, true)) {
    client_.evb_->runInLoop(this, true /* thisIteration */);
    return;
  }
  rescheduled_ = false;
  client_.writeScheduledRequestsToSocket();
}

void RocketClient::writeScheduledRequestsToSocket() noexcept {
  DestructorGuard dg(this);

  if (state_ == ConnectionState::CONNECTED) {
    auto buf = queue_.getNextScheduledWritesBatch();

    if (buf) {
      socket_->writeChain(this, std::move(buf), folly::WriteFlags::NONE);
    }
  }

  notifyIfDetachable();
}

void RocketClient::writeSuccess() noexcept {
  DestructorGuard dg(this);
  DCHECK(state_ != ConnectionState::CLOSED);

  queue_.markNextSendingBatchAsSent([&](auto& req) {
    req.onWriteSuccess();
    if (req.isRequestResponse()) {
      req.scheduleTimeoutForResponse();
    } else {
      queue_.markAsResponded(req);
    }
  });

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

  queue_.markNextSendingBatchAsSent([](auto&&) {});

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
  // invalidating any outstanding iterators. Also, since the client is shutting
  // down now, we don't bother with notifyIfDetachable().
  auto streams = std::move(streams_);
  for (const auto& callback : streams) {
    folly::variant_match(callback, [&](const auto& serverCallback) {
      if (firstResponseTimeouts_.count(serverCallback->streamId())) {
        serverCallback->onInitialError(ew);
      } else {
        serverCallback->onStreamTransportError(ew);
      }
    });
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

bool RocketClient::streamExists(StreamId streamId) const {
  return streams_.find(streamId) != streams_.end();
}

void RocketClient::freeStream(StreamId streamId) {
  streams_.erase(streamId);
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
  const auto streamIt = client_.streams_.find(streamId_);
  CHECK(streamIt != client_.streams_.end());

  folly::variant_match(*streamIt, [&](const auto& serverCallback) {
    serverCallback->onInitialError(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TIMED_OUT));
  });
}

void RocketClient::attachEventBase(folly::EventBase& evb) {
  DCHECK(!evb_);
  evb.dcheckIsInEventBaseThread();

  evb_ = &evb;
  fm_ = &folly::fibers::getFiberManager(*evb_);
  socket_->attachEventBase(evb_);
  evb_->runOnDestruction(*eventBaseDestructionCallback_);
}

void RocketClient::detachEventBase() {
  DCHECK(evb_);
  evb_->dcheckIsInEventBaseThread();
  DCHECK(!writeLoopCallback_.isLoopCallbackScheduled());
  eventBaseDestructionCallback_->cancel();
  socket_->detachEventBase();
  fm_ = nullptr;
  evb_ = nullptr;
  flushList_ = nullptr;
}

} // namespace rocket
} // namespace thrift
} // namespace apache
