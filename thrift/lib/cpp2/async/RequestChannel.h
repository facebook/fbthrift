/*
 * Copyright 2014 Facebook, Inc.
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

#include <functional>
#include <memory>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp/Thrift.h>
#include <folly/Function.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/Request.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <folly/ExceptionWrapper.h>
#include <folly/String.h>
#include <folly/io/IOBufQueue.h>
#include <wangle/deprecated/rx/Subject.h>

#include <glog/logging.h>

#include <chrono>

namespace folly {
class IOBuf;
}

namespace apache { namespace thrift {

class ClientReceiveState {
 public:
  ClientReceiveState()
      : protocolId_(-1),
        isSecurityActive_(false),
        isStreamEnd_(false) {
  }

  ClientReceiveState(uint16_t _protocolId,
                    std::unique_ptr<folly::IOBuf> _buf,
                    std::unique_ptr<apache::thrift::transport::THeader> _header,
                    std::shared_ptr<apache::thrift::ContextStack> _ctx,
                    bool _isSecurityActive,
                    bool _isStreamEnd = false)
    : protocolId_(_protocolId),
      ctx_(std::move(_ctx)),
      buf_(std::move(_buf)),
      header_(std::move(_header)),
      isSecurityActive_(_isSecurityActive),
      isStreamEnd_(_isStreamEnd) {
  }
  ClientReceiveState(folly::exception_wrapper _excw,
                     std::shared_ptr<apache::thrift::ContextStack> _ctx,
                     bool _isSecurityActive)
    : protocolId_(-1),
      ctx_(std::move(_ctx)),
      header_(folly::make_unique<apache::thrift::transport::THeader>()),
      excw_(std::move(_excw)),
      isSecurityActive_(_isSecurityActive),
      isStreamEnd_(false) {
  }

  bool isException() const {
    return excw_ ? true : false;
  }

  folly::exception_wrapper exceptionWrapper() {
    return excw_;
  }

  folly::exception_wrapper&& moveExceptionWrapper() {
    return std::move(excw_);
  }

  std::exception_ptr exception() {
    if (!exc_ && excw_) {
      exc_ = excw_.to_exception_ptr();
    }
    return exc_;
  }

  void resetException(folly::exception_wrapper ex) {
    excw_ = std::move(ex);
    exc_ = std::exception_ptr();
  }

  uint16_t protocolId() const {
    return protocolId_;
  }

  folly::IOBuf* buf() const {
    return buf_.get();
  }

  std::unique_ptr<folly::IOBuf> extractBuf() {
    return std::move(buf_);
  }

  apache::thrift::transport::THeader* header() const {
    return header_.get();
  }

  std::unique_ptr<apache::thrift::transport::THeader> extractHeader() {
    return std::move(header_);
  }

  void resetHeader(std::unique_ptr<apache::thrift::transport::THeader> h) {
    header_ = std::move(h);
  }

  apache::thrift::ContextStack* ctx() const {
    return ctx_.get();
  }

  bool isSecurityActive() const {
    return isSecurityActive_;
  }

  void resetCtx(std::shared_ptr<apache::thrift::ContextStack> _ctx) {
    ctx_ = std::move(_ctx);
  }

  bool isStreamEnd() const {
    return isStreamEnd_;
  }

 private:
  uint16_t protocolId_;
  std::shared_ptr<apache::thrift::ContextStack> ctx_;
  std::unique_ptr<folly::IOBuf> buf_;
  std::unique_ptr<apache::thrift::transport::THeader> header_;
  std::exception_ptr exc_;
  folly::exception_wrapper excw_;
  bool isSecurityActive_;
  bool isStreamEnd_;
};

class RequestCallback {
 public:
  virtual ~RequestCallback() {}
  virtual void requestSent() = 0;
  virtual void replyReceived(ClientReceiveState&&) = 0;
  virtual void requestError(ClientReceiveState&&) = 0;

  std::shared_ptr<folly::RequestContext> context_;
  // To log latency incurred for doing thrift security
  int64_t securityStart_ = 0;
  int64_t securityEnd_ = 0;
};

/* FunctionReplyCallback is meant to make RequestCallback easy to use
 * with folly::Function objects.  It is slower than implementing
 * RequestCallback directly.  It also throws the specific error
 * away, since there is no place to save it in a backwards
 * compatible way to thrift1.  It is still logged, though.
 *
 * Recommend upgrading to RequestCallback if possible
 */
