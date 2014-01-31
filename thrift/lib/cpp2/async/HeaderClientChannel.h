/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_
#define THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_ 1

#include "thrift/lib/cpp/async/HHWheelTimer.h"
#include "thrift/lib/cpp2/async/MessageChannel.h"
#include "thrift/lib/cpp2/async/RequestChannel.h"
#include "thrift/lib/cpp2/async/SaslClient.h"
#include "thrift/lib/cpp2/async/Stream.h"
#include "thrift/lib/cpp2/async/Cpp2Channel.h"
#include "thrift/lib/cpp/async/TDelayedDestruction.h"
#include "thrift/lib/cpp/async/Request.h"
#include "thrift/lib/cpp/transport/THeader.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include <memory>

#include <unordered_map>
#include <deque>

using apache::thrift::async::TDelayedDestruction;

namespace apache { namespace thrift {

/**
 * HeaderClientChannel
 *
 * This is a channel implementation that reads and writes
 * messages encoded using THeaderProtocol.
 */
class HeaderClientChannel : public RequestChannel,
                            public MessageChannel::RecvCallback,
                            public SaslClient::Callback,
                            protected Cpp2Channel,
                            public apache::thrift::async::HHWheelTimer {
 private:
  virtual ~HeaderClientChannel(){}

 public:
  explicit HeaderClientChannel(
    const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport);

  typedef
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>
    Ptr;

  static Ptr newChannel(
    const std::shared_ptr<
    apache::thrift::async::TAsyncTransport>& transport) {
    return Ptr(new HeaderClientChannel(transport));
  }

  // TDelayedDestruction methods
  void destroy();

  apache::thrift::async::TAsyncTransport* getTransport() {
    return transport_.get();
  }

  // Client interface from RequestChannel
  using RequestChannel::sendRequest;
  void sendRequest(const RpcOptions&,
                   std::unique_ptr<RequestCallback>,
                   std::unique_ptr<apache::thrift::ContextStack>,
                   std::unique_ptr<folly::IOBuf>);

  using RequestChannel::sendOnewayRequest;
  void sendOnewayRequest(const RpcOptions&,
                         std::unique_ptr<RequestCallback>,
                         std::unique_ptr<apache::thrift::ContextStack>,
                         std::unique_ptr<folly::IOBuf>);

  void sendStreamingMessage(uint32_t streamSequenceId,
                            SendCallback* callback,
                            std::unique_ptr<folly::IOBuf>&& buf,
                            HEADER_FLAGS streamFlag);

  void setCloseCallback(CloseCallback*);

  // Interface from MessageChannel::RecvCallback
  void messageReceived(std::unique_ptr<folly::IOBuf>&&,
                       std::unique_ptr<MessageChannel::RecvCallback::sample>);
  void messageChannelEOF();
  void messageReceiveError(std::exception_ptr&&);

  // Interface from SaslClient::Callback
  void saslSendServer(std::unique_ptr<folly::IOBuf>&&);
  void saslError(std::exception_ptr&&);
  void saslComplete();

  // Client timeouts for read, write.
  // Servers should use timeout methods on underlying transport.
  void setTimeout(uint32_t ms);
  uint32_t getTimeout() {
    return transport_->getSendTimeout();
  }

  // SASL handshake timeout. This timeout is reset between each trip
  // in the handshake. So if you set it to 500ms, the full handshake timeout
  // would be 500ms * number of round trips.
  void setSaslTimeout(uint32_t ms);
  uint32_t getSaslTimeout() {
    return timeoutSASL_;
  }

  // If a Close Callback is set, should we reregister callbacks for it
  // alone?  Basically, this means that loop() will return if the only thing
  // outstanding is close callbacks.
  void setKeepRegisteredForClose(bool keepRegisteredForClose) {
    keepRegisteredForClose_ = keepRegisteredForClose;
    setBaseReceivedCallback();
  }

  bool getKeepRegisteredForClose() {
    return keepRegisteredForClose_;
  }

  apache::thrift::async::TEventBase* getEventBase() {
      return Cpp2Channel::getEventBase();
  }

  // event base methods
  void attachEventBase(apache::thrift::async::TEventBase*);
  void detachEventBase();

  // Header framing
  virtual std::unique_ptr<folly::IOBuf>
    frameMessage(std::unique_ptr<folly::IOBuf>&&);
  virtual std::unique_ptr<folly::IOBuf>
    removeFrame(folly::IOBufQueue*, size_t& remaining);

  apache::thrift::transport::THeader* getHeader() {
    return header_.get();
  }

  uint16_t getProtocolId() {
    return header_->getProtocolId();
  }

  // If security negotiation has not yet started, begin.  Depending on
  // the availability of keying material, etc., this may be a noop, or
  // it may start exchanging messages to negotiate security, or it may
  // begin asynchronous processing which results in no messages being
  // exchanged at all.  It is not necessary to call this; if
  // THRIFT_HEADER_SASL_CLIENT_TYPE is supported on the channel, this
  // will happen when the first request is made.  Calling this
  // function will just start the work sooner.
  void startSecurity();

  // The default SASL implementation can be overridden for testing or
  // other purposes.  Most users will never need to call this.
  void setSaslClient(std::unique_ptr<SaslClient> client) {
    saslClient_ = std::move(client);
  }

  // Return pointer to sasl client for mutation.
  SaslClient* getSaslClient() {
    return saslClient_.get();
  }

  // Returns the identity of the remote peer.  Value will be empty if
  // security was not negotiated.
  std::string getSaslPeerIdentity() {
    if (protectionState_ == ProtectionState::VALID) {
      return saslClient_->getServerIdentity();
    } else {
      return "";
    }
  }

private:
  bool clientSupportHeader();
  /**
   * Callback to manage the lifetime of a two-way call.
   * Deletes itself when it receives both a send and recv callback.
   * Exceptions:
   * 1) If we get a messageSendError, we will never get a recv callback,
   *    so it is safe to delete.
   * 2) timeoutExpired uninstalls the recv callback, so it is safe to delete
   *    if it was already sent.
   *
   * Deletion automatically uninstalls the timeout.
   */
  class TwowayCallback
      : public MessageChannel::SendCallback
      , public apache::thrift::async::HHWheelTimer::Callback {
   public:
#define X_CHECK_STATE_EQ(state, expected) \
    CHECK_EQ(static_cast<int>(state), static_cast<int>(expected))
#define X_CHECK_STATE_NE(state, expected) \
    CHECK_NE(static_cast<int>(state), static_cast<int>(expected))
    // Keep separate state for send and receive.
    // Starts as queued for receive (as that's how it's created in
    // HeaderClientChannel::sendRequest).
    // We then try to send and either get messageSendError() or sendQueued().
    // If we get sendQueued(), we know to wait for either messageSendError()
    // or messageSent() before deleting.
    TwowayCallback(HeaderClientChannel* channel,
                   uint32_t sendSeqId,
                   uint16_t protoId,
                   std::chrono::milliseconds streamTimeout,
                   std::unique_ptr<RequestCallback> cb,
                   std::unique_ptr<apache::thrift::ContextStack> ctx)
        : channel_(channel)
        , sendSeqId_(sendSeqId)
        , protoId_(protoId)
        , streamTimeout_(streamTimeout)
        , cb_(std::move(cb))
        , ctx_(std::move(ctx))
        , sendState_(QState::INIT)
        , recvState_(QState::QUEUED)
        , cbCalled_(false) { }
    ~TwowayCallback() {
      X_CHECK_STATE_EQ(sendState_, QState::DONE);
      X_CHECK_STATE_EQ(recvState_, QState::DONE);
      CHECK(cbCalled_);
    }
    void sendQueued() {
      X_CHECK_STATE_EQ(sendState_, QState::INIT);
      sendState_ = QState::QUEUED;
    }
    void messageSent() {
      X_CHECK_STATE_EQ(sendState_, QState::QUEUED);
      CHECK(cb_);
      auto old_ctx =
        apache::thrift::async::RequestContext::setContext(cb_->context_);
      cb_->requestSent();
      apache::thrift::async::RequestContext::setContext(old_ctx);
      sendState_ = QState::DONE;
      maybeDeleteThis();
    }
    void messageSendError(std::exception_ptr&& ex) {
      X_CHECK_STATE_NE(sendState_, QState::DONE);
      sendState_ = QState::DONE;
      if (recvState_ == QState::QUEUED) {
        recvState_ = QState::DONE;
        channel_->eraseCallback(sendSeqId_, this);
        cancelTimeout();
      }
      if (!cbCalled_) {
        cbCalled_ = true;
        auto old_ctx =
          apache::thrift::async::RequestContext::setContext(cb_->context_);
        cb_->requestError(ClientReceiveState(ex, std::move(ctx_)));
        apache::thrift::async::RequestContext::setContext(old_ctx);
      }
      delete this;
    }
    void replyReceived(std::unique_ptr<folly::IOBuf> buf,
                       bool serverExpectsStreaming) {
      X_CHECK_STATE_NE(sendState_, QState::INIT);
      X_CHECK_STATE_EQ(recvState_, QState::QUEUED);
      recvState_ = QState::DONE;
      cancelTimeout();

      CHECK(!cbCalled_);
      CHECK(cb_);
      cbCalled_ = true;

      std::unique_ptr<StreamManager> streamManager;
      auto old_ctx =
        apache::thrift::async::RequestContext::setContext(cb_->context_);
      cb_->replyReceived(ClientReceiveState(protoId_,
                                            std::move(buf),
                                            std::move(ctx_),
                                            &streamManager));

      if (streamManager && !streamManager->isDone()) {
        if (serverExpectsStreaming) {
          StreamCallback* stream = new StreamCallback(channel_,
                                                      sendSeqId_,
                                                      streamTimeout_,
                                                      std::move(streamManager));
          channel_->registerStream(sendSeqId_, stream);
          stream->messageSent();

        } else {
          streamManager->cancel();
        }
      }
      apache::thrift::async::RequestContext::setContext(old_ctx);
      maybeDeleteThis();
    }
    void requestError(std::exception_ptr ex) {
      X_CHECK_STATE_EQ(recvState_, QState::QUEUED);
      recvState_ = QState::DONE;
      cancelTimeout();
      CHECK(cb_);
      if (!cbCalled_) {
        cbCalled_ = true;
        auto old_ctx =
          apache::thrift::async::RequestContext::setContext(cb_->context_);
        cb_->requestError(ClientReceiveState(ex, std::move(ctx_)));
        apache::thrift::async::RequestContext::setContext(old_ctx);
      }
      maybeDeleteThis();
    }
    void timeoutExpired() noexcept {
      X_CHECK_STATE_EQ(recvState_, QState::QUEUED);
      channel_->eraseCallback(sendSeqId_, this);
      recvState_ = QState::DONE;

      if (!cbCalled_) {
        using apache::thrift::transport::TTransportException;

        cbCalled_ = true;
        TTransportException ex(TTransportException::TIMED_OUT, "Timed Out");
        ex.setOptions(TTransportException::CHANNEL_IS_VALID);  // framing okay
        auto old_ctx =
          apache::thrift::async::RequestContext::setContext(cb_->context_);
        cb_->requestError(ClientReceiveState(std::make_exception_ptr(ex),
                                             std::move(ctx_)));
        apache::thrift::async::RequestContext::setContext(old_ctx);
      }
      maybeDeleteThis();
    }
   private:
    enum class QState {
      INIT, QUEUED, DONE
    };
    void maybeDeleteThis() {
      if (sendState_ == QState::DONE && recvState_ == QState::DONE) {
        delete this;
      }
    }
    HeaderClientChannel* channel_;
    uint32_t sendSeqId_;
    uint16_t protoId_;
    std::chrono::milliseconds streamTimeout_;
    std::unique_ptr<RequestCallback> cb_;
    std::unique_ptr<apache::thrift::ContextStack> ctx_;
    QState sendState_;
    QState recvState_;
    bool cbCalled_;
#undef X_CHECK_STATE_NE
#undef X_CHECK_STATE_EQ
  };

  class OnewayCallback : public MessageChannel::SendCallback {
   public:
    OnewayCallback(std::unique_ptr<RequestCallback> cb,
                   std::unique_ptr<apache::thrift::ContextStack> ctx)
        : cb_(std::move(cb))
        , ctx_(std::move(ctx)) {}
    void sendQueued() { }
    void messageSent() {
      CHECK(cb_);
      auto old_ctx =
        apache::thrift::async::RequestContext::setContext(cb_->context_);
      cb_->requestSent();
      apache::thrift::async::RequestContext::setContext(old_ctx);
      delete this;
    }
    void messageSendError(std::exception_ptr&& ex) {
      CHECK(cb_);
      auto old_ctx =
        apache::thrift::async::RequestContext::setContext(cb_->context_);
      cb_->requestError(ClientReceiveState(ex, std::move(ctx_)));
      apache::thrift::async::RequestContext::setContext(old_ctx);
      delete this;
    }
   private:
    std::unique_ptr<RequestCallback> cb_;
    std::unique_ptr<apache::thrift::ContextStack> ctx_;
  };

  // Remove a callback from the recvCallbacks_ map.
  void eraseCallback(uint32_t seqId, TwowayCallback* cb);

  // Set the base class callback based on current state.
  void setBaseReceivedCallback();

  class StreamCallback : public MessageChannel::SendCallback,
                         public StreamChannelCallback,
                         public apache::thrift::async::HHWheelTimer::Callback {
    public:
      StreamCallback(HeaderClientChannel* channel,
                     uint32_t sequenceId,
                     std::chrono::milliseconds timeout,
                     std::unique_ptr<StreamManager>&& manager);

      void sendQueued();
      void messageSent();
      void messageSendError(std::exception_ptr&& ex);

      void replyReceived(std::unique_ptr<folly::IOBuf> buf);
      void requestError(std::exception_ptr ex);

      void timeoutExpired() noexcept;

      void onStreamSend(std::unique_ptr<folly::IOBuf>&& buf);
      void onOutOfLoopStreamError(const std::exception_ptr& error);

      ~StreamCallback();

    private:
      HeaderClientChannel* channel_;
      uint32_t sequenceId_;
      std::chrono::milliseconds timeout_;
      std::unique_ptr<StreamManager> manager_;
      bool hasOutstandingSend_;

      void resetTimeout();
      void deleteThisIfNecessary();
  };

  void registerStream(uint32_t seqId, StreamCallback* cb);
  void unregisterStream(uint32_t seqId, StreamCallback* cb);

  std::unique_ptr<folly::IOBuf> handleSecurityMessage(
    std::unique_ptr<folly::IOBuf>&& buf);

  // Returns true if authentication messages are still pending, false
  // otherwise.  As a side effect, if authentication negotiation has
  // not yet begun, this will start exchanging messages.
  bool isSecurityPending();
  void setSecurityComplete(ProtectionState state);

  void maybeSetPriorityHeader(const RpcOptions& rpcOptions);
  void maybeSetTimeoutHeader(const RpcOptions& rpcOptions);

  uint32_t sendSeqId_;
  std::unique_ptr<SaslClient> saslClient_;
  typedef void (HeaderClientChannel::*AfterSecurityMethod)(
    const RpcOptions&,
    std::unique_ptr<RequestCallback>,
    std::unique_ptr<apache::thrift::ContextStack>,
    std::unique_ptr<folly::IOBuf>);
  std::deque<std::tuple<AfterSecurityMethod,
                        RpcOptions,
                        std::unique_ptr<RequestCallback>,
                        std::unique_ptr<apache::thrift::ContextStack>,
                        std::unique_ptr<folly::IOBuf>>> afterSecurity_;
  std::unordered_map<uint32_t, TwowayCallback*> recvCallbacks_;
  std::unordered_map<uint32_t, StreamCallback*> streamCallbacks_;
  std::deque<uint32_t> recvCallbackOrder_;
  std::unique_ptr<apache::thrift::transport::THeader> header_;
  CloseCallback* closeCallback_;

  uint32_t timeout_;
  uint32_t timeoutSASL_;
  uint32_t handshakeMessagesSent_;

  bool keepRegisteredForClose_;
};

}} // apache::thrift

#endif // THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_
