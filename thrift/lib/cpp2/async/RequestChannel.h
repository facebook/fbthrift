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
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/Request.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <folly/ExceptionWrapper.h>
#include <folly/String.h>

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
        isSecurityActive_(false) {
  }

  ClientReceiveState(uint16_t protocolId,
                     std::unique_ptr<folly::IOBuf> buf,
                     std::unique_ptr<apache::thrift::ContextStack> ctx,
                     bool isSecurityActive)
    : protocolId_(protocolId),
      ctx_(std::move(ctx)),
      buf_(std::move(buf)),
      isSecurityActive_(isSecurityActive) {
  }
  ClientReceiveState(folly::exception_wrapper excw,
                     std::unique_ptr<apache::thrift::ContextStack> ctx,
                     bool isSecurityActive)
    : protocolId_(-1),
      ctx_(std::move(ctx)),
      excw_(std::move(excw)),
      isSecurityActive_(isSecurityActive) {
  }

  bool isException() const {
    return excw_.get();
  }

  folly::exception_wrapper exceptionWrapper() {
    return excw_;
  }

  std::exception_ptr exception() {
    if (!exc_ && excw_.get()) {
      exc_ = excw_.getExceptionPtr();
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

  apache::thrift::ContextStack* ctx() const {
    return ctx_.get();
  }
  bool isSecurityActive() const {
    return isSecurityActive_;
  }

  void resetCtx(std::unique_ptr<apache::thrift::ContextStack> ctx) {
    ctx_ = std::move(ctx);
  }

  // Used by servicerouter to steal the context back for retrying a request
  std::unique_ptr<apache::thrift::ContextStack> releaseCtx() {
    return std::move(ctx_);
  }

 private:
  uint16_t protocolId_;
  std::unique_ptr<apache::thrift::ContextStack> ctx_;
  std::unique_ptr<folly::IOBuf> buf_;
  std::exception_ptr exc_;
  folly::exception_wrapper excw_;
  bool isSecurityActive_;
};

class RequestCallback {
 public:
  virtual ~RequestCallback() {}
  virtual void requestSent() = 0;
  virtual void replyReceived(ClientReceiveState&&) = 0;
  virtual void requestError(ClientReceiveState&&) = 0;

  std::shared_ptr<apache::thrift::async::RequestContext> context_;
};

/* FunctionReplyCallback is meant to make RequestCallback easy to use
 * with std::function objects.  It is slower than implementing
 * RequestCallback directly.  It also throws the specific error
 * away, since there is no place to save it in a backwards
 * compatible way to thrift1.  It is still logged, though.
 *
 * Recommend upgrading to RequestCallback if possible
 */
class FunctionReplyCallback : public RequestCallback {
 public:
  explicit FunctionReplyCallback(
    std::function<void (ClientReceiveState&&)> callback)
      : callback_(callback) {}
  void replyReceived(ClientReceiveState&& state) {
    callback_(std::move(state));
  }
  void requestError(ClientReceiveState&& state) {
    VLOG(1)
      << "Got an exception in FunctionReplyCallback replyReceiveError: "
      << folly::exceptionStr(state.exception());
    callback_(std::move(state));
  }
  void requestSent() {}
private:
  std::function<void (ClientReceiveState&&)> callback_;
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
  typedef apache::thrift::concurrency::PriorityThreadManager::PRIORITY PRIORITY;
  RpcOptions()
   : timeout_(0),
     priority_(apache::thrift::concurrency::N_PRIORITIES)
  { }

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
 private:
  std::chrono::milliseconds timeout_;
  PRIORITY priority_;
};

/**
 * RequestChannel defines an asynchronous API for request-based I/O.
 */
class RequestChannel : virtual public TDelayedDestruction {
 protected:
  virtual ~RequestChannel() {}

 public:
  /**
   * ReplyCallback will be invoked when the reply to this request is
   * received.  TRequestChannel is responsible for associating requests with
   * responses, and invoking the correct ReplyCallback when a response
   * message is received.
   *
   * cb must not be null.
   */
  virtual uint32_t sendRequest(const RpcOptions&,
                               std::unique_ptr<RequestCallback>,
                               std::unique_ptr<apache::thrift::ContextStack>,
                               std::unique_ptr<folly::IOBuf>) = 0;
  uint32_t sendRequest(std::unique_ptr<RequestCallback> cb,
                       std::unique_ptr<apache::thrift::ContextStack> ctx,
                       std::unique_ptr<folly::IOBuf> buf) {
    return sendRequest(RpcOptions(),
                       std::move(cb),
                       std::move(ctx),
                       std::move(buf));
  }

  /* Similar to sendRequest, although replyReceived will never be called
   *
   * Null RequestCallback is allowed for oneway requests
   */
  virtual uint32_t sendOnewayRequest(
      const RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>) = 0;

  uint32_t sendOnewayRequest(std::unique_ptr<RequestCallback> cb,
                             std::unique_ptr<apache::thrift::ContextStack> ctx,
                             std::unique_ptr<folly::IOBuf> buf) {
    return sendOnewayRequest(RpcOptions(),
                             std::move(cb),
                             std::move(ctx),
                             std::move(buf));
  }

  virtual void setCloseCallback(CloseCallback*) = 0;

  virtual apache::thrift::async::TEventBase* getEventBase() = 0;

  virtual uint16_t getProtocolId() = 0;

  virtual apache::thrift::transport::THeader* getHeader() {
    return nullptr;
  }
};

class ClientSyncCallback : public RequestCallback {
 public:
  ClientSyncCallback(ClientReceiveState* rs,
                     apache::thrift::async::TEventBase* eb,
                     bool oneway = false)
      : rs_(rs)
      , eb_(eb)
      , oneway_(oneway) {}

  void requestSent(){
    if (oneway_) {
      assert(eb_);
      eb_->terminateLoopSoon();
    }
  }
  void replyReceived(ClientReceiveState&& rs) {
    assert(rs.buf());
    assert(eb_);
    assert(!oneway_);
    *rs_ = std::move(rs);
    eb_->terminateLoopSoon();
  }
  void requestError(ClientReceiveState&& rs) {
    assert(rs.exception());
    assert(eb_);
    *rs_ = std::move(rs);
    eb_->terminateLoopSoon();
  }
 private:
  ClientReceiveState* rs_;
  apache::thrift::async::TEventBase* eb_;
  bool oneway_;
};

}} // apache::thrift

#endif // #ifndef THRIFT_ASYNC_REQUESTCHANNEL_H_
