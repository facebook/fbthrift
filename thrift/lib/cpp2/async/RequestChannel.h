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

#ifndef THRIFT_ASYNC_REQUESTCHANNEL_H_
#define THRIFT_ASYNC_REQUESTCHANNEL_H_ 1

#include <chrono>
#include <functional>
#include <memory>

#include <glog/logging.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Function.h>
#include <folly/String.h>
#include <folly/Utility.h>
#include <folly/fibers/FiberManager.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/Request.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp2/async/Interaction.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/RequestCallback.h>
#include <thrift/lib/cpp2/async/RpcTypes.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/transport/core/EnvelopeUtil.h>
#include <thrift/lib/cpp2/transport/core/RpcMetadataUtil.h>
#include <thrift/lib/cpp2/util/Checksum.h>
#include <thrift/lib/cpp2/util/MethodMetadata.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace folly {
class IOBuf;
}

namespace apache {
namespace thrift {

class StreamClientCallback;
class SinkClientCallback;
class RequestChannel;

namespace detail {
template <RpcKind Kind>
struct RequestClientCallbackType {};
template <>
struct RequestClientCallbackType<RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE> {
  using Ptr = RequestClientCallback::Ptr;
};
template <>
struct RequestClientCallbackType<RpcKind::SINGLE_REQUEST_NO_RESPONSE> {
  using Ptr = RequestClientCallback::Ptr;
};
template <>
struct RequestClientCallbackType<RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE> {
  using Ptr = StreamClientCallback*;
};
template <>
struct RequestClientCallbackType<RpcKind::SINK> {
  using Ptr = SinkClientCallback*;
};
template <typename T>
struct const_if_lvalue_ref {
  using type = T;
};
template <typename T>
struct const_if_lvalue_ref<T&> {
  using type = const T&;
};
template <RpcKind Kind, typename RpcOptions>
using ChannelSendFunc = void (RequestChannel::*)(
    typename const_if_lvalue_ref<RpcOptions>::type&&,
    MethodMetadata&&,
    SerializedRequest&&,
    std::shared_ptr<transport::THeader>,
    typename RequestClientCallbackType<Kind>::Ptr);
} // namespace detail

/**
 * RequestChannel defines an asynchronous API for request-based I/O.
 */
class RequestChannel : virtual public folly::DelayedDestruction {
 protected:
  ~RequestChannel() override {}

 public:
  /**
   * ReplyCallback will be invoked when the reply to this request is
   * received.  TRequestChannel is responsible for associating requests with
   * responses, and invoking the correct ReplyCallback when a response
   * message is received.
   *
   * cb must not be null.
   */
  template <RpcKind Kind, typename RpcOptions>
  void sendRequestAsync(
      RpcOptions&&,
      MethodMetadata&&,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>&&,
      typename apache::thrift::detail::RequestClientCallbackType<Kind>::Ptr);

  /**
   * ReplyCallback will be invoked when the reply to this request is
   * received.  TRequestChannel is responsible for associating requests with
   * responses, and invoking the correct ReplyCallback when a response
   * message is received.
   *
   * cb must not be null.
   */
  virtual void sendRequestResponse(
      const RpcOptions&,
      MethodMetadata&&,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr);

  /* Similar to sendRequest, although replyReceived will never be called
   *
   * Null RequestCallback is allowed for oneway requests
   */
  virtual void sendRequestNoResponse(
      const RpcOptions&,
      MethodMetadata&&,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr);

  /**
   * ReplyCallback will be invoked when the reply to this request is
   * received.  RequestChannel is responsible for associating requests with
   * responses, and invoking the correct ReplyCallback when a response
   * message is received. A response to this request may contain a stream.
   *
   * cb must not be null.
   */
  virtual void sendRequestStream(
      const RpcOptions& rpcOptions,
      MethodMetadata&&,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader> header,
      StreamClientCallback* clientCallback);

  virtual void sendRequestSink(
      const RpcOptions& rpcOptions,
      MethodMetadata&&,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader> header,
      SinkClientCallback* clientCallback);

  // Some channels can make use of rvalue RpcOptions as an optimization.
  virtual void sendRequestResponse(
      RpcOptions&&,
      MethodMetadata&&,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr);
  virtual void sendRequestNoResponse(
      RpcOptions&&,
      MethodMetadata&&,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr);
  virtual void sendRequestStream(
      RpcOptions&&,
      MethodMetadata&&,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader>,
      StreamClientCallback*);
  virtual void sendRequestSink(
      RpcOptions&&,
      MethodMetadata&&,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader>,
      SinkClientCallback*);

  virtual void setCloseCallback(CloseCallback*) = 0;

  virtual folly::EventBase* getEventBase() const = 0;

  virtual uint16_t getProtocolId() = 0;

  virtual void terminateInteraction(InteractionId id);

  // registers a new interaction with the channel
  // returns id of created interaction (always nonzero)
  virtual InteractionId createInteraction(ManagedStringView&& name);

  // registers an interaction with a nested channel
  // only some channels can be nested; the rest call terminate here
  virtual InteractionId registerInteraction(
      ManagedStringView&& name, int64_t id);

