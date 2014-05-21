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
#ifndef THRIFT_ASYNC_THEADERSERVERCHANNEL_H_
#define THRIFT_ASYNC_THEADERSERVERCHANNEL_H_ 1

#include "thrift/lib/cpp/async/HHWheelTimer.h"
#include "thrift/lib/cpp2/async/MessageChannel.h"
#include "thrift/lib/cpp2/async/ResponseChannel.h"
#include "thrift/lib/cpp2/async/SaslServer.h"
#include "thrift/lib/cpp2/async/Cpp2Channel.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"
#include "thrift/lib/cpp/async/TDelayedDestruction.h"
#include "thrift/lib/cpp/transport/THeader.h"
#include <memory>

#include <unordered_map>

namespace apache { namespace thrift {

/**
 * HeaderServerChannel
 *
 * This is a server channel implementation that
 * manages requests / responses via seqId.
 */
class HeaderServerChannel : public ResponseChannel,
                            public MessageChannel::RecvCallback,
                            protected Cpp2Channel {
 private:
  virtual ~HeaderServerChannel(){}

 public:
  explicit HeaderServerChannel(
    const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport);

  static std::unique_ptr<HeaderServerChannel,
                         apache::thrift::async::TDelayedDestruction::Destructor>
  newChannel(
    const std::shared_ptr<
    apache::thrift::async::TAsyncTransport>& transport) {
    return std::unique_ptr<HeaderServerChannel,
      apache::thrift::async::TDelayedDestruction::Destructor>(
      new HeaderServerChannel(transport));
  }

  // TDelayedDestruction methods
  void destroy();

  apache::thrift::async::TAsyncTransport* getTransport() {
    return transport_.get();
  }

  void setTransport(
    std::shared_ptr<apache::thrift::async::TAsyncTransport> transport) {
    transport_ = transport;
  }

  // Server interface from ResponseChannel
  void setCallback(ResponseChannel::Callback* callback) {
    callback_ = callback;

    if (callback) {
      setReceiveCallback(this);
    } else {
      setReceiveCallback(nullptr);
    }
  }

  void sendStreamingMessage(SendCallback* callback,
                            std::unique_ptr<folly::IOBuf>&& buf,
                            HEADER_FLAGS streamFlag);

  // Interface from MessageChannel::RecvCallback
  bool shouldSample();
  void messageReceived(std::unique_ptr<folly::IOBuf>&&,
                       std::unique_ptr<sample>);
  void messageChannelEOF();
  void messageReceiveError(std::exception_ptr&&);
  void messageReceiveErrorWrapped(folly::exception_wrapper&&);

  // Header framing
  virtual std::unique_ptr<folly::IOBuf>
    frameMessage(std::unique_ptr<folly::IOBuf>&&);
  virtual std::unique_ptr<folly::IOBuf>
    removeFrame(folly::IOBufQueue*, size_t& remaining);

  apache::thrift::async::TEventBase* getEventBase() {
      return Cpp2Channel::getEventBase();
  }

  void sendCatchupRequests(
    std::unique_ptr<folly::IOBuf> next_req,
    MessageChannel::SendCallback* cb,
    std::vector<uint16_t> transforms,
    apache::thrift::transport::THeader::StringToStringMap&&);

  class Stream : public StreamChannelCallback,
                 public MessageChannel::SendCallback,
                 public apache::thrift::async::HHWheelTimer::Callback {
    public:
      Stream(HeaderServerChannel* channel,
             uint32_t sequenceId,
             const std::vector<uint16_t>& trans,
             const std::map<std::string, std::string>& headers,
             std::chrono::milliseconds timeout,
             std::unique_ptr<folly::IOBuf>&& buffer,
             std::unique_ptr<StreamManager>&& manager,
             MessageChannel::SendCallback* sendCallback);

      ~Stream();

      void sendQueued();
      void messageSent();
      void messageSendError(std::exception_ptr&&);
      void messageSendErrorWrapped(folly::exception_wrapper&&);

      void onStreamSend(std::unique_ptr<folly::IOBuf>&& data);
      void onOutOfLoopStreamError(const std::exception_ptr& error);

      void timeoutExpired() noexcept;
      void onChannelDestroy();

      void notifyError(const std::exception_ptr& error);
      void notifyReceive(std::unique_ptr<folly::IOBuf>&& buf);

    private:
      HeaderServerChannel* channel_;
      uint32_t sequenceId_;
      std::vector<uint16_t> transforms_;
      std::map<std::string, std::string> headers_;
      std::chrono::milliseconds timeout_;
      std::unique_ptr<folly::IOBuf> buffer_;
      std::unique_ptr<StreamManager> manager_;
      MessageChannel::SendCallback* sendCallback_;
      bool hasOutstandingSend_;

      bool hasSendCallback();
      void resetTimeout();
      void sendStreamingMessage(std::unique_ptr<folly::IOBuf>&& buf,
                                SendCallback* callback);
      void deleteThisIfNecessary();
  };

  class HeaderRequest : public Request {
   public:
    HeaderRequest(uint32_t seqId,
                  HeaderServerChannel* channel,
                  std::unique_ptr<folly::IOBuf>&& buf,
                  const std::map<std::string, std::string>& headers,
                  const std::vector<uint16_t>& trans,
                  bool outOfOrder,
                  bool clientExpectsStreams,
                  Stream** streamPtrReturn,
                  std::unique_ptr<sample> sample);

    bool isActive() { return active_; }
    void cancel() { active_ = false; }

    bool isOneway() {return seqId_ == ONEWAY_REQUEST_ID; }

    void sendReply(std::unique_ptr<folly::IOBuf>&& buf,
                   MessageChannel::SendCallback* cb = nullptr) {
      apache::thrift::transport::THeader::StringToStringMap headers;
      sendReply(std::move(buf), cb, std::move(headers));
    }
    void sendReply(std::unique_ptr<folly::IOBuf>&&,
                   MessageChannel::SendCallback* cb,
                   apache::thrift::transport::THeader::StringToStringMap&&);
    void sendError(std::exception_ptr ex,
                   std::string exCode,
                   MessageChannel::SendCallback* cb = nullptr);

    // XXX this function should only be called in the async thread, which is
    //     fine because we do not support sync versions of streams on the
    //     server side
    void sendReplyWithStreams(std::unique_ptr<folly::IOBuf>&&,
                              std::unique_ptr<StreamManager>&&,
                              MessageChannel::SendCallback* cb = nullptr);
    void setStreamTimeout(const std::chrono::milliseconds& timeout);

   private:
    HeaderServerChannel* channel_;
    uint32_t seqId_;
    std::map<std::string, std::string> headers_;
    std::vector<uint16_t> transforms_;
    bool outOfOrder_;
    bool clientExpectsStreams_;
    std::chrono::milliseconds streamTimeout_;
    Stream** streamPtrReturn_;
    std::atomic<bool> active_;
  };

  apache::thrift::transport::THeader* getHeader() {
    return header_.get();
  }

  // The default SASL implementation can be overridden for testing or
  // other purposes.  Most users will never need to call this.
  void setSaslServer(std::unique_ptr<SaslServer> server) {
    saslServer_ = std::move(server);
  }

  // Return pointer to sasl server for mutation.
  SaslServer* getSaslServer() {
    return saslServer_.get();
  }

  // Returns the identity of the remote peer.  Value will be empty if
  // security was not negotiated.
  std::string getSaslPeerIdentity() {
    if (protectionState_ == ProtectionState::VALID) {
      return saslServer_->getClientIdentity();
    } else {
      return "";
    }
  }

  void setSampleRate(uint32_t sampleRate) {
    sampleRate_ = sampleRate;
  }

  void unregisterStream(uint32_t sequenceId);
  void destroyStreams();

  void setQueueSends(bool queueSends) {
    Cpp2Channel::setQueueSends(queueSends);
  }

  void closeNow() {
    Cpp2Channel::closeNow();
  }

private:
  std::unique_ptr<folly::IOBuf> handleSecurityMessage(
      std::unique_ptr<folly::IOBuf>&& buf);

  std::string getTransportDebugString(
      apache::thrift::async::TAsyncTransport *transport);

  std::unique_ptr<apache::thrift::transport::THeader> header_;
  ResponseChannel::Callback* callback_;
  std::unique_ptr<SaslServer> saslServer_;

  // For backwards-compatible in-order responses support
  std::unordered_map<
    uint32_t,
    std::tuple<
      MessageChannel::SendCallback*,
      std::unique_ptr<folly::IOBuf>,
      std::vector<uint16_t>,
      apache::thrift::transport::THeader::StringToStringMap>> inOrderRequests_;
  std::unordered_map<uint32_t, Stream*> streams_;

  uint32_t arrivalSeqId_;
  uint32_t lastWrittenSeqId_;
  static const int MAX_REQUEST_SIZE = 2000;
  static std::atomic<uint32_t> sample_;
  uint32_t sampleRate_;

  uint32_t timeoutSASL_;

  apache::thrift::async::HHWheelTimer::UniquePtr timer_;

  class SaslServerCallback : public SaslServer::Callback {
   public:
    explicit SaslServerCallback(HeaderServerChannel& channel)
      : channel_(channel) {}
    virtual void saslSendClient(std::unique_ptr<folly::IOBuf>&&);
    virtual void saslError(std::exception_ptr&&);
    virtual void saslComplete();
   private:
    HeaderServerChannel& channel_;
  } saslServerCallback_;
};

}} // apache::thrift

#endif // THRIFT_ASYNC_THEADERSERVERCHANNEL_H_
