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
#include <limits>
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
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <folly/lang/Exception.h>

#include <thrift/lib/cpp/transport/TTransportException.h>
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

THRIFT_FLAG_DEFINE_int64(rocket_server_version_timeout_ms, 500);

namespace {
folly::exception_wrapper err(folly::Try<void> t) {
  return t.hasException() ? std::move(t.exception())
                          : folly::exception_wrapper();
}

transport::TTransportException toTransportException(
    folly::exception_wrapper ew) {
  transport::TTransportException result;
  if (ew.with_exception<transport::TTransportException>(
          [&](transport::TTransportException ex) { result = std::move(ex); })) {
    return result;
  }

  return transport::TTransportException(folly::exceptionStr(ew).toStdString());
}
} // namespace

folly::EventBaseLocal<RocketClient::FlushManager>&
RocketClient::getEventBaseLocal() {
  static folly::Indestructible<
      folly::EventBaseLocal<RocketClient::FlushManager>>
      evbLocal;
  return *evbLocal;
}

RocketClient::RocketClient(
    folly::EventBase& evb,
    folly::AsyncTransport::UniquePtr socket,
    std::unique_ptr<SetupFrame> setupFrame)
    : evb_(&evb),
      writeLoopCallback_(*this),
      socket_(std::move(socket)),
      parser_(*this),
      detachableLoopCallback_(*this),
      closeLoopCallback_(*this),
      eventBaseDestructionCallback_(*this),
      setupFrame_(std::move(setupFrame)) {
  DCHECK(socket_ != nullptr);
  socket_->setReadCB(&parser_);
  if (auto socket = dynamic_cast<folly::AsyncSocket*>(socket_.get())) {
    socket->setCloseOnFailedWrite(false);
  }
  evb_->runOnDestruction(eventBaseDestructionCallback_);
  // Get or create flush manager from EventBaseLocal
  flushManager_ = &FlushManager::getInstance(*evb_);
}

RocketClient::~RocketClient() {
  closeNow(transport::TTransportException("Destroying RocketClient"));
  eventBaseDestructionCallback_.cancel();
  detachableLoopCallback_.cancelLoopCallback();

  // All outstanding request contexts should have been cleaned up in closeNow()
  DCHECK(streams_.empty());
}

RocketClient::Ptr RocketClient::create(
    folly::EventBase& evb,
    folly::AsyncTransport::UniquePtr socket,
    std::unique_ptr<SetupFrame> setupFrame) {
  return Ptr(new RocketClient(evb, std::move(socket), std::move(setupFrame)));
}

