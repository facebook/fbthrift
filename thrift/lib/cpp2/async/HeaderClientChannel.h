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

#ifndef THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_
#define THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_ 1

#include <thrift/lib/cpp2/async/ChannelCallbacks.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/HeaderChannel.h>
#include <thrift/lib/cpp2/async/HeaderChannelTrait.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>
#include <thrift/lib/cpp2/async/SaslClient.h>
#include <thrift/lib/cpp2/async/Cpp2Channel.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/Request.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/util/THttpParser.h>
#include <memory>

#include <unordered_map>
#include <deque>

namespace apache { namespace thrift {

/**
 * HeaderClientChannel
 *
 * This is a channel implementation that reads and writes
 * messages encoded using THeaderProtocol.
 */
class HeaderClientChannel : public ClientChannel,
                            public HeaderChannelTrait,
                            public MessageChannel::RecvCallback,
                            public ChannelCallbacks,
                            virtual public folly::DelayedDestruction {
  typedef ProtectionHandler::ProtectionState ProtectionState;
 protected:
  ~HeaderClientChannel() override {}

 public:
  explicit HeaderClientChannel(
    const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport);

  explicit HeaderClientChannel(
    const std::shared_ptr<Cpp2Channel>& cpp2Channel);

  typedef
    std::unique_ptr<HeaderClientChannel,
                    folly::DelayedDestruction::Destructor>
    Ptr;

  static Ptr newChannel(
    const std::shared_ptr<
    apache::thrift::async::TAsyncTransport>& transport) {
    return Ptr(new HeaderClientChannel(transport));
  }

  virtual void sendMessage(Cpp2Channel::SendCallback* callback,
                           std::unique_ptr<folly::IOBuf> buf,
                           apache::thrift::transport::THeader* header) {
    cpp2Channel_->sendMessage(callback, std::move(buf), header);
  }

  void closeNow() override;

  // DelayedDestruction methods
  void destroy() override;

  apache::thrift::async::TAsyncTransport* getTransport() override {
    return cpp2Channel_->getTransport();
  }

  void setReadBufferSize(uint32_t readBufferSize) {
    cpp2Channel_->setReadBufferSize(readBufferSize);
  }

  // Client interface from RequestChannel
  using RequestChannel::sendRequest;
  uint32_t sendRequest(RpcOptions&,
                       std::unique_ptr<RequestCallback>,
                       std::unique_ptr<apache::thrift::ContextStack>,
                       std::unique_ptr<folly::IOBuf>,
                       std::shared_ptr<apache::thrift::transport::THeader>) override;

  using RequestChannel::sendOnewayRequest;
  uint32_t sendOnewayRequest(RpcOptions&,
                             std::unique_ptr<RequestCallback>,
                             std::unique_ptr<apache::thrift::ContextStack>,
                             std::unique_ptr<folly::IOBuf>,
                             std::shared_ptr<apache::thrift::transport::THeader>) override;

  void setCloseCallback(CloseCallback*) override;

  // Interface from MessageChannel::RecvCallback
  void messageReceived(std::unique_ptr<folly::IOBuf>&&,
                       std::unique_ptr<apache::thrift::transport::THeader>&&,
                       std::unique_ptr<MessageChannel::RecvCallback::sample>) override;
  void messageChannelEOF() override;
  void messageReceiveErrorWrapped(folly::exception_wrapper&&) override;

  // Client timeouts for read, write.
  // Servers should use timeout methods on underlying transport.
  void setTimeout(uint32_t ms) override;
  uint32_t getTimeout() override {
    return getTransport()->getSendTimeout();
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

  folly::EventBase* getEventBase() override {
      return cpp2Channel_->getEventBase();
  }

  /**
   * Set the channel up in HTTP CLIENT mode. host can be an empty string
   */
  void useAsHttpClient(const std::string& host, const std::string& uri);

  // event base methods
  void attachEventBase(folly::EventBase*) override;
  void detachEventBase() override;
  bool isDetachable() override;

  uint16_t getProtocolId() override;
  void setProtocolId(uint16_t protocolId) { protocolId_ = protocolId; }

  bool expireCallback(uint32_t seqId);

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
  // Can be set only once.
  void setSaslClient(std::unique_ptr<SaslClient> client) {
    DCHECK(!saslClient_);
    saslClient_ = std::move(client);
  }

  // Return pointer to sasl client for mutation.
  SaslClient* getSaslClient() {
    return saslClient_.get();
  }

  // Returns the identity of the remote peer.  Value will be empty if
  // security was not negotiated.
  std::string getSaslPeerIdentity() {
    if (getProtectionState() == ProtectionState::VALID) {
      return saslClient_->getServerIdentity();
    } else {
      return "";
    }
  }

  // Returns true if security is negotiated and used
  bool isSecurityActive() override {
    return getProtectionState() == ProtectionState::VALID;
  }

  CLIENT_TYPE getClientType() override {
    return HeaderChannelTrait::getClientType();
  }

  class ClientFramingHandler : public FramingHandler {
  public:
    explicit ClientFramingHandler(HeaderClientChannel& channel)
      : channel_(channel) {}

    std::tuple<std::unique_ptr<folly::IOBuf>,
               size_t,
               std::unique_ptr<apache::thrift::transport::THeader>>
    removeFrame(folly::IOBufQueue* q) override;

    std::unique_ptr<folly::IOBuf> addFrame(
        std::unique_ptr<folly::IOBuf> buf,
        apache::thrift::transport::THeader* header) override;
  private:
    HeaderClientChannel& channel_;
  };

  class SaslClientCallback : public SaslClient::Callback {
   public:
    explicit SaslClientCallback(HeaderClientChannel& channel)
      : channel_(channel)
      , header_(new apache::thrift::transport::THeader) {}
    void saslStarted() override;
    void saslSendServer(std::unique_ptr<folly::IOBuf>&&) override;
    void saslError(folly::exception_wrapper&&) override;
    void saslComplete() override;

    void setHeader(
        std::unique_ptr<apache::thrift::transport::THeader>&& header) {
      header_ = std::move(header);
    }

    void setSendServerHook(std::function<void()> hook) {
      sendServerHook_ = std::move(hook);
    }
   private:
    HeaderClientChannel& channel_;
    std::unique_ptr<apache::thrift::transport::THeader> header_;
    std::function<void()> sendServerHook_;
  };

  SaslClientCallback* getSaslClientCallback() {
    return &saslClientCallback_;
  }

  // Remove a callback from the recvCallbacks_ map.
  void eraseCallback(uint32_t seqId, TwowayCallback<HeaderClientChannel>* cb);

protected:
  bool clientSupportHeader() override;
  void setPersistentAuthHeader(bool auth) override {
    setPersistentHeader("thrift_auth", auth ? "1" : "0");
  }

private:

  void setRequestHeaderOptions(apache::thrift::transport::THeader* header);

  std::shared_ptr<apache::thrift::util::THttpClientParser> httpClientParser_;

  // Set the base class callback based on current state.
  void setBaseReceivedCallback();

  std::unique_ptr<folly::IOBuf> handleSecurityMessage(
    std::unique_ptr<folly::IOBuf>&& buf,
    std::unique_ptr<apache::thrift::transport::THeader>&& header);

  // Returns true if authentication messages are still pending, false
  // otherwise.  As a side effect, if authentication negotiation has
  // not yet begun, this will start exchanging messages.
  bool isSecurityPending();
  void setSecurityComplete(ProtectionState state);

  uint32_t sendSeqId_;
  uint32_t sendSecurityPendingSeqId_;

  std::unique_ptr<SaslClient> saslClient_;

  typedef uint32_t (HeaderClientChannel::*AfterSecurityMethod)(
    RpcOptions&,
    std::unique_ptr<RequestCallback>,
    std::unique_ptr<apache::thrift::ContextStack>,
    std::unique_ptr<folly::IOBuf>,
    std::shared_ptr<apache::thrift::transport::THeader>);
  std::deque<std::tuple<AfterSecurityMethod,
                        RpcOptions,
                        std::unique_ptr<RequestCallback>,
                        std::unique_ptr<apache::thrift::ContextStack>,
                        std::unique_ptr<folly::IOBuf>,
                        std::shared_ptr<apache::thrift::transport::THeader>>> afterSecurity_;
  std::unordered_map<uint32_t, TwowayCallback<HeaderClientChannel>*>
      recvCallbacks_;
  std::deque<uint32_t> recvCallbackOrder_;
  CloseCallback* closeCallback_;

  uint32_t timeout_;
  uint32_t timeoutSASL_;
  uint32_t handshakeMessagesSent_;

  bool keepRegisteredForClose_;

  ProtectionState getProtectionState() {
    return cpp2Channel_->getProtectionHandler()->getProtectionState();
  }

  void setProtectionState(ProtectionState newState) {
    cpp2Channel_->getProtectionHandler()->setProtectionState(newState,
                                                             saslClient_.get());
  }

  SaslClientCallback saslClientCallback_;

  std::shared_ptr<Cpp2Channel> cpp2Channel_;

  uint16_t protocolId_;
  uint16_t userProtocolId_;
};

}} // apache::thrift

#endif // THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_
