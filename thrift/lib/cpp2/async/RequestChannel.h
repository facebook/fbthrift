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
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace folly {
class IOBuf;
}

namespace apache {
namespace thrift {

class StreamClientCallback;
class SinkClientCallback;

namespace detail {
template <RpcKind Kind>
struct RequestClientCallbackType {
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
  template <RpcKind Kind>
  void sendRequestAsync(
      const apache::thrift::RpcOptions&,
      folly::StringPiece methodName,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      typename detail::RequestClientCallbackType<Kind>::Ptr) = delete;

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
      folly::StringPiece methodName,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) = 0;

  /* Similar to sendRequest, although replyReceived will never be called
   *
   * Null RequestCallback is allowed for oneway requests
   */
  virtual void sendRequestNoResponse(
      const RpcOptions&,
      folly::StringPiece methodName,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) = 0;

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
      folly::StringPiece methodName,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader> header,
      StreamClientCallback* clientCallback);

  virtual void sendRequestSink(
      const RpcOptions& rpcOptions,
      folly::StringPiece methodName,
      SerializedRequest&&,
      std::shared_ptr<transport::THeader> header,
      SinkClientCallback* clientCallback);

  virtual void setCloseCallback(CloseCallback*) = 0;

  virtual folly::EventBase* getEventBase() const = 0;

  virtual uint16_t getProtocolId() = 0;

  virtual void terminateInteraction(InteractionId id);

  // registers a new interaction with the channel
  // returns id of created interaction (always nonzero)
  virtual InteractionId createInteraction(folly::StringPiece name);

  // registers an interaction with a nested channel
  // only some channels can be nested; the rest call terminate here
  virtual void registerInteraction(folly::StringPiece name, int64_t id);

 protected:
  static InteractionId createInteractionId(int64_t id);
  static void releaseInteractionId(InteractionId&& id);
};

template <>
void RequestChannel::sendRequestAsync<RpcKind::SINGLE_REQUEST_NO_RESPONSE>(
    const apache::thrift::RpcOptions&,
    folly::StringPiece methodName,
    SerializedRequest&&,
    std::shared_ptr<apache::thrift::transport::THeader>,
    RequestClientCallback::Ptr);
template <>
void RequestChannel::sendRequestAsync<RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE>(
    const apache::thrift::RpcOptions&,
    folly::StringPiece methodName,
    SerializedRequest&&,
    std::shared_ptr<apache::thrift::transport::THeader>,
    RequestClientCallback::Ptr);
template <>
void RequestChannel::sendRequestAsync<
    RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE>(
    const apache::thrift::RpcOptions&,
    folly::StringPiece methodName,
    SerializedRequest&&,
    std::shared_ptr<apache::thrift::transport::THeader>,
    StreamClientCallback*);
template <>
void RequestChannel::sendRequestAsync<RpcKind::SINK>(
    const apache::thrift::RpcOptions&,
    folly::StringPiece methodName,
    SerializedRequest&&,
    std::shared_ptr<apache::thrift::transport::THeader>,
    SinkClientCallback*);

template <bool oneWay>
class ClientSyncCallback : public RequestClientCallback {
 public:
  explicit ClientSyncCallback(ClientReceiveState* rs) : rs_(rs) {}

  void waitUntilDone(folly::EventBase* evb) {
    if (evb) {
      if (!evb->inRunningEventBaseThread() || !folly::fibers::onFiber()) {
        while (!doneBaton_.ready()) {
          evb->drive();
        }
      }
    }
    doneBaton_.wait();
  }

  void onRequestSent() noexcept override {
    if (oneWay) {
      doneBaton_.post();
    }
  }
  void onResponse(ClientReceiveState&& rs) noexcept override {
    assert(rs.buf());
    assert(!oneWay);
    *rs_ = std::move(rs);
    doneBaton_.post();
  }
  void onResponseError(folly::exception_wrapper ex) noexcept override {
    *rs_ = ClientReceiveState(std::move(ex), nullptr);
    doneBaton_.post();
  }

  bool isInlineSafe() const override {
    return true;
  }

  bool isSync() const override {
    return true;
  }

 private:
  ClientReceiveState* rs_;
  folly::fibers::Baton doneBaton_;
};

inline StreamClientCallback* createStreamClientCallback(
    RequestClientCallback::Ptr requestCallback,
    int32_t bufferSize) {
  DCHECK(requestCallback->isInlineSafe());
  class RequestClientCallbackWrapper
      : public detail::ClientStreamBridge::FirstResponseCallback {
   public:
    RequestClientCallbackWrapper(
        RequestClientCallback::Ptr requestCallback,
        int32_t bufferSize)
        : requestCallback_(std::move(requestCallback)),
          bufferSize_(bufferSize) {}

    void onFirstResponse(
        FirstResponsePayload&& firstResponse,
        detail::ClientStreamBridge::ClientPtr clientStreamBridge) override {
      auto tHeader = std::make_unique<transport::THeader>();
      tHeader->setClientType(THRIFT_HTTP_CLIENT_TYPE);
      detail::fillTHeaderFromResponseRpcMetadata(
          firstResponse.metadata, *tHeader);
      requestCallback_.release()->onResponse(ClientReceiveState(
          static_cast<uint16_t>(-1),
          std::move(firstResponse.payload),
          std::move(tHeader),
          nullptr,
          std::move(clientStreamBridge),
          bufferSize_));
      delete this;
    }

    void onFirstResponseError(folly::exception_wrapper ew) override {
      requestCallback_.release()->onResponseError(std::move(ew));
      delete this;
    }

   private:
    RequestClientCallback::Ptr requestCallback_;
    const int32_t bufferSize_;
  };

  return detail::ClientStreamBridge::create(
      new RequestClientCallbackWrapper(std::move(requestCallback), bufferSize));
}

template <class Protocol>
SerializedRequest preprocessSendT(
    Protocol* prot,
    const apache::thrift::RpcOptions& rpcOptions,
    apache::thrift::ContextStack& ctx,
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
      ctx.preWrite();
      writefunc(prot);
      ::apache::thrift::SerializedMessage smsg;
      smsg.protocolType = prot->protocolType();
      smsg.buffer = queue.front();
      smsg.methodName = methodName;
      ctx.onWriteData(smsg);
      ctx.postWrite(folly::to_narrow(queue.chainLength()));
    } catch (const apache::thrift::TException& ex) {
      ctx.handlerErrorWrapped(
          folly::exception_wrapper(std::current_exception(), ex));
      throw;
    }

    if (rpcOptions.getEnableChecksum()) {
      header.setCrc32c(apache::thrift::checksum::crc32c(*queue.front()));
    }

    return SerializedRequest(queue.move());
  });
}

template <RpcKind Kind, class Protocol>
void clientSendT(
    Protocol* prot,
    const apache::thrift::RpcOptions& rpcOptions,
    typename detail::RequestClientCallbackType<Kind>::Ptr callback,
    apache::thrift::ContextStack& ctx,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestChannel* channel,
    folly::StringPiece methodName,
    folly::FunctionRef<void(Protocol*)> writefunc,
    folly::FunctionRef<size_t(Protocol*)> sizefunc) {
  auto request = preprocessSendT(
      prot, rpcOptions, ctx, *header, methodName, writefunc, sizefunc);

  channel->sendRequestAsync<Kind>(
      rpcOptions,
      methodName,
      std::move(request),
      std::move(header),
      std::move(callback));
}

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_ASYNC_REQUESTCHANNEL_H_