  using Ptr =
      std::unique_ptr<RequestChannel, folly::DelayedDestruction::Destructor>;

 protected:
  static InteractionId createInteractionId(int64_t id);
  static void releaseInteractionId(InteractionId&& id);
};

template <bool oneWay>
class ClientSyncCallback : public RequestClientCallback {
 public:
  explicit ClientSyncCallback(ClientReceiveState* rs) : rs_(rs) {}

  template <typename F>
  void waitUntilDone(folly::EventBase* evb, F&& sendF) {
    if (evb &&
        (!evb->inRunningEventBaseThread() || !folly::fibers::onFiber())) {
      folly::fibers::runInMainContext([&] {
        sendF();
        while (!doneBaton_.ready()) {
          evb->drive();
        }
      });
    } else {
      sendF();
      // Check if it's ready to avoid unnecessarily preempting a fiber.
      if (!doneBaton_.ready()) {
        doneBaton_.wait();
      }
    }
  }

  void waitUntilDone(folly::EventBase* evb) {
    waitUntilDone(evb, [] {});
  }

  // This approach avoids an inner coroutine frame
  folly::fibers::Baton& co_waitUntilDone() { return doneBaton_; }

  void onRequestSent() noexcept override {
    if (oneWay) {
      doneBaton_.post();
    }
  }
  void onResponse(ClientReceiveState&& rs) noexcept override {
    assert(rs.hasResponseBuffer());
    assert(!oneWay);
    *rs_ = std::move(rs);
    doneBaton_.post();
  }
  void onResponseError(folly::exception_wrapper ex) noexcept override {
    *rs_ = ClientReceiveState(std::move(ex), nullptr);
    doneBaton_.post();
  }

  bool isInlineSafe() const override { return true; }

  bool isSync() const override { return true; }