class FunctionReplyCallback : public RequestCallback {
 public:
  explicit FunctionReplyCallback(
    folly::Function<void (ClientReceiveState&&)> callback)
      : callback_(std::move(callback)) {}
  void replyReceived(ClientReceiveState&& state) override {
    callback_(std::move(state));
  }
  void requestError(ClientReceiveState&& state) override {
    VLOG(1)
      << "Got an exception in FunctionReplyCallback replyReceiveError: "
      << folly::exceptionStr(state.exception());
    callback_(std::move(state));
  }
  void requestSent() override {}

private:
  folly::Function<void (ClientReceiveState&&)> callback_;
};

/* Useful for oneway methods. */
class FunctionSendCallback : public RequestCallback {
 public:
  explicit FunctionSendCallback(
    folly::Function<void (ClientReceiveState&&)>&& callback)
      : callback_(std::move(callback)) {}
  void requestSent() override {
    auto cb = std::move(callback_);
    cb(ClientReceiveState(folly::exception_wrapper(), nullptr, false));
  }
  void requestError(ClientReceiveState&& state) override {
    auto cb = std::move(callback_);
    cb(std::move(state));
  }
  void replyReceived(ClientReceiveState&& /*state*/) override {}
 private:
  folly::Function<void (ClientReceiveState&&)> callback_;
};

class CloseCallback {
 public:
  /**
   * When the channel is closed, replyError() will be invoked on all of the
   * outstanding replies, then channelClosed() on the CloseCallback.
   */
  virtual void channelClosed() = 0;

  virtual ~CloseCallback() {}
};

/**
 * RpcOptions class to set per-RPC options (such as request timeout).
 */
class RpcOptions {
 public:
  typedef apache::thrift::concurrency::PRIORITY PRIORITY;
  RpcOptions()
   : timeout_(0),
     priority_(apache::thrift::concurrency::N_PRIORITIES),
     chunkTimeout_(0),
     queueTimeout_(0)
  { }

  /**
   * NOTE: This only sets the receive timeout, and not the send timeout on
   * transport. Probably you want to use HeaderClientChannel::setTimeout()
   */
  RpcOptions& setTimeout(std::chrono::milliseconds timeout) {
    timeout_ = timeout;
    return *this;
  }

  std::chrono::milliseconds getTimeout() const {
    return timeout_;
  }

  RpcOptions& setPriority(PRIORITY priority) {
    priority_ = priority;
    return *this;
  }

  PRIORITY getPriority() const {
    return priority_;
  }

  RpcOptions& setChunkTimeout(std::chrono::milliseconds chunkTimeout) {
    chunkTimeout_ = chunkTimeout;
    return *this;
  }

  std::chrono::milliseconds getChunkTimeout() const {
    return chunkTimeout_;
  }


  RpcOptions& setQueueTimeout(std::chrono::milliseconds queueTimeout) {
    queueTimeout_ = queueTimeout;
    return *this;
  }

  std::chrono::milliseconds getQueueTimeout() const {
    return queueTimeout_;
  }

  void setWriteHeader(const std::string& key, const std::string& value) {
    writeHeaders_[key] = value;
  }

  void setReadHeaders(std::map<std::string, std::string>&& readHeaders) {
    readHeaders_ = std::move(readHeaders);
  }

  const std::map<std::string, std::string>& getReadHeaders() const {
    return readHeaders_;
  }

  const std::map<std::string, std::string>& getWriteHeaders() const {
    return writeHeaders_;
  }

  std::map<std::string, std::string> releaseWriteHeaders() {
    std::map<std::string, std::string> headers;
    writeHeaders_.swap(headers);
    return headers;
  }
 private:
  std::chrono::milliseconds timeout_;
  PRIORITY priority_;
  std::chrono::milliseconds chunkTimeout_;
  std::chrono::milliseconds queueTimeout_;

