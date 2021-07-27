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

#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <string>

#include <folly/Try.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp/ContextStack.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ClientSinkBridge.h>
#include <thrift/lib/cpp2/async/ClientStreamBridge.h>
#include <thrift/lib/cpp2/async/Interaction.h>
#include <thrift/lib/cpp2/async/RpcOptions.h>
#include <thrift/lib/cpp2/async/RpcTypes.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

namespace apache {
namespace thrift {

struct RpcSizeStats {
  RpcSizeStats() = default;

  uint32_t requestSerializedSizeBytes{0};
  uint32_t requestWireSizeBytes{0};
  uint32_t responseSerializedSizeBytes{0};
  uint32_t responseWireSizeBytes{0};
};

class ClientReceiveState {
 public:
  ClientReceiveState() : protocolId_(-1) {}

  ClientReceiveState(
      uint16_t _protocolId,
      std::unique_ptr<folly::IOBuf> _buf,
      std::unique_ptr<apache::thrift::transport::THeader> _header,
      std::unique_ptr<apache::thrift::ContextStack> _ctx)
      : protocolId_(_protocolId),
        ctx_(std::move(_ctx)),
        header_(std::move(_header)) {
    initFromLegacyFormat(std::move(_buf));
  }
  ClientReceiveState(
      uint16_t _protocolId,
      std::unique_ptr<folly::IOBuf> _buf,
      std::unique_ptr<apache::thrift::transport::THeader> _header,
      std::unique_ptr<apache::thrift::ContextStack> _ctx,
      const RpcSizeStats& _rpcWireSizeStats)
      : protocolId_(_protocolId),
        ctx_(std::move(_ctx)),
        header_(std::move(_header)),
        rpcSizeStats_(_rpcWireSizeStats) {
    initFromLegacyFormat(std::move(_buf));
  }
  ClientReceiveState(
      uint16_t _protocolId,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<apache::thrift::transport::THeader> _header,
      std::unique_ptr<apache::thrift::ContextStack> _ctx,
      apache::thrift::detail::ClientStreamBridge::ClientPtr streamBridge,
      const BufferOptions& bufferOptions)
      : protocolId_(_protocolId),
        ctx_(std::move(_ctx)),
        header_(std::move(_header)),
        streamBridge_(std::move(streamBridge)),
        bufferOptions_(bufferOptions) {
    initFromLegacyFormat(std::move(buf));
  }
  ClientReceiveState(
      folly::exception_wrapper _excw,
      std::unique_ptr<apache::thrift::ContextStack> _ctx)
      : protocolId_(-1),
        ctx_(std::move(_ctx)),
        header_(std::make_unique<apache::thrift::transport::THeader>()),
        excw_(std::move(_excw)) {}
  ClientReceiveState(
      uint16_t _protocolId,
      std::unique_ptr<folly::IOBuf> _buf,
      apache::thrift::detail::ClientSinkBridge::Ptr sink,
      std::unique_ptr<apache::thrift::transport::THeader> _header,
      std::unique_ptr<apache::thrift::ContextStack> _ctx)
      : protocolId_(_protocolId),
        ctx_(std::move(_ctx)),
        header_(std::move(_header)),
        sink_(std::move(sink)) {
    initFromLegacyFormat(std::move(_buf));
  }
  ClientReceiveState(
      uint16_t _protocolId,
      MessageType mtype,
      SerializedResponse response,
      std::unique_ptr<apache::thrift::transport::THeader> _header,
      std::unique_ptr<apache::thrift::ContextStack> _ctx,
      const RpcSizeStats& _rpcWireSizeStats)
      : protocolId_(_protocolId),
        messageType_(mtype),
        ctx_(std::move(_ctx)),
        response_(std::move(response)),
        header_(std::move(_header)),
        rpcSizeStats_(_rpcWireSizeStats) {}

  bool isException() const { return excw_ ? true : false; }

  folly::exception_wrapper const& exception() const { return excw_; }

  folly::exception_wrapper& exception() { return excw_; }

  uint16_t protocolId() const { return protocolId_; }

  bool hasResponseBuffer() const { return (bool)response_.buffer; }

  MessageType messageType() const { return messageType_; }

  SerializedResponse& serializedResponse() { return response_; }
  const SerializedResponse& serializedResponse() const { return response_; }

  SerializedResponse extractSerializedResponse() {
    return std::move(response_);
  }

