/*
 * Copyright 2014-present Facebook, Inc.
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
#ifndef THRIFT_ASYNC_REQUESTCHANNEL_H_
#define THRIFT_ASYNC_REQUESTCHANNEL_H_ 1

#include <chrono>
#include <functional>
#include <memory>

#include <glog/logging.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Function.h>
#include <folly/String.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/Request.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/RequestCallback.h>
#include <thrift/lib/cpp2/async/SemiStream.h>
#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/util/Checksum.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <wangle/deprecated/rx/Subject.h>

namespace folly {
class IOBuf;
}

namespace apache {
namespace thrift {

class StreamClientCallback;

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
  virtual void sendRequestSync(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RpcKind kind);

  void sendRequestAsync(
      apache::thrift::RpcOptions&,
      std::unique_ptr<apache::thrift::RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RpcKind kind);
  /**
   * ReplyCallback will be invoked when the reply to this request is
   * received.  TRequestChannel is responsible for associating requests with
   * responses, and invoking the correct ReplyCallback when a response
   * message is received.
   *
   * cb must not be null.
   */
  virtual uint32_t sendRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) = 0;

  uint32_t sendRequest(
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<apache::thrift::ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header) {
    RpcOptions options;
    return sendRequest(
        options,
        std::move(cb),
        std::move(ctx),
        std::move(buf),
        std::move(header));
  }

  /* Similar to sendRequest, although replyReceived will never be called
   *
   * Null RequestCallback is allowed for oneway requests
   */
  virtual uint32_t sendOnewayRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) = 0;

  uint32_t sendOnewayRequest(
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<apache::thrift::ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header) {
    RpcOptions options;
    return sendOnewayRequest(
        options,
        std::move(cb),
        std::move(ctx),
        std::move(buf),
        std::move(header));
  }

  /**
   * ReplyCallback will be invoked when the reply to this request is
   * received.  RequestChannel is responsible for associating requests with
   * responses, and invoking the correct ReplyCallback when a response
   * message is received. A response to this request may contain a stream.
   *
   * cb must not be null.
   */
  virtual uint32_t sendStreamRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>);

  virtual void sendRequestStream(
      RpcOptions& rpcOptions,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<transport::THeader> header,
      StreamClientCallback* clientCallback);

  virtual void setCloseCallback(CloseCallback*) = 0;

  virtual folly::EventBase* getEventBase() const = 0;

  virtual uint16_t getProtocolId() = 0;
};

class ClientSyncCallback : public RequestCallback {
 public:
  explicit ClientSyncCallback(
      ClientReceiveState* rs,
      RpcKind kind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE)
      : rs_(rs), rpcKind_(kind) {}

  void requestSent() override {}
  void replyReceived(ClientReceiveState&& rs) override {
    assert(rs.buf());
    assert(!isOneway());
    *rs_ = std::move(rs);
  }
  void requestError(ClientReceiveState&& rs) override {
    assert(!!rs.exception());
    *rs_ = std::move(rs);
  }
  bool isOneway() const {
    return rpcKind_ == RpcKind::SINGLE_REQUEST_NO_RESPONSE;
  }
  RpcKind rpcKind() const {
    return rpcKind_;
  }

 private:
  ClientReceiveState* rs_;
  RpcKind rpcKind_;
};

template <typename T>
void clientCallbackToObservable(
    ClientReceiveState& state,
    folly::exception_wrapper (*recv_wrapped)(T&, ClientReceiveState&),
    wangle::SubjectPtr<T>& subj) {
  if (auto const& ew = state.exception()) {
    subj->onError(ew);
    return;
  }
  T value;
  if (auto ew = recv_wrapped(value, state)) {
    subj->onError(ew);
    return;
  }
  if (state.isStreamEnd()) {
    subj->onCompleted();
    return;
  }
  subj->onNext(value);
}

template <class Protocol>
void clientSendT(
    Protocol* prot,
    apache::thrift::RpcOptions& rpcOptions,
    std::unique_ptr<apache::thrift::RequestCallback> callback,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestChannel* channel,
    const char* methodName,
    folly::FunctionRef<void(Protocol*)> writefunc,
    folly::FunctionRef<size_t(Protocol*)> sizefunc,
    RpcKind kind,
    bool sync) {
  size_t bufSize = sizefunc(prot);
  bufSize += prot->serializedMessageSize(methodName);
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  prot->setOutput(&queue, bufSize);
  auto guard = folly::makeGuard([&] { prot->setOutput(nullptr); });
  size_t crcSkip = 0;
  try {
    ctx->preWrite();
    prot->writeMessageBegin(methodName, apache::thrift::T_CALL, 0);
    crcSkip = queue.chainLength();
    writefunc(prot);
    prot->writeMessageEnd();
    ::apache::thrift::SerializedMessage smsg;
    smsg.protocolType = prot->protocolType();
    smsg.buffer = queue.front();
    ctx->onWriteData(smsg);
    ctx->postWrite(queue.chainLength());
  } catch (const apache::thrift::TException& ex) {
    ctx->handlerErrorWrapped(
        folly::exception_wrapper(std::current_exception(), ex));
    throw;
  }
  header->setCrc32c(apache::thrift::checksum::crc32c(*queue.front(), crcSkip));

  if (sync) {
    channel->sendRequestSync(
        rpcOptions,
        std::move(callback),
        std::move(ctx),
        queue.move(),
        std::move(header),
        kind);
    return;
  }

  channel->sendRequestAsync(
      rpcOptions,
      std::move(callback),
      std::move(ctx),
      queue.move(),
      std::move(header),
      kind);
}
} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_ASYNC_REQUESTCHANNEL_H_