  // For sending and receiving headers.
  std::map<std::string, std::string> writeHeaders_;
  std::map<std::string, std::string> readHeaders_;
};

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
    return sendRequest(options,
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
    return sendOnewayRequest(options,
                             std::move(cb),
                             std::move(ctx),
                             std::move(buf),
                             std::move(header));
  }

  virtual void setCloseCallback(CloseCallback*) = 0;

  virtual folly::EventBase* getEventBase() const = 0;

  virtual uint16_t getProtocolId() = 0;
};

class ClientSyncCallback : public RequestCallback {
 public:
  ClientSyncCallback(ClientReceiveState* rs,
                     folly::EventBase* eb,
                     bool oneway = false)
      : rs_(rs)
      , eb_(eb)
      , oneway_(oneway) {}

  void requestSent() override {
    if (oneway_) {
      assert(eb_);
      eb_->terminateLoopSoon();
    }
  }
  void replyReceived(ClientReceiveState&& rs) override {
    assert(rs.buf());
    assert(eb_);
    assert(!oneway_);
    *rs_ = std::move(rs);
    eb_->terminateLoopSoon();
  }
  void requestError(ClientReceiveState&& rs) override {
    assert(rs.exception());
    assert(eb_);
    *rs_ = std::move(rs);
    eb_->terminateLoopSoon();
  }
 private:
  ClientReceiveState* rs_;
  folly::EventBase* eb_;
  bool oneway_;
};

template <typename T>
void clientCallbackToObservable(ClientReceiveState& state,
    folly::exception_wrapper (*recv_wrapped)(T&, ClientReceiveState&),
    wangle::SubjectPtr<T>& subj) {
  if (auto ew = state.exceptionWrapper()) {
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

template <bool oneway, class Protocol, class Pargs, class WriteFunc, class SizeFunc>
static void clientSendT(
    Protocol* prot,
    apache::thrift::RpcOptions& rpcOptions,
    std::unique_ptr<apache::thrift::RequestCallback> callback,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::shared_ptr<apache::thrift::transport::THeader> header,
    RequestChannel* channel,
    Pargs& pargs,
    const char* methodName,
    WriteFunc&& writefunc,
    SizeFunc&& sizefunc) {
  size_t bufSize = sizefunc(prot, pargs);
  bufSize += prot->serializedMessageSize(methodName);
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  prot->setOutput(&queue, bufSize);
  auto guard = folly::makeGuard([&]{prot->setOutput(nullptr);});
  try {
    ctx->preWrite();
    prot->writeMessageBegin(methodName, apache::thrift::T_CALL, 0);
    writefunc(prot, pargs);
    prot->writeMessageEnd();
    ::apache::thrift::SerializedMessage smsg;
    smsg.protocolType = prot->protocolType();
    smsg.buffer = queue.front();
    ctx->onWriteData(smsg);
    ctx->postWrite(queue.chainLength());
  }
  catch (const apache::thrift::TException &ex) {
    ctx->handlerErrorWrapped(
        folly::exception_wrapper(std::current_exception(), ex));
    throw;
  }

  auto eb = channel->getEventBase();
  if(!eb || eb->isInEventBaseThread()) {
    if (oneway) {
      // Calling asyncComplete before sending because
      // sendOnewayRequest moves from ctx and clears it.
      ctx->asyncComplete();
      channel->sendOnewayRequest(rpcOptions, std::move(callback),
          std::move(ctx), queue.move(), header);
    } else {
      channel->sendRequest(rpcOptions, std::move(callback),
          std::move(ctx), queue.move(), header);
    }
  }
  else {
    eb->runInEventBaseThread([
      channel,
      rpcOptions,
      callback = std::move(callback),
      ctx = std::move(ctx),
      queue = queue.move(),
      header
    ]() mutable {
      if (oneway) {
        // Calling asyncComplete before sending because
        // sendOnewayRequest moves from ctx and clears it.
        ctx->asyncComplete();
        channel->sendOnewayRequest(
            rpcOptions,
            std::move(callback),
            std::move(ctx),
            std::move(queue),
            header);
      } else {
        channel->sendRequest(
            rpcOptions,
            std::move(callback),
            std::move(ctx),
            std::move(queue),
            header);
      }
    });
  }
}
}} // apache::thrift

#endif // #ifndef THRIFT_ASYNC_REQUESTCHANNEL_H_