  apache::thrift::detail::ClientStreamBridge::ClientPtr extractStreamBridge() {
    return std::move(streamBridge_);
  }

  apache::thrift::detail::ClientSinkBridge::Ptr extractSink() {
    return std::move(sink_);
  }

  const BufferOptions& bufferOptions() const { return bufferOptions_; }

  apache::thrift::transport::THeader* header() const { return header_.get(); }

  std::unique_ptr<apache::thrift::transport::THeader> extractHeader() {
    return std::move(header_);
  }

  void resetHeader(std::unique_ptr<apache::thrift::transport::THeader> h) {
    header_ = std::move(h);
  }

  apache::thrift::ContextStack* ctx() const { return ctx_.get(); }

  void resetProtocolId(uint16_t protocolId) { protocolId_ = protocolId; }

  void resetCtx(std::unique_ptr<apache::thrift::ContextStack> _ctx) {
    ctx_ = std::move(_ctx);
  }

  const RpcSizeStats& getRpcSizeStats() const { return rpcSizeStats_; }

 private:
  uint16_t protocolId_;
  MessageType messageType_{MessageType::T_CALL};
  std::unique_ptr<apache::thrift::ContextStack> ctx_;
  SerializedResponse response_{nullptr};
  std::unique_ptr<apache::thrift::transport::THeader> header_;
  folly::exception_wrapper excw_;
  apache::thrift::detail::ClientStreamBridge::ClientPtr streamBridge_;
  apache::thrift::detail::ClientSinkBridge::Ptr sink_;
  BufferOptions bufferOptions_;
  RpcSizeStats rpcSizeStats_;

  void initFromLegacyFormat(std::unique_ptr<folly::IOBuf> buffer);
};

class RequestClientCallback {
 public:
  struct RequestClientCallbackDeleter {
    void operator()(RequestClientCallback* callback) const {
      callback->onResponseError(folly::exception_wrapper(
          std::logic_error("Request callback detached")));
    }
  };
  using Ptr =
      std::unique_ptr<RequestClientCallback, RequestClientCallbackDeleter>;

  virtual ~RequestClientCallback() {}
  virtual void onRequestSent() noexcept = 0;
  virtual void onResponse(ClientReceiveState&&) noexcept = 0;
  virtual void onResponseError(folly::exception_wrapper) noexcept = 0;

  // If true, the transport can block current thread/fiber until the request is
  // complete.
  virtual bool isSync() const { return false; }

  // If true, the transport can safely run this callback on its internal thread.
  // Should only be used for Thrift internal callbacks.
  virtual bool isInlineSafe() const { return false; }
};

class RequestCallback : public RequestClientCallback {
 public:
  virtual void requestSent() = 0;
  virtual void replyReceived(ClientReceiveState&&) = 0;
  virtual void requestError(ClientReceiveState&&) = 0;

  void onRequestSent() noexcept override {
    CHECK(thriftContext_);
    {
      auto work = [&]() noexcept {
        try {
          requestSent();
        } catch (...) {
          LOG(DFATAL)
              << "Exception thrown while executing requestSent() callback. "
              << "Exception: " << folly::exceptionStr(std::current_exception());
        }
      };
      if (thriftContext_->oneWay) {
        folly::RequestContextScopeGuard rctx(std::move(context_));
        work();
      } else {
        folly::RequestContextScopeGuard rctx(context_);
        work();
      }
    }
    if (unmanaged_ && thriftContext_->oneWay) {
      delete this;
    }
  }

  void onResponse(ClientReceiveState&& state) noexcept override {
    CHECK(thriftContext_);
    state.resetProtocolId(thriftContext_->protocolId);
    state.resetCtx(std::move(thriftContext_->ctx));
    {
      auto work = [&]() noexcept {
        try {
          replyReceived(std::move(state));
        } catch (...) {
          LOG(DFATAL)
              << "Exception thrown while executing replyReceived() callback. "
              << "Exception: " << folly::exceptionStr(std::current_exception());
        }
      };
      folly::RequestContextScopeGuard rctx(std::move(context_));
      work();
    }
    if (unmanaged_) {
      delete this;
    }
  }