void RocketClient::handleFrame(std::unique_ptr<folly::IOBuf> frame) {
  DestructorGuard dg(this);

  folly::io::Cursor cursor(frame.get());

  const auto streamId = readStreamId(cursor);
  FrameType frameType;
  std::tie(frameType, std::ignore) = readFrameTypeAndFlags(cursor);

  if (UNLIKELY(frameType == FrameType::ERROR && streamId == StreamId{0})) {
    ErrorFrame errorFrame(std::move(frame));
    handleError(RocketException(
        errorFrame.errorCode(), std::move(errorFrame.payload()).data()));
    return;
  }
  if (frameType == FrameType::METADATA_PUSH && streamId == StreamId{0}) {
    MetadataPushFrame mdPushFrame(std::move(frame));
    if (!mdPushFrame.metadata()) {
      return;
    }
    ServerPushMetadata serverMeta;
    try {
      unpackCompact(serverMeta, std::move(mdPushFrame.metadata()));
    } catch (...) {
      close(transport::TTransportException(
          transport::TTransportException::CORRUPTED_DATA,
          "Failed to deserialize metadata push frame"));
      return;
    }
    switch (serverMeta.getType()) {
      case ServerPushMetadata::setupResponse: {
        setServerVersion(std::min(
            serverMeta.setupResponse_ref()->version_ref().value_or(0),
            (int32_t)std::numeric_limits<int16_t>::max()));
        break;
      }
      case ServerPushMetadata::streamHeadersPush: {
        DCHECK(serverVersion_ == -1 || serverVersion_ >= 7);
        StreamId sid(
            serverMeta.streamHeadersPush_ref()->streamId_ref().value_or(0));
        auto it = streams_.find(sid);
        if (it != streams_.end()) {
          it->match([&](auto* serverCallbackPtr) {
            serverCallbackPtr->onStreamHeaders(
                HeadersPayload(serverMeta.streamHeadersPush_ref()
                                   ->headersPayloadContent_ref()
                                   .value_or({})));
          });
        }
        return;
      }
      case ServerPushMetadata::drainCompletePush: {
        DCHECK(serverVersion_ == -1 || serverVersion_ >= 7);
        auto drainCode =
            serverMeta.drainCompletePush_ref()->drainCompleteCode_ref();
        if (drainCode &&
            *drainCode == DrainCompleteCode::EXCEEDED_INGRESS_MEM_LIMIT) {
          handleError(RocketException(ErrorCode::EXCEEDED_INGRESS_MEM_LIMIT));
        } else {
          handleError(RocketException(ErrorCode::CONNECTION_DRAIN_COMPLETE));
        }
        return;
      }
      default:
        break;
    }
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
    case FrameType::PAYLOAD: {
      PayloadFrame payloadFrame(std::move(frame));
      if (!payloadFrame.hasNext() || !payloadFrame.hasComplete()) {
        return close(transport::TTransportException(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Client received single response payload without next or "
            "complete flag"));
      }
      return ctx.onPayloadFrame(std::move(payloadFrame));
    }
    case FrameType::ERROR: {
      // Protocol specifies that partial PAYLOAD cannot have arrived before an
      // ERROR frame.
      if (ctx.hasPartialPayload()) {
        return close(transport::TTransportException(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Partial payload has arrived before an error frame."));
      }
      return ctx.onErrorFrame(ErrorFrame(std::move(frame)));
    }
    default:
      close(transport::TTransportException(
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
  StreamChannelStatus status = it->match([&](auto* serverCallbackPtr) {
    auto& serverCallback = *serverCallbackPtr;
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
        this->close(transport::TTransportException(
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
      close(transport::TTransportException(
          transport::TTransportException::TTransportExceptionType::
              STREAMING_CONTRACT_VIOLATION,
          "Streaming contract violation. Closing the connection."));
      break;
  };
}

template <typename CallbackType>
StreamChannelStatus RocketClient::handlePayloadFrame(
    CallbackType& serverCallback, std::unique_ptr<folly::IOBuf> frame) {
  PayloadFrame payloadFrame{std::move(frame)};
  const auto streamId = payloadFrame.streamId();
  // Note that if the payload frame arrives in fragments, we rely on the
  // last fragment having the right next and/or complete flags set.
  const bool next = payloadFrame.hasNext();
  const bool complete = payloadFrame.hasComplete();
  if (auto fullPayload = bufferOrGetFullPayload(std::move(payloadFrame))) {
    if (isFirstResponse(streamId)) {
      acknowledgeFirstResponse(streamId);
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
      auto status =
          serverCallback.onInitialPayload(std::move(*firstResponse), evb_);
      if (status != StreamChannelStatus::Alive) {
        return status;
      }
      if (complete) {
        return serverCallback.onStreamComplete();
      }
      return StreamChannelStatus::Alive;
    }
    if (next) {
      auto streamPayload = unpack<StreamPayload>(std::move(*fullPayload));
      if (streamPayload.hasException()) {
        return serverCallback.onStreamError(
            std::move(streamPayload.exception()));
      }
      auto payloadMetadataRef = streamPayload->metadata.payloadMetadata_ref();
      if (payloadMetadataRef &&
          payloadMetadataRef->getType() == PayloadMetadata::exceptionMetadata) {
        return serverCallback.onStreamError(
            apache::thrift::detail::EncodedStreamError(
                std::move(streamPayload.value())));
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
    CallbackType& serverCallback, std::unique_ptr<folly::IOBuf> frame) {
  ErrorFrame errorFrame{std::move(frame)};
  const auto streamId = errorFrame.streamId();
  auto ew = folly::make_exception_wrapper<RocketException>(
      errorFrame.errorCode(), std::move(errorFrame.payload()).data());
  if (isFirstResponse(streamId)) {
    acknowledgeFirstResponse(streamId);
    serverCallback.onInitialError(std::move(ew));
    return StreamChannelStatus::Complete;
  }
  return serverCallback.onStreamError(std::move(ew));
}

template <typename CallbackType>
StreamChannelStatus RocketClient::handleRequestNFrame(
    CallbackType& serverCallback, std::unique_ptr<folly::IOBuf> frame) {
  RequestNFrame requestNFrame{std::move(frame)};
  if (isFirstResponse(requestNFrame.streamId())) {
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
    CallbackType& serverCallback, std::unique_ptr<folly::IOBuf> frame) {
  CancelFrame cancelFrame{std::move(frame)};
  if (isFirstResponse(cancelFrame.streamId())) {
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
    CallbackType& serverCallback, std::unique_ptr<folly::IOBuf> frame) {
  ExtFrame extFrame{std::move(frame)};
  if (isFirstResponse(extFrame.streamId())) {
    serverCallback.onInitialError(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::
                STREAMING_CONTRACT_VIOLATION,
            "Missing initial response: handleExtFrame"));
    return StreamChannelStatus::ContractViolation;
  }

  if (extFrame.extFrameType() == ExtFrameType::HEADERS_PUSH) {
    DCHECK(serverVersion_ == -1 || serverVersion_ >= 7);
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

  close(transport::TTransportException(
      transport::TTransportException::TTransportExceptionType::NOT_SUPPORTED,
      fmt::format(
          "Received unhandleable ext frame type ({}) without ignore flag",
          static_cast<uint32_t>(extFrame.extFrameType()))));
  return StreamChannelStatus::ContractViolation;
}

void RocketClient::handleError(RocketException&& rex) {
  auto enrichMsg = [](const char* msg, RocketException& rex) {
    if (auto errorData = rex.moveErrorData()) {
      return fmt::format(
          "{}: {} [{}]",
          msg,
          folly::StringPiece(errorData->coalesce()),
          toString(rex.getErrorCode()));
    }
    return fmt::format("{} [{}]", msg, toString(rex.getErrorCode()));
  };
  folly::exception_wrapper ew;
  switch (rex.getErrorCode()) {
    case ErrorCode::CONNECTION_CLOSE: {
      if (clientState_.connState != ConnectionState::CONNECTED) {
        return;
      }

      clientState_.connState = ConnectionState::CLOSING;

      writeLoopCallback_.cancelLoopCallback();
      queue_.failAllScheduledWrites(transport::TTransportException(
          transport::TTransportException::NOT_OPEN,
          "Connection closed by server"));

      notifyIfDetachable();

      return;
    }
    case ErrorCode::INVALID_SETUP: {
      ew = transport::TTransportException(
          transport::TTransportException::NOT_OPEN,
          enrichMsg("Connection setup failed", rex));
      break;
    }
    case ErrorCode::CONNECTION_DRAIN_COMPLETE: {
      ew = transport::TTransportException(
          transport::TTransportException::NOT_OPEN,
          enrichMsg("Server shutdown", rex));
      break;
    }
    case ErrorCode::EXCEEDED_INGRESS_MEM_LIMIT: {
      ew = std::move(rex);
      break;
    }
    default: {
      close(transport::TTransportException(
          transport::TTransportException::END_OF_FILE,
          enrichMsg("Unhandled error frame on control stream", rex)));
      return;
    }
  }
  if (clientState_.connState == ConnectionState::ERROR) {
    error_ = std::move(ew);
  } else {
    close(std::move(ew));
  }
}

folly::Try<Payload> RocketClient::sendRequestResponseSync(
    Payload&& request,
    std::chrono::milliseconds timeout,
    WriteSuccessCallback* writeSuccessCallback) {
  DestructorGuard dg(this);
  auto g = makeRequestCountGuard(RequestType::CLIENT);
  auto setupFrame = moveOutSetupFrame();
  RequestContext ctx(
      RequestResponseFrame(makeStreamId(), std::move(request)),
      queue_,
      setupFrame.get(),
      writeSuccessCallback);
  if (auto ew = err(scheduleWrite(ctx))) {
    return folly::Try<Payload>(std::move(ew));
  }
  return ctx.waitForResponse(timeout);
}

void RocketClient::sendRequestResponse(
    Payload&& request,
    std::chrono::milliseconds timeout,
    std::unique_ptr<RequestResponseCallback> callback) {
  auto setupFrame = moveOutSetupFrame();
  auto rctx = std::make_unique<RequestContext>(
      RequestResponseFrame(makeStreamId(), std::move(request)),
      queue_,
      setupFrame.get(),
      callback.get());
  auto callbackWithGuard = [dg = DestructorGuard(this),
                            g = makeRequestCountGuard(RequestType::CLIENT),
                            callback =
                                std::move(callback)](auto&& response) mutable {
    callback->onResponsePayload(std::move(response));
  };

  using CallbackWithGuard = decltype(callbackWithGuard);
  class Context : public folly::fibers::Baton::Waiter,
                  public folly::HHWheelTimer::Callback {
   public:
    Context(std::unique_ptr<RequestContext> rctx, CallbackWithGuard&& callback)
        : callback_(std::move(callback)), rctx_(std::move(rctx)) {}

    void send(RocketClient& client, std::chrono::milliseconds timeout) && {
      if (auto ew = err(client.scheduleWrite(*rctx_))) {
        SCOPE_EXIT { delete this; };
        return callback_(folly::Try<rocket::Payload>(std::move(ew)));
      }
      rctx_->setTimeoutInfo(client.evb_->timer(), *this, timeout);
      rctx_->waitForWriteToCompleteSchedule(this);
    }

   private:
    // DestructorGuard held by callback_ is needed to avoid freeing RocketClient
    // within the invocation of callback_.
    CallbackWithGuard callback_;
    std::unique_ptr<RequestContext> rctx_;

    void post() override {
      // On the timeout path, post() will be called once more in
      // abortSentRequest() within getResponse(). Avoid recursive misbehavior.
      if (auto rctx = std::move(rctx_)) {
        SCOPE_EXIT { delete this; };
        cancelTimeout();
        callback_(std::move(*rctx).getResponse());
      }
    }

    void timeoutExpired() noexcept override { post(); }
  };

  auto* context = new Context(std::move(rctx), std::move(callbackWithGuard));
  std::move(*context).send(*this, timeout);
}

folly::Try<void> RocketClient::sendRequestFnfSync(Payload&& request) {
  CHECK(folly::fibers::onFiber());
  DestructorGuard dg(this);
  auto g = makeRequestCountGuard(RequestType::CLIENT);
  auto setupFrame = moveOutSetupFrame();
  RequestContext ctx(
      RequestFnfFrame(makeStreamId(), std::move(request)),
      queue_,
      setupFrame.get());
  if (auto ew = err(scheduleWrite(ctx))) {
    return folly::Try<void>(std::move(ew));
  }
  return ctx.waitForWriteToComplete();
}

void RocketClient::sendRequestFnf(
    Payload&& request, std::unique_ptr<RequestFnfCallback> callback) {
  auto setupFrame = moveOutSetupFrame();
  auto rctx = std::make_unique<RequestContext>(
      RequestFnfFrame(makeStreamId(), std::move(request)),
      queue_,
      setupFrame.get());
  auto callbackWithGuard =
      [dg = DestructorGuard(this),
       g = makeRequestCountGuard(RequestType::CLIENT),
       callback = std::move(callback)](auto&& writeResult) mutable {
        callback->onWrite(std::move(writeResult));
      };

  using CallbackWithGuard = decltype(callbackWithGuard);
  class Context : public folly::fibers::Baton::Waiter {
   public:
    Context(std::unique_ptr<RequestContext> rctx, CallbackWithGuard&& callback)
        : callback_(std::move(callback)), rctx_(std::move(rctx)) {}

    void send(RocketClient& client) && {
      if (auto ew = err(client.scheduleWrite(*rctx_))) {
        SCOPE_EXIT { delete this; };
        return callback_(folly::Try<void>(std::move(ew)));
      }
      rctx_->waitForWriteToCompleteSchedule(this);
    }

   private:
    // DestructorGuard held by callback_ is needed to avoid freeing
    // RocketClient within the invocation of callback_.
    CallbackWithGuard callback_;
    std::unique_ptr<RequestContext> rctx_;

    void post() override {
      SCOPE_EXIT { delete this; };
      callback_(rctx_->waitForWriteToCompleteResult());
    }
  };

  auto* context = new Context(std::move(rctx), std::move(callbackWithGuard));
  std::move(*context).send(*this);
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

void RocketClient::sendRequestSink(
    Payload&& request,
    std::chrono::milliseconds firstResponseTimeout,
    SinkClientCallback* clientCallback,
    bool pageAligned,
    folly::Optional<CompressionConfig> compressionConfig) {
  const auto streamId = makeStreamId();

  std::unique_ptr<CompressionConfig> compressionConfigP;
  if (compressionConfig.has_value()) {
    compressionConfigP =
        std::make_unique<CompressionConfig>(*compressionConfig);
  }
  auto serverCallback = std::make_unique<RocketSinkServerCallback>(
      streamId,
      *this,
      *clientCallback,
      pageAligned,
      std::move(compressionConfigP));
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
              nullptr /* writeCallback */),
          serverCallback_(serverCallback) {}

    ~Context() {
      if (!complete_) {
        client_.freeStream(streamId_);
      }
    }

    static void run(
        std::unique_ptr<Context> self,
        std::chrono::milliseconds firstResponseTimeout) {
      if (auto ew = err(self->client_.scheduleWrite(self->ctx_))) {
        return self->serverCallback_.onInitialError(std::move(ew));
      }

      self->client_.maybeScheduleFirstResponseTimeout(
          self->streamId_, firstResponseTimeout);
      auto& ctx = self->ctx_;
      ctx.waitForWriteToCompleteSchedule(self.release());
    }

   private:
    void post() override {
      SCOPE_EXIT { delete this; };

      auto writeCompleted = ctx_.waitForWriteToCompleteResult();
      // If the write failed, check that the stream has not already received the
      // first response. The writeErr() callback for a batch of requests may
      // fire after the initial payload/error has already been processed.
      if (writeCompleted.hasException() && client_.isFirstResponse(streamId_)) {
        return serverCallback_.onInitialError(
            std::move(writeCompleted.exception()));
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
          moveOutSetupFrame(),
          Frame(streamId, std::move(request), initialRequestN),
          queue_,
          *serverCallbackPtr),
      firstResponseTimeout);
}

template <class OnError>
class RocketClient::SendFrameContext : public folly::fibers::Baton::Waiter {
 public:
  template <class Frame>
  SendFrameContext(RocketClient& client, Frame&& frame, OnError&& onError)
      : client_(client),
        ctx_(std::forward<Frame>(frame), client_.queue_),
        onError_(std::forward<OnError>(onError)) {}

  template <class InitFunc>
  SendFrameContext(
      RocketClient& client,
      InitFunc&& initFunc,
      StreamId streamId,
      OnError&& onError)
      : client_(client),
        ctx_(
            std::forward<InitFunc>(initFunc),
            client_.serverVersion_,
            streamId,
            client_.queue_),
        onError_(std::forward<OnError>(onError)) {
    // if server version is not yet available, start buffering requests
    if (UNLIKELY(ctx_.state() == RequestContext::State::DEFERRED_INIT)) {
      client_.onServerVersionRequired();
    }
  }

  FOLLY_NODISCARD static bool run(std::unique_ptr<SendFrameContext> self) {
    if (auto ew = err(self->client_.scheduleWrite(self->ctx_))) {
      self->onError_(toTransportException(std::move(ew)));
      return false;
    }
    auto& ctx = self->ctx_;
    ctx.waitForWriteToCompleteSchedule(self.release());
    return true;
  }

 private:
  void post() override {
    std::unique_ptr<SendFrameContext> self(this);
    auto writeCompleted = ctx_.waitForWriteToCompleteResult();
    if (writeCompleted.hasException()) {
      self->onError_(
          toTransportException(std::move(writeCompleted.exception())));
    }
  }

  RocketClient& client_;
  RequestContext ctx_;
  std::decay_t<OnError> onError_;
};

template <typename Frame, typename OnError>
bool RocketClient::sendFrame(Frame&& frame, OnError&& onError) {
  using Context = SendFrameContext<OnError>;
  auto ctx = std::make_unique<Context>(
      *this, std::forward<Frame>(frame), std::forward<OnError>(onError));
  return Context::run(std::move(ctx));
}

template <typename DeferredInitFunc, typename OnError>
bool RocketClient::sendVersionDependentFrame(
    DeferredInitFunc&& deferredInit, StreamId streamId, OnError&& onError) {
  using Context = SendFrameContext<OnError>;
  auto ctx = std::make_unique<Context>(
      *this,
      std::forward<DeferredInitFunc>(deferredInit),
      streamId,
      std::forward<OnError>(onError));
  return Context::run(std::move(ctx));
}

bool RocketClient::sendRequestN(StreamId streamId, int32_t n) {
  auto g = makeRequestCountGuard(RequestType::INTERNAL);
  if (UNLIKELY(n <= 0)) {
    return true;
  }

  DCHECK(streamExists(streamId));

  return sendFrame(
      RequestNFrame(streamId, n),
      [dg = DestructorGuard(this), this, g = std::move(g)](
          transport::TTransportException ex) {
        FB_LOG_EVERY_MS(ERROR, 1000)
            << "sendRequestN failed, closing now: " << ex.what();
        close(std::move(ex));
      });
}

void RocketClient::cancelStream(StreamId streamId) {
  auto g = makeRequestCountGuard(RequestType::INTERNAL);
  freeStream(streamId);
  std::ignore = sendFrame(
      CancelFrame(streamId),
      [dg = DestructorGuard(this), this, g = std::move(g)](
          transport::TTransportException ex) {
        FB_LOG_EVERY_MS(ERROR, 1000)
            << "cancelStream failed, closing now: " << ex.what();
        close(std::move(ex));
      });
}

void RocketClient::sendPayload(
    StreamId streamId, StreamPayload&& payload, Flags flags) {
  std::ignore = sendFrame(
      PayloadFrame(streamId, pack(std::move(payload)), flags),
      [this,
       dg = DestructorGuard(this),
       g = makeRequestCountGuard(RequestType::INTERNAL)](
          transport::TTransportException ex) {
        FB_LOG_EVERY_MS(ERROR, 1000)
            << "sendPayload failed, closing now: " << ex.what();
        close(std::move(ex));
      });
}

void RocketClient::sendError(StreamId streamId, RocketException&& rex) {
  freeStream(streamId);
  std::ignore = sendFrame(
      ErrorFrame(streamId, std::move(rex)),
      [this,
       dg = DestructorGuard(this),
       g = makeRequestCountGuard(RequestType::INTERNAL)](
          transport::TTransportException ex) {
        FB_LOG_EVERY_MS(ERROR, 1000)
            << "sendError failed, closing now: " << ex.what();
        close(std::move(ex));
      });
}

void RocketClient::sendComplete(StreamId streamId, bool closeStream) {
  auto g = makeRequestCountGuard(RequestType::INTERNAL);
  if (closeStream) {
    freeStream(streamId);
  }
  sendPayload(
      streamId,
      StreamPayload(std::unique_ptr<folly::IOBuf>{}, {}),
      rocket::Flags::none().complete(true));
}

bool RocketClient::sendHeadersPush(
    StreamId streamId, HeadersPayload&& payload) {
  auto g = makeRequestCountGuard(RequestType::INTERNAL);
  auto onError = [dg = DestructorGuard(this), this, g = std::move(g)](
                     transport::TTransportException ex) {
    FB_LOG_EVERY_MS(ERROR, 1000)
        << "sendHeadersPush failed, closing now: " << ex.what();
    close(std::move(ex));
  };
  return sendVersionDependentFrame(
      [=, payload = std::move(payload)](int32_t serverVersion) mutable {
        if (serverVersion >= 7) {
          ClientPushMetadata clientMeta;
          clientMeta.streamHeadersPush_ref().ensure().streamId_ref() =
              static_cast<uint32_t>(streamId);
          clientMeta.streamHeadersPush_ref()->headersPayloadContent_ref() =
              std::move(payload.payload);
          return std::make_pair<std::unique_ptr<folly::IOBuf>, FrameType>(
              MetadataPushFrame::makeFromMetadata(
                  packCompact(std::move(clientMeta)))
                  .serialize(),
              FrameType::METADATA_PUSH);
        } else {
          return std::make_pair<std::unique_ptr<folly::IOBuf>, FrameType>(
              ExtFrame(
                  streamId,
                  pack(std::move(payload)),
                  rocket::Flags::none().ignore(true),
                  ExtFrameType::HEADERS_PUSH)
                  .serialize(),
              FrameType::EXT);
        }
      },
      streamId,
      std::move(onError));
}

bool RocketClient::sendSinkError(StreamId streamId, StreamPayload&& payload) {
  freeStream(streamId);
  auto g = makeRequestCountGuard(RequestType::INTERNAL);
  auto onError = [dg = DestructorGuard(this), this, g = std::move(g)](
                     transport::TTransportException ex) {
    FB_LOG_EVERY_MS(ERROR, 1000)
        << "sendSinkError failed, closing now: " << ex.what();
    close(std::move(ex));
  };
  return sendVersionDependentFrame(
      [streamId = streamId,
       payload = std::move(payload)](int32_t serverVersion) mutable {
        if (serverVersion >= 8) {
          return std::make_pair<std::unique_ptr<folly::IOBuf>, FrameType>(
              PayloadFrame(
                  streamId,
                  pack(std::move(payload)),
                  Flags::none().next(true).complete(true))
                  .serialize(),
              FrameType::PAYLOAD);
        } else {
          return std::make_pair<std::unique_ptr<folly::IOBuf>, FrameType>(
              ErrorFrame(
                  streamId,
                  RocketException(
                      ErrorCode::APPLICATION_ERROR, std::move(payload.payload)))
                  .serialize(),
              FrameType::ERROR);
        }
      },
      streamId,
      std::move(onError));
}

void RocketClient::sendExtAlignedPage(
    StreamId streamId, std::unique_ptr<folly::IOBuf> payload, Flags flags) {
  auto g = makeRequestCountGuard(RequestType::INTERNAL);
  auto onError = [dg = DestructorGuard(this), this, g = std::move(g)](
                     transport::TTransportException ex) {
    FB_LOG_EVERY_MS(ERROR, 1000)
        << "sendExtAlignedPage failed, closing now: " << ex.what();
    close(std::move(ex));
  };

  std::ignore = sendFrame(
      ExtFrame(
          streamId,
          Payload::makeFromData(std::move(payload)),
          flags.ignore(true),
          ExtFrameType::ALIGNED_PAGE),
      std::move(onError));
}

folly::Try<void> RocketClient::scheduleWrite(RequestContext& ctx) {
  if (!evb_) {
    return folly::Try<
        void>(folly::make_exception_wrapper<transport::TTransportException>(
        transport::TTransportException::TTransportExceptionType::INVALID_STATE,
        "Cannot send requests on a detached client"));
  }

  if (clientState_.connState != ConnectionState::CONNECTED) {
    return folly::Try<void>(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TTransportExceptionType::NOT_OPEN,
            "Write not scheduled on disconnected client"));
  }

  queue_.enqueueScheduledWrite(ctx);
  scheduleWriteLoopCallback();
  return {};
}

StreamId RocketClient::makeStreamId() {
  StreamId id;
  do {
    id = nextStreamId_;
    nextStreamId_ += 2;
  } while (clientState_.hitMaxStreamId && streams_.contains(id));

  if (UNLIKELY(id == StreamId::maxClientStreamId())) {
    nextStreamId_ = StreamId(1);
    clientState_.hitMaxStreamId = true;
  }
  return id;
}

void RocketClient::FlushManager::runLoopCallback() noexcept {
  // always reschedule until the end of event loop.
  if (!std::exchange(rescheduled_, true)) {
    evb_.runInLoop(this, true /* thisIteration */);
    return;
  }
  rescheduled_ = false;

  auto cbs = std::move(flushList_);
  while (!cbs.empty()) {
    auto callback = &cbs.front();
    cbs.pop_front();
    callback->runLoopCallback();
  }
}

void RocketClient::FlushManager::enqueueFlush(RocketClient& client) {
  // add write callback to flush list and schedule flush manager callback
  flushList_.push_back(client.writeLoopCallback_);
  if (!isLoopCallbackScheduled()) {
    evb_.runInLoop(this);
  }
}

void RocketClient::WriteLoopCallback::runLoopCallback() noexcept {
  client_.writeScheduledRequestsToSocket();
}

void RocketClient::scheduleWriteLoopCallback() {
  if (!writeLoopCallback_.isLoopCallbackScheduled()) {
    if (flushList_) {
      flushList_->push_back(writeLoopCallback_);
    } else {
      // use FlushManager to handle scheduling
      flushManager_->enqueueFlush(*this);
    }
  }
}

void RocketClient::writeScheduledRequestsToSocket() noexcept {
  DestructorGuard dg(this);

  if (clientState_.connState == ConnectionState::CONNECTED) {
    auto buf = queue_.getNextScheduledWritesBatch();

    if (buf) {
      socket_->writeChain(this, std::move(buf), folly::WriteFlags::NONE);
    }
  }

  notifyIfDetachable();
}

void RocketClient::writeSuccess() noexcept {
  DestructorGuard dg(this);
  DCHECK(clientState_.connState != ConnectionState::CLOSED);

  queue_.markNextSendingBatchAsSent([&](auto& req) {
    req.onWriteSuccess();
    if (!req.isRequestResponse()) {
      queue_.markAsResponded(req);
    }
  });
}

void RocketClient::writeErr(
    size_t bytesWritten, const folly::AsyncSocketException& ex) noexcept {
  DestructorGuard dg(this);
  DCHECK(clientState_.connState != ConnectionState::CLOSED);

  queue_.markNextSendingBatchAsSent([&](auto& req) {
    if (bytesWritten == 0) {
      queue_.abortSentRequest(
          req,
          transport::TTransportException(
              transport::TTransportException::NOT_OPEN,
              fmt::format(
                  "Failed to write to remote endpoint. Wrote 0 bytes."
                  " AsyncSocketException: {}",
                  ex.what())));
    }
  });

  close(transport::TTransportException(
      transport::TTransportException::UNKNOWN,
      fmt::format(
          "Failed to write to remote endpoint. Wrote {} bytes."
          " AsyncSocketException: {}",
          bytesWritten,
          ex.what())));
}

void RocketClient::closeNow(transport::TTransportException ex) noexcept {
  DestructorGuard dg(this);

  if (clientState_.connState == ConnectionState::CLOSED) {
    return;
  }

  close(ex);
  closeLoopCallback_.cancelLoopCallback();
  closeNowImpl();
}

void RocketClient::close(folly::exception_wrapper ew) noexcept {
  DestructorGuard dg(this);

  if (clientState_.connState != ConnectionState::CONNECTED &&
      clientState_.connState != ConnectionState::CLOSING) {
    return;
  }

  error_ = std::move(ew);
  clientState_.connState = ConnectionState::ERROR;

  writeLoopCallback_.cancelLoopCallback();
  queue_.failAllScheduledWrites(error_);

  if (evb_) {
    evb_->runInLoop(&closeLoopCallback_);
  }
}

void RocketClient::closeNowImpl() noexcept {
  DestructorGuard dg(this);
  DCHECK(clientState_.connState == ConnectionState::ERROR);

  DCHECK(socket_);
  socket_->closeNow();
  // AsyncSocket::closeNow() may not unset the read callback. AsyncSocket
  // destruction may also be delayed past RocketClient destruction. Unset the
  // read callback to ensure that AsyncSocket does not subsequently use a
  // destroyed RocketClient.
  socket_->setReadCB(nullptr);
  socket_.reset();

  clientState_.connState = ConnectionState::CLOSED;
  if (auto closeCallback = std::move(closeCallback_)) {
    closeCallback();
  }

  queue_.failAllSentWrites(error_);

  // Move streams_ into a local copy before iterating and erasing. Note that
  // flowable->onError() may itself attempt to erase an element of streams_,
  // invalidating any outstanding iterators. Also, since the client is
  // shutting down now, we don't bother with notifyIfDetachable().
  auto streams = std::move(streams_);
  for (const auto& callback : streams) {
    callback.match([&](auto* serverCallback) {
      if (isFirstResponse(serverCallback->streamId())) {
        serverCallback->onInitialError(error_);
      } else {
        serverCallback->onStreamTransportError(error_);
      }
    });
  }
  firstResponseTimeouts_.clear();
  bufferedFragments_.clear();
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

void RocketClient::maybeScheduleFirstResponseTimeout(
    StreamId streamId, std::chrono::milliseconds timeout) {
  DCHECK(evb_);
  DCHECK(firstResponseTimeouts_.find(streamId) == firstResponseTimeouts_.end());

  if (timeout == std::chrono::milliseconds::zero()) {
    firstResponseTimeouts_.emplace(streamId, nullptr);
    return;
  }

  auto firstResponseTimeout =
      std::make_unique<FirstResponseTimeout>(*this, streamId);
  evb_->timer().scheduleTimeout(firstResponseTimeout.get(), timeout);
  firstResponseTimeouts_.emplace(streamId, std::move(firstResponseTimeout));
}

bool RocketClient::isFirstResponse(StreamId streamId) const {
  return firstResponseTimeouts_.count(streamId) > 0;
}

void RocketClient::acknowledgeFirstResponse(StreamId streamId) {
  firstResponseTimeouts_.erase(streamId);
}

void RocketClient::FirstResponseTimeout::timeoutExpired() noexcept {
  // remove ourselves from the timeout set to avoid being freed prematurely by
  // the callback below
  auto ptr = std::move(client_.firstResponseTimeouts_.at(streamId_));

  const auto streamIt = client_.streams_.find(streamId_);
  CHECK(streamIt != client_.streams_.end());

  streamIt->match([&](auto* serverCallback) {
    serverCallback->onInitialError(
        folly::make_exception_wrapper<transport::TTransportException>(
            transport::TTransportException::TIMED_OUT));
  });
}

void RocketClient::attachEventBase(folly::EventBase& evb) {
  if (evb_ == &evb) {
    return;
  }

  DCHECK(!evb_);
  evb.dcheckIsInEventBaseThread();

  evb_ = &evb;
  socket_->attachEventBase(evb_);
  evb_->runOnDestruction(eventBaseDestructionCallback_);
  flushManager_ = &FlushManager::getInstance(*evb_);
}

void RocketClient::detachEventBase() {
  DCHECK(getDestructorGuardCount() == 0);
  DCHECK(evb_);
  evb_->dcheckIsInEventBaseThread();
  DCHECK(!writeLoopCallback_.isLoopCallbackScheduled());

  if (!parser_.getNewBufferLogicEnabled()) {
    // trigger the buffer resize timeout ensure rocket client not holding extra
    // buffer when detaching
    parser_.cancelTimeout();
    parser_.timeoutExpired();
  }

  eventBaseDestructionCallback_.cancel();
  detachableLoopCallback_.cancelLoopCallback();
  socket_->detachEventBase();
  flushManager_ = nullptr;
  evb_ = nullptr;
  flushList_ = nullptr;
}

void RocketClient::DetachableLoopCallback::runLoopCallback() noexcept {
  if (client_.onDetachable_ && client_.isDetachable()) {
    client_.onDetachable_();
  }
}

void RocketClient::CloseLoopCallback::runLoopCallback() noexcept {
  client_.closeNowImpl();
}

void RocketClient::OnEventBaseDestructionCallback::
    onEventBaseDestruction() noexcept {
  // Make sure we never run RocketClient destructor inline from
  // OnEventBaseDestructionCallback, since it will try to deregister itself from
  // the EventBase and deadlock.
  client_.evb_->runInLoop([dg = DestructorGuard(&client_)] {});
  client_.closeNow(transport::TTransportException("Destroying EventBase"));
}

void RocketClient::terminateInteraction(int64_t id) {
  auto guard = folly::makeGuard([this] {
    if (!--interactions_) {
      notifyIfDetachable();
    }
  });

  if (setupFrame_) {
    // we haven't sent any requests so don't need to send the termination
    return;
  }

  auto onError =
      [dg = DestructorGuard(this),
       this,
       guard = std::move(guard),
       ka = folly::getKeepAliveToken(evb_)](transport::TTransportException ex) {
        close(std::move(ex));
      };

  std::ignore = sendVersionDependentFrame(
      [id](int32_t serverVersion) {
        InteractionTerminate term;
        term.interactionId_ref() = id;
        if (serverVersion >= 7) {
          ClientPushMetadata clientMeta;
          clientMeta.interactionTerminate_ref() = std::move(term);
          return std::make_pair<std::unique_ptr<folly::IOBuf>, FrameType>(
              MetadataPushFrame::makeFromMetadata(
                  packCompact(std::move(clientMeta)))
                  .serialize(),
              FrameType::METADATA_PUSH);
        } else {
          return std::make_pair<std::unique_ptr<folly::IOBuf>, FrameType>(
              ExtFrame(
                  StreamId(),
                  Payload::makeFromData(packCompact(std::move(term))),
                  Flags::none(),
                  ExtFrameType::INTERACTION_TERMINATE)
                  .serialize(),
              FrameType::EXT);
        }
      },
      StreamId(),
      std::move(onError));
}

void RocketClient::onServerVersionRequired() {
  // if a version dependent frame is scheduled but server setup response has
  // not been received, buffer all other requests until server version is
  // known
  DCHECK(serverVersion_ == -1);
  if (!queue_.startBufferingRequests()) {
    return; // already buffering
  }
  // include a timeout to close the connection if server does not respond
  serverVersionTimeout_.reset(new ServerVersionTimeout(*this));
  evb_->timer().scheduleTimeout(
      serverVersionTimeout_.get(),
      std::chrono::milliseconds(THRIFT_FLAG(rocket_server_version_timeout_ms)));
}

void RocketClient::setServerVersion(int32_t serverVersion) {
  // set server version, cancel timeout and resolve any buffered requests
  serverVersion_ = serverVersion;
  serverVersionTimeout_.reset();
  if (queue_.resolveWriteBuffer(serverVersion)) {
    scheduleWriteLoopCallback();
  }
}

std::unique_ptr<SetupFrame> RocketClient::moveOutSetupFrame() {
  if (UNLIKELY(clientState_.hasPendingSetupFrame)) {
    clientState_.hasPendingSetupFrame = false;
    return std::move(setupFrame_);
  }
  return nullptr;
}

} // namespace rocket
} // namespace thrift
} // namespace apache
