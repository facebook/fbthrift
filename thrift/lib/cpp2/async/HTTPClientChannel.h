/*
 * Copyright 2015-present Facebook, Inc.
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

#include <deque>
#include <memory>
#include <unordered_map>

#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/HHWheelTimer.h>
#include <folly/io/async/Request.h>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>

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
 public:
  using Ptr = std::unique_ptr<HTTPClientChannel,
                              folly::DelayedDestruction::Destructor>;

  static HTTPClientChannel::Ptr newHTTP1xChannel(
      async::TAsyncTransport::UniquePtr transport,
      const std::string& httpHost,
      const std::string& httpUrl);

  static HTTPClientChannel::Ptr newHTTP2Channel(
      async::TAsyncTransport::UniquePtr transport);

  void setHTTPHost(const std::string& host) { httpHost_ = host; }
  void setHTTPUrl(const std::string& url) { httpUrl_ = url; }

  void setProtocolId(uint16_t protocolId) { protocolId_ = protocolId; }

  // apache::thrift::ClientChannel methods

  void closeNow() override;

  void attachEventBase(folly::EventBase*) override;
  void detachEventBase() override;
  bool isDetachable() override;

  bool isSecurityActive() override { return false; }

  // Client timeouts for read, write.
  // Servers should use timeout methods on underlying transport.
  void setTimeout(uint32_t ms) override {
      timeout_ = std::chrono::milliseconds(ms);
  }
  uint32_t getTimeout() override { return timeout_.count(); }

  CLIENT_TYPE getClientType() override { return THRIFT_HTTP_CLIENT_TYPE; }

  // end apache::thrift::ClientChannel methods

  // folly::DelayedDestruction methods

  void destroy() override;

  // end folly::DelayedDestruction methods

  // apache::thrift::RequestChannel methods

  folly::EventBase* getEventBase() const override { return evb_; }

  uint32_t sendRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) override;

  uint32_t sendOnewayRequest(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) override;

  void setCloseCallback(CloseCallback* cb) override { closeCallback_ = cb; }

  uint16_t getProtocolId() override { return protocolId_; }

  async::TAsyncTransport* getTransport() override {
    if (httpSession_) {
      return dynamic_cast<async::TAsyncTransport*>(
          httpSession_->getTransport());
    } else {
      return nullptr;
    }
  }

  // end apache::thrift::RequestChannel methods

  void setFlowControl(size_t initialReceiveWindow,
                      size_t receiveStreamWindowSize,
                      size_t receiveSessionWindowSize);

 protected:
   uint32_t sendRequest_(
       RpcOptions&,
       bool oneway,
       std::unique_ptr<RequestCallback>,
       std::unique_ptr<apache::thrift::ContextStack>,
       std::unique_ptr<folly::IOBuf>,
       std::shared_ptr<apache::thrift::transport::THeader>);

 private:
  HTTPClientChannel(
      async::TAsyncTransport::UniquePtr transport,
      std::unique_ptr<proxygen::HTTPCodec> codec);

  ~HTTPClientChannel() override;

  class HTTPTransactionCallback
      : public MessageChannel::SendCallback,
        public proxygen::HTTPTransactionHandler,
        public proxygen::HTTPTransaction::TransportCallback,
        public folly::HHWheelTimer::Callback {
   public:
    HTTPTransactionCallback(
        bool oneway,
        std::unique_ptr<RequestCallback> cb,
        std::unique_ptr<apache::thrift::ContextStack> ctx,
        bool isSecurityActive,
        uint16_t protoId);

    ~HTTPTransactionCallback();

    void startTimer(
        folly::HHWheelTimer& timer,
        std::chrono::milliseconds timeout);

    // MessageChannel::SendCallback methods

    void sendQueued() override { }

    void messageSent() override;

    void messageSendError(folly::exception_wrapper&& ex) override;

    // end MessageChannel::SendCallback methods

    // proxygen::HTTPTransactionHandler methods

    void setTransaction(proxygen::HTTPTransaction* txn) noexcept override;

    void detachTransaction() noexcept override;

    void onHeadersComplete(
        std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override;

    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;

    void onChunkHeader(size_t /* length */) noexcept override {
      // HTTP/1.1 function, do not need attention here
    }

    void onChunkComplete() noexcept override {
      // HTTP/1.1 function, do not need attention here
    }

    void onTrailers(
        std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept override;

    void onEOM() noexcept override;

    void onUpgrade(proxygen::UpgradeProtocol /*protocol*/) noexcept override {
      // If code comes here, it is seriously wrong
      // TODO (geniusye) destroy the channel here
    }

    void onError(const proxygen::HTTPException& error) noexcept override;

    void onEgressPaused() noexcept override {
      // we could notify servicerouter to throttle on this channel
      // it is okay not to throttle too,
      // it won't immediately causing any problem
    }

    void onEgressResumed() noexcept override {
      // we could notify servicerouter to stop throttle on this channel
      // it is okay not to throttle too,
      // it won't immediately causing any problem
    }

    void onPushedTransaction(
        proxygen::HTTPTransaction* /*txn*/) noexcept override {}

    // end proxygen::HTTPTransactionHandler methods

    // proxygen::HTTPTransaction::TransportCallback methods

    // most of the methods in TransportCallback is not interesting to us,
    // thus, we don't have to handle them, except the one that notifies the
    // fact the request is sent.

    void firstHeaderByteFlushed() noexcept override {}
    void firstByteFlushed() noexcept override {}

    void lastByteFlushed() noexcept override;

    void lastByteAcked(
        std::chrono::milliseconds /*latency*/) noexcept override {}
    void headerBytesGenerated(
        proxygen::HTTPHeaderSize& /*size*/) noexcept override {}
    void headerBytesReceived(
        const proxygen::HTTPHeaderSize& /*size*/) noexcept override {}
    void bodyBytesGenerated(size_t /*nbytes*/) noexcept override {}
    void bodyBytesReceived(size_t /*size*/) noexcept override {}

    // end proxygen::HTTPTransaction::TransportCallback methods

    // folly::HHWheelTimer::Callback methods

    void timeoutExpired() noexcept override;

    // end folly::HHWheelTimer::Callback methods

    void requestError(folly::exception_wrapper ex);

    proxygen::HTTPTransaction* getTransaction() noexcept { return txn_; }

   private:
    bool oneway_;

    std::unique_ptr<RequestCallback> cb_;
    std::unique_ptr<apache::thrift::ContextStack> ctx_;
    bool isSecurityActive_;
    uint16_t protoId_;

    proxygen::HTTPTransaction* txn_;
    std::unique_ptr<proxygen::HTTPMessage> msg_;
    std::unique_ptr<folly::IOBufQueue> body_;
    std::unique_ptr<proxygen::HTTPHeaders> trailers_;
  };

  proxygen::HTTPMessage buildHTTPMessage(transport::THeader* header);

  // HTTPSession::InfoCallback methods

  void onCreate(const proxygen::HTTPSession&) override {}
  void onIngressError(const proxygen::HTTPSession&,
                      proxygen::ProxygenError /*error*/) override {}
  void onIngressEOF() override {}
  void onRead(const proxygen::HTTPSession&, size_t /*bytesRead*/) override {}
  void onWrite(
      const proxygen::HTTPSession&, size_t /*bytesWritten*/) override { }
  void onRequestBegin(const proxygen::HTTPSession&) override {}
  void onRequestEnd(const proxygen::HTTPSession&,
                    uint32_t /*maxIngressQueueSize*/) override {}
  void onActivateConnection(const proxygen::HTTPSession&) override {}
  void onDeactivateConnection(const proxygen::HTTPSession&) override {}
  void onDestroy(const proxygen::HTTPSession&) override;
  void onIngressMessage(const proxygen::HTTPSession&,
                        const proxygen::HTTPMessage&) override {}
  void onIngressLimitExceeded(const proxygen::HTTPSession&) override {}
  void onIngressPaused(const proxygen::HTTPSession&) override {}
  void onTransactionDetached(const proxygen::HTTPSession&) override {}
  void onPingReplySent(int64_t /*latency*/) override {}
  void onPingReplyReceived() override {}
  void onSettingsOutgoingStreamsFull(const proxygen::HTTPSession&) override {}
  void onSettingsOutgoingStreamsNotFull(
      const proxygen::HTTPSession&) override { }
  void onFlowControlWindowClosed(const proxygen::HTTPSession&) override {}
  void onEgressBuffered(const proxygen::HTTPSession&) override {}
  void onEgressBufferCleared(const proxygen::HTTPSession&) override {}

  // end HTTPSession::InfoCallback methods

  void setRequestHeaderOptions(apache::thrift::transport::THeader* header);

  proxygen::HTTPUpstreamSession* httpSession_ = nullptr;
  folly::EventBase* evb_;
  std::string httpHost_;
  std::string httpUrl_;
  std::chrono::milliseconds timeout_;
  proxygen::WheelTimerInstance timer_;
  uint16_t protocolId_;
  CloseCallback* closeCallback_;

};

}} // apache::thrift