  void onResponseError(folly::exception_wrapper ex) noexcept override {
    CHECK(thriftContext_);
    {
      folly::RequestContextScopeGuard rctx(std::move(context_));
      try {
        requestError(
            ClientReceiveState(std::move(ex), std::move(thriftContext_->ctx)));
      } catch (...) {
        LOG(DFATAL)
            << "Exception thrown while executing requestError() callback. "
            << "Exception: " << folly::exceptionStr(std::current_exception());
      }
    }
    if (unmanaged_) {
      delete this;
    }
  }

  std::shared_ptr<folly::RequestContext> context_;

  struct Context {
    bool oneWay{false};
    uint16_t protocolId;
    std::unique_ptr<apache::thrift::ContextStack> ctx;
  };

 private:
  friend RequestClientCallback::Ptr toRequestClientCallbackPtr(
      std::unique_ptr<RequestCallback>, RequestCallback::Context);

  void setContext(Context context) {
    context_ = folly::RequestContext::saveContext();
    thriftContext_ = std::move(context);
  }

  void setUnmanaged() { unmanaged_ = true; }

  bool unmanaged_{false};
  folly::Optional<Context> thriftContext_;
};

inline RequestClientCallback::Ptr toRequestClientCallbackPtr(
    std::unique_ptr<RequestCallback> cb, RequestCallback::Context context) {
  if (!cb) {
    return RequestClientCallback::Ptr();
  }
  cb->setContext(std::move(context));
  cb->setUnmanaged();
  return RequestClientCallback::Ptr(cb.release());
}

/***
 *  Like RequestCallback, a base class to be derived, but with a different set
 *  of overridable member functions which may be better suited to some cases.
 */
class SendRecvRequestCallback : public RequestCallback {
 public:
  virtual void send(folly::exception_wrapper&& ex) = 0;
  virtual void recv(ClientReceiveState&& state) = 0;

 private:
  enum struct Phase { Send, Recv };

  void requestSent() final {
    send({});
    phase_ = Phase::Recv;
  }
  void requestError(ClientReceiveState&& state) final {
    switch (phase_) {
      case Phase::Send:
        send(std::move(state.exception()));
        phase_ = Phase::Recv;
        break;
      case Phase::Recv:
        recv(std::move(state));
        break;
    }
  }
  void replyReceived(ClientReceiveState&& state) final {
    recv(std::move(state));
  }

  Phase phase_{Phase::Send};
};

class FunctionSendRecvRequestCallback final : public SendRecvRequestCallback {
 public:
  using Send = folly::Function<void(folly::exception_wrapper&&)>;
  using Recv = folly::Function<void(ClientReceiveState&&)>;
  FunctionSendRecvRequestCallback(Send sendf, Recv recvf)
      : sendf_(std::move(sendf)), recvf_(std::move(recvf)) {}
  void send(folly::exception_wrapper&& ew) override { sendf_(std::move(ew)); }
  void recv(ClientReceiveState&& state) override { recvf_(std::move(state)); }

 private:
  Send sendf_;
  Recv recvf_;
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
      folly::Function<void(ClientReceiveState&&)> callback)
      : callback_(std::move(callback)) {}
  void replyReceived(ClientReceiveState&& state) override {
    callback_(std::move(state));
  }
  void requestError(ClientReceiveState&& state) override {
    VLOG(1) << "Got an exception in FunctionReplyCallback replyReceiveError: "
            << state.exception();
    callback_(std::move(state));
  }
  void requestSent() override {}

 private:
  folly::Function<void(ClientReceiveState&&)> callback_;
};

/* Useful for oneway methods. */
class FunctionSendCallback : public RequestCallback {
 public:
  explicit FunctionSendCallback(
      folly::Function<void(ClientReceiveState&&)>&& callback)
      : callback_(std::move(callback)) {}
  void requestSent() override {
    auto cb = std::move(callback_);
    cb(ClientReceiveState(folly::exception_wrapper(), nullptr));
  }
  void requestError(ClientReceiveState&& state) override {
    auto cb = std::move(callback_);
    cb(std::move(state));
  }
  void replyReceived(ClientReceiveState&& /*state*/) override {}

 private:
  folly::Function<void(ClientReceiveState&&)> callback_;
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

struct RpcResponseContext {
  transport::THeader::StringToStringMap headers;
  RpcSizeStats rpcSizeStats;
  std::optional<int64_t> serverLoad;
};

template <class T>
struct RpcResponseComplete {
  using response_type = T;

  folly::Try<T> response;
  RpcResponseContext responseContext;
};

} // namespace thrift
} // namespace apache