 private:
  ClientReceiveState* rs_;
  folly::fibers::Baton doneBaton_;
};

inline StreamClientCallback* createStreamClientCallback(
    RequestClientCallback::Ptr requestCallback,
    const BufferOptions& bufferOptions) {
  DCHECK(requestCallback->isInlineSafe())
      << "Streaming methods do not support the callback client method flavor. "
         "Use co_, sync_, or semifuture_ instead.";
  class RequestClientCallbackWrapper
      : public apache::thrift::detail::ClientStreamBridge::
            FirstResponseCallback {
   public:
    RequestClientCallbackWrapper(
        RequestClientCallback::Ptr requestCallback,
        const BufferOptions& bufferOptions)
        : requestCallback_(std::move(requestCallback)),
          bufferOptions_(bufferOptions) {}

    void onFirstResponse(
        FirstResponsePayload&& firstResponse,
        apache::thrift::detail::ClientStreamBridge::ClientPtr
            clientStreamBridge) override {
      auto tHeader = std::make_unique<transport::THeader>();
      tHeader->setClientType(THRIFT_ROCKET_CLIENT_TYPE);
      apache::thrift::detail::fillTHeaderFromResponseRpcMetadata(
          firstResponse.metadata, *tHeader);
      requestCallback_.release()->onResponse(ClientReceiveState(
          static_cast<uint16_t>(-1),
          std::move(firstResponse.payload),
          std::move(tHeader),
          nullptr,
          std::move(clientStreamBridge),
          bufferOptions_));
      delete this;
    }

    void onFirstResponseError(folly::exception_wrapper ew) override {
      requestCallback_.release()->onResponseError(std::move(ew));
      delete this;
    }

   private:
    RequestClientCallback::Ptr requestCallback_;
    BufferOptions bufferOptions_;
  };

  return apache::thrift::detail::ClientStreamBridge::create(
      new RequestClientCallbackWrapper(
          std::move(requestCallback), bufferOptions));
}

#if FOLLY_HAS_COROUTINES
inline SinkClientCallback* createSinkClientCallback(
    RequestClientCallback::Ptr requestCallback) {
  DCHECK(requestCallback->isInlineSafe());
  class RequestClientCallbackWrapper
      : public apache::thrift::detail::ClientSinkBridge::FirstResponseCallback {
   public:
    explicit RequestClientCallbackWrapper(
        RequestClientCallback::Ptr requestCallback)
        : requestCallback_(std::move(requestCallback)) {}

    void onFirstResponse(
        FirstResponsePayload&& firstResponse,
        apache::thrift::detail::ClientSinkBridge::Ptr clientSinkBridge)
        override {
      auto tHeader = std::make_unique<transport::THeader>();
      tHeader->setClientType(THRIFT_ROCKET_CLIENT_TYPE);
      apache::thrift::detail::fillTHeaderFromResponseRpcMetadata(
          firstResponse.metadata, *tHeader);
      requestCallback_.release()->onResponse(ClientReceiveState(
          static_cast<uint16_t>(-1),
          std::move(firstResponse.payload),
          std::move(clientSinkBridge),
          std::move(tHeader),
          nullptr));
      delete this;
    }

    void onFirstResponseError(folly::exception_wrapper ew) override {
      requestCallback_.release()->onResponseError(std::move(ew));
      delete this;
    }

   private:
    RequestClientCallback::Ptr requestCallback_;
  };

  return apache::thrift::detail::ClientSinkBridge::create(
      new RequestClientCallbackWrapper(std::move(requestCallback)));
}
#endif

template <class Protocol>
SerializedRequest preprocessSendT(
    Protocol* prot,
    const apache::thrift::RpcOptions& rpcOptions,
    apache::thrift::ContextStack* ctx,
    apache::thrift::transport::THeader& header,
    folly::StringPiece methodName,
    folly::FunctionRef<void(Protocol*)> writefunc,
    folly::FunctionRef<size_t(Protocol*)> sizefunc) {
  return folly::fibers::runInMainContext([&] {
    size_t bufSize = sizefunc(prot);
    folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());

    // Preallocate small buffer headroom for transports metadata & framing.
    constexpr size_t kHeadroomBytes = 128;
    auto buf = folly::IOBuf::create(kHeadroomBytes + bufSize);
    buf->advance(kHeadroomBytes);
    queue.append(std::move(buf));

    prot->setOutput(&queue, bufSize);
    auto guard = folly::makeGuard([&] { prot->setOutput(nullptr); });
    try {
      if (ctx) {
        ctx->preWrite();
      }
      writefunc(prot);
      ::apache::thrift::SerializedMessage smsg;
      smsg.protocolType = prot->protocolType();
      smsg.buffer = queue.front();
      smsg.methodName = methodName;
      if (ctx) {
        ctx->onWriteData(smsg);
        ctx->postWrite(folly::to_narrow(queue.chainLength()));
        ctx->resetClientRequestContextHeader();
      }
    } catch (const apache::thrift::TException& ex) {
      if (ctx) {
        ctx->handlerErrorWrapped(
            folly::exception_wrapper(std::current_exception(), ex));
      }
      throw;
    }

    if (rpcOptions.getEnableChecksum()) {
      header.setCrc32c(apache::thrift::checksum::crc32c(*queue.front()));
    }

    return SerializedRequest(queue.move());
  });
}

namespace detail {
template <RpcKind Kind, typename RpcOptions>
constexpr ChannelSendFunc<Kind, RpcOptions> getChannelSendFunc() {
  if constexpr (Kind == RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE) {
    return &RequestChannel::sendRequestResponse;
  } else if constexpr (Kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
    return &RequestChannel::sendRequestNoResponse;
  } else if constexpr (Kind == RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE) {
    return &RequestChannel::sendRequestStream;
  } else {
    static_assert(Kind == RpcKind::SINK);
    return &RequestChannel::sendRequestSink;
  }
}
} // namespace detail

template <RpcKind Kind, typename RpcOptions>
void RequestChannel::sendRequestAsync(
    RpcOptions&& rpcOptions,
    MethodMetadata&& methodMetadata,
    SerializedRequest&& request,
    std::shared_ptr<apache::thrift::transport::THeader>&& header,
    typename apache::thrift::detail::RequestClientCallbackType<Kind>::Ptr
        callback) {
  auto* eb = getEventBase();
  if (!eb || eb->isInEventBaseThread()) {
    auto send = apache::thrift::detail::getChannelSendFunc<Kind, RpcOptions>();
    (this->*send)(
        std::forward<RpcOptions>(rpcOptions),
        std::move(methodMetadata),
        std::move(request),
        std::move(header),
        std::move(callback));
  } else {
    eb->runInEventBaseThread([this,
                              rpcOptions = std::forward<RpcOptions>(rpcOptions),
                              methodMetadata = std::move(methodMetadata),
                              request = std::move(request),
                              header = std::move(header),
                              callback = std::move(callback)]() mutable {
      auto send =
          apache::thrift::detail::getChannelSendFunc<Kind, RpcOptions>();
      (this->*send)(
          std::forward<RpcOptions>(rpcOptions),
          std::move(methodMetadata),
          std::move(request),
          std::move(header),
          std::move(callback));
    });
  }
}

template <RpcKind Kind, class Protocol, typename RpcOptions>
void clientSendT(
    Protocol* prot,
    RpcOptions&& rpcOptions,
    typename apache::thrift::detail::RequestClientCallbackType<Kind>::Ptr
        callback,
    apache::thrift::ContextStack* ctx,
    std::shared_ptr<apache::thrift::transport::THeader>&& header,
    RequestChannel* channel,
    apache::thrift::MethodMetadata&& methodMetadata,
    folly::FunctionRef<void(Protocol*)> writefunc,
    folly::FunctionRef<size_t(Protocol*)> sizefunc) {
  auto request = preprocessSendT(
      prot,
      rpcOptions,
      ctx,
      *header,
      methodMetadata.name_view(),
      writefunc,
      sizefunc);

  channel->sendRequestAsync<Kind>(
      std::forward<RpcOptions>(rpcOptions),
      std::move(methodMetadata),
      std::move(request),
      std::move(header),
      std::move(callback));
}

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_ASYNC_REQUESTCHANNEL_H_
