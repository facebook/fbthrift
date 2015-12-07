/*
 * Copyright 2015 Facebook, Inc.
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

#pragma once

#include <folly/io/async/HHWheelTimer.h>
#include <proxygen/lib/http/codec/HTTPCodec.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/Request.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <folly/io/async/EventBase.h>
#include <memory>

#include <unordered_map>
#include <deque>

namespace apache {
namespace thrift {

/**
 * HTTPClientChannel
 *
 * This is a channel implementation that reads and writes
 * messages encoded using THttpProtocol.
 */
class HTTPClientChannel : public ClientChannel,
                          private proxygen::HTTPSession::InfoCallback,
                          virtual public folly::DelayedDestruction {
 protected:
  ~HTTPClientChannel() override {}

 public:
  explicit HTTPClientChannel(
      apache::thrift::async::TAsyncTransport::UniquePtr transport,
      const std::string& host,
      const std::string& url,
      std::unique_ptr<proxygen::HTTPCodec> codec);

  typedef std::unique_ptr<HTTPClientChannel,
                          folly::DelayedDestruction::Destructor> Ptr;

  static Ptr newChannel(
      apache::thrift::async::TAsyncTransport::UniquePtr transport,
      const std::string& host,
      const std::string& url,
      std::unique_ptr<proxygen::HTTPCodec> codec) {
    return Ptr(new HTTPClientChannel(
        std::move(transport), host, url, std::move(codec)));
  }

  static Ptr newHTTP1xChannel(
      apache::thrift::async::TAsyncTransport::UniquePtr transport,
      const std::string& host,
      const std::string& url);

  static Ptr newHTTP2Channel(
      apache::thrift::async::TAsyncTransport::UniquePtr transport,
      const std::string& host,
      const std::string& url);

  void closeNow() override;

  // DelayedDestruction methods
  void destroy() override;

  // Client interface from RequestChannel
  using RequestChannel::sendRequest;
  uint32_t sendRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) override;

  using RequestChannel::sendOnewayRequest;
  uint32_t sendOnewayRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) override;

  void setCloseCallback(CloseCallback*) override;

  // Client timeouts for read, write.
  // Servers should use timeout methods on underlying transport.
  void setTimeout(uint32_t ms) override;
  uint32_t getTimeout() override { return timeout_; }

  void setFlowControl(size_t initialReceiveWindow,
                      size_t receiveStreamWindowSize,
                      size_t receiveSessionWindowSize);

  // If a Close Callback is set, should we reregister callbacks for it
  // alone?  Basically, this means that loop() will return if the only thing
  // outstanding is close callbacks.
  void setKeepRegisteredForClose(bool keepRegisteredForClose) {
    keepRegisteredForClose_ = keepRegisteredForClose;
    setBaseReceivedCallback();
  }

  bool getKeepRegisteredForClose() { return keepRegisteredForClose_; }

  folly::EventBase* getEventBase() override { return evb_; }

  // event base methods
  void attachEventBase(folly::EventBase*) override;
  void detachEventBase() override;
  bool isDetachable() override;

  uint16_t getProtocolId() override { return protocolId_; }
  void setProtocolId(uint16_t protocolId) { protocolId_ = protocolId; }

  bool isSecurityActive() override { return false; }

  async::TAsyncTransport* getTransport() override {
    if (httpSession_) {
      return dynamic_cast<async::TAsyncTransport*>(
          httpSession_->getTransport());
    } else {
      return nullptr;
    }
  }

  CLIENT_TYPE getClientType() override { return THRIFT_HTTP_CLIENT_TYPE; }

  void setHTTPHost(const std::string& host) {
    httpHost_ = host;
  }

  void setHTTPUrl(const std::string& url) {
    httpUrl_ = url;
  }

 private:
  class HTTPTransactionTwowayCallback
      : public MessageChannel::SendCallback,
        public proxygen::HTTPTransactionHandler,
        public proxygen::HTTPTransaction::TransportCallback,
        public folly::HHWheelTimer::Callback {
   public:
    explicit HTTPTransactionTwowayCallback(
        std::unique_ptr<RequestCallback> cb,
        std::unique_ptr<apache::thrift::ContextStack> ctx,
        bool isSecurityActive,
        uint16_t protoId,
        folly::HHWheelTimer* timer,
        std::chrono::milliseconds timeout)
        : cb_(std::move(cb)),
          ctx_(std::move(ctx)),
          cbCalled_(false),
          isSecurityActive_(isSecurityActive),
          protoId_(protoId),
          timer_(timer),
          txn_(nullptr) {
      if (timeout.count()) {
        timer_->scheduleTimeout(this, timeout);
      }
    }

    ~HTTPTransactionTwowayCallback() {
      cancelTimeout();
      if (txn_) {
        txn_->setHandler(nullptr);
        txn_->setTransportCallback(nullptr);
      }
    }

    virtual void setTransaction(
        proxygen::HTTPTransaction* txn) noexcept override {
      txn_ = txn;
      txn_->setTransportCallback(this);
    }

    proxygen::HTTPTransaction* getTransaction() noexcept { return txn_; }

    virtual void detachTransaction() noexcept override { delete this; }

    void sendQueued() override {}

    void messageSent() override {
      CHECK(cb_);
      auto old_ctx = folly::RequestContext::setContext(cb_->context_);
      cb_->requestSent();
      folly::RequestContext::setContext(old_ctx);
    }

    void messageSendError(folly::exception_wrapper&& ex) override {
      if (!cbCalled_) {
        cbCalled_ = true;
        auto old_ctx = folly::RequestContext::setContext(cb_->context_);
        cb_->requestError(ClientReceiveState(
            std::move(ex), std::move(ctx_), isSecurityActive_));
        folly::RequestContext::setContext(old_ctx);
      }
    }

    void requestError(folly::exception_wrapper ex) {
      CHECK(cb_);

      if (!cbCalled_) {
        cbCalled_ = true;
        auto old_ctx = folly::RequestContext::setContext(cb_->context_);
        cb_->requestError(ClientReceiveState(
            std::move(ex), std::move(ctx_), isSecurityActive_));
        folly::RequestContext::setContext(old_ctx);
      }
    }

    virtual void onHeadersComplete(
        std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override {
      msg_ = std::move(msg);
    }

    virtual void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {
      if (body_) {
        body_->prependChain(std::move(body));
      } else {
        body_ = std::move(body);
      }
    }

    /*
    virtual void onChunkHeader(size_t length) noexcept override {
      // TODO(ckwalsh): Look into streaming support over http
    }

    virtual void onChunkComplete() noexcept override {
      // TODO(ckwalsh): Look into streaming support over http
    }
    */

    virtual void onTrailers(
        std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept override {
      trailers_ = std::move(trailers);
    }

    virtual void onEOM() noexcept override {
      auto header = folly::make_unique<transport::THeader>();
      header->setClientType(THRIFT_HTTP_CLIENT_TYPE);
      apache::thrift::transport::THeader::StringToStringMap readHeaders;
      msg_->getHeaders().forEach(
          [&readHeaders](const std::string& key, const std::string& val) {
            readHeaders[key] = val;
          });
      header->setReadHeaders(std::move(readHeaders));
      // TODO(ckwalsh) Does THeader need additional data?

      CHECK(!cbCalled_);
      CHECK(cb_);
      cbCalled_ = true;

      auto old_ctx = folly::RequestContext::setContext(cb_->context_);

      cb_->replyReceived(ClientReceiveState(protoId_,
                                            std::move(body_),
                                            std::move(header),
                                            std::move(ctx_),
                                            isSecurityActive_,
                                            true));

      folly::RequestContext::setContext(old_ctx);
    }

    virtual void onUpgrade(
        proxygen::UpgradeProtocol /*protocol*/) noexcept override {}

    virtual void onError(
        const proxygen::HTTPException& error) noexcept override {
      if (sendQueued_) {
        messageSendError(
            folly::make_exception_wrapper<transport::TTransportException>(
                error.what()));
      } else {
        requestError(
            folly::make_exception_wrapper<transport::TTransportException>(
                error.what()));
      }
    }

    virtual void onEgressPaused() noexcept override {}

    virtual void onEgressResumed() noexcept override {}

    virtual void onPushedTransaction(
        proxygen::HTTPTransaction* /*txn*/) noexcept override {}

    // proxygen::HTTPTransaction::TransportCallback
    virtual void firstHeaderByteFlushed() noexcept override {}
    virtual void firstByteFlushed() noexcept override {}

    virtual void lastByteFlushed() noexcept override {
      auto old_ctx = folly::RequestContext::setContext(cb_->context_);
      cb_->requestSent();
      folly::RequestContext::setContext(old_ctx);
      sendQueued_ = false;
    }

    virtual void lastByteAcked(
        std::chrono::milliseconds /*latency*/) noexcept override {}
    virtual void headerBytesGenerated(
        proxygen::HTTPHeaderSize& /*size*/) noexcept override {}
    virtual void headerBytesReceived(
        const proxygen::HTTPHeaderSize& /*size*/) noexcept override {}
    virtual void bodyBytesGenerated(size_t /*nbytes*/) noexcept override {}
    virtual void bodyBytesReceived(size_t /*size*/) noexcept override {}

    void timeoutExpired() noexcept override {
      using apache::thrift::transport::TTransportException;

      TTransportException ex(TTransportException::TIMED_OUT, "Timed Out");
      ex.setOptions(TTransportException::CHANNEL_IS_VALID);

      requestError(
          folly::make_exception_wrapper<TTransportException>(std::move(ex)));

      delete this;
    }

   private:
    std::unique_ptr<RequestCallback> cb_;
    std::unique_ptr<apache::thrift::ContextStack> ctx_;
    bool cbCalled_;
    bool isSecurityActive_;
    uint16_t protoId_;
    folly::HHWheelTimer* timer_;

    bool sendQueued_ = true;
    proxygen::HTTPTransaction* txn_;
    std::unique_ptr<proxygen::HTTPMessage> msg_;
    std::unique_ptr<folly::IOBuf> body_;
    std::unique_ptr<proxygen::HTTPHeaders> trailers_;
  };

  class HTTPTransactionOnewayCallback
      : public MessageChannel::SendCallback,
        public proxygen::HTTPTransactionHandler,
        public proxygen::HTTPTransaction::TransportCallback {
   public:
    HTTPTransactionOnewayCallback(
        std::unique_ptr<RequestCallback> cb,
        std::unique_ptr<apache::thrift::ContextStack> ctx,
        bool isSecurityActive)
        : cb_(std::move(cb)),
          ctx_(std::move(ctx)),
          isSecurityActive_(isSecurityActive),
          txn_(nullptr),
          active_(true) {}

    virtual ~HTTPTransactionOnewayCallback() {
      if (txn_) {
        txn_->setHandler(nullptr);
        txn_->setTransportCallback(nullptr);
      }
    }

    proxygen::HTTPTransaction* getTransaction() noexcept { return txn_; }

    virtual void detachTransaction() noexcept override { delete this; }

    virtual void setTransaction(
        proxygen::HTTPTransaction* txn) noexcept override {
      txn_ = txn;
      txn_->setTransportCallback(this);
    }

    void sendQueued() override {}
    void messageSent() override {
      CHECK(cb_);
      auto old_ctx = folly::RequestContext::setContext(cb_->context_);
      cb_->requestSent();
      folly::RequestContext::setContext(old_ctx);
      active_ = false;
    }

    void messageSendError(folly::exception_wrapper&& ex) override {
      CHECK(cb_);
      auto old_ctx = folly::RequestContext::setContext(cb_->context_);
      cb_->requestError(
          ClientReceiveState(ex, std::move(ctx_), isSecurityActive_));
      folly::RequestContext::setContext(old_ctx);
      active_ = false;
    }

    virtual void onHeadersComplete(
        std::unique_ptr<proxygen::HTTPMessage> /*msg*/) noexcept override {}

    virtual void onBody(
        std::unique_ptr<folly::IOBuf> /*chain*/) noexcept override {}

    virtual void onTrailers(
        std::unique_ptr<proxygen::HTTPHeaders> /*trailers*/) noexcept override {
    }

    virtual void onEOM() noexcept override {}

    virtual void onUpgrade(
        proxygen::UpgradeProtocol /*protocol*/) noexcept override {}

    virtual void onError(
        const proxygen::HTTPException& error) noexcept override {
      if (active_) {
        messageSendError(
            folly::make_exception_wrapper<transport::TTransportException>(
                error.what()));
      }
    }

    virtual void onEgressPaused() noexcept override {}

    virtual void onEgressResumed() noexcept override {}

    virtual void onPushedTransaction(
        proxygen::HTTPTransaction* /*txn*/) noexcept override {}

    // proxygen::HTTPTransaction::TransportCallback
    virtual void firstHeaderByteFlushed() noexcept override {}
    virtual void firstByteFlushed() noexcept override {}

    virtual void lastByteFlushed() noexcept override { messageSent(); }

    virtual void lastByteAcked(
        std::chrono::milliseconds /*latency*/) noexcept override {}
    virtual void headerBytesGenerated(
        proxygen::HTTPHeaderSize& /*size*/) noexcept override {}
    virtual void headerBytesReceived(
        const proxygen::HTTPHeaderSize& /*size*/) noexcept override {}
    virtual void bodyBytesGenerated(size_t /*nbytes*/) noexcept override {}
    virtual void bodyBytesReceived(size_t /*size*/) noexcept override {}

   private:
    std::unique_ptr<RequestCallback> cb_;
    std::unique_ptr<apache::thrift::ContextStack> ctx_;
    bool isSecurityActive_;
    proxygen::HTTPTransaction* txn_;
    bool active_;
  };

  proxygen::HTTPMessage buildHTTPMessage(transport::THeader* header);

  // HTTPSession::InfoCallback methods
  void onCreate(const proxygen::HTTPSession&) override {}
  void onIngressError(const proxygen::HTTPSession&,
                      proxygen::ProxygenError /*error*/) override {}
  void onRead(const proxygen::HTTPSession&, size_t /*bytesRead*/) override {}
  void onWrite(const proxygen::HTTPSession&, size_t /*bytesWritten*/) override {
  }
  void onRequestBegin(const proxygen::HTTPSession&) override {}
  void onRequestEnd(const proxygen::HTTPSession&,
                    uint32_t /*maxIngressQueueSize*/) override {}
  void onActivateConnection(const proxygen::HTTPSession&) override {}
  void onDeactivateConnection(const proxygen::HTTPSession&,
                              const proxygen::TransactionInfo&) override {}
  void onDestroy(const proxygen::HTTPSession&) override {
    httpSession_ = nullptr;
  }
  void onIngressMessage(const proxygen::HTTPSession&,
                        const proxygen::HTTPMessage&) override {}
  void onIngressLimitExceeded(const proxygen::HTTPSession&) override {}
  void onIngressPaused(const proxygen::HTTPSession&) override {}
  void onTransactionDetached(const proxygen::HTTPSession&,
                             const proxygen::TransactionInfo&) override {}
  void onPingReplySent(int64_t /*latency*/) override {}
  void onPingReplyReceived() override {}
  void onSettingsOutgoingStreamsFull(const proxygen::HTTPSession&) override {}
  void onSettingsOutgoingStreamsNotFull(const proxygen::HTTPSession&) override {
  }
  void onFlowControlWindowClosed(const proxygen::HTTPSession&) override {}
  void onEgressBuffered(const proxygen::HTTPSession&) override{};

  void setRequestHeaderOptions(apache::thrift::transport::THeader* header);

  // Set the base class callback based on current state.
  void setBaseReceivedCallback();

  proxygen::HTTPUpstreamSession* httpSession_;
  std::string httpHost_;
  std::string httpUrl_;

  // TODO(ckwalsh): wire this up
  CloseCallback* closeCallback_;

  uint32_t timeout_;

  bool keepRegisteredForClose_;

  folly::EventBase* evb_;
  folly::HHWheelTimer::UniquePtr timer_;

  uint16_t protocolId_;
};
}
} // apache::thrift
