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

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/ServerChannel.h>
#include <thrift/lib/cpp2/async/HeaderChannelTrait.h>
#include <thrift/lib/cpp2/async/SaslServer.h>
#include <thrift/lib/cpp2/async/Cpp2Channel.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <folly/io/async/DelayedDestruction.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <memory>

#include <unordered_map>

namespace apache { namespace thrift {

/**
 * HeaderServerChannel
 *
 * This is a server channel implementation that
 * manages requests / responses via seqId.
 */
class HeaderServerChannel : public ServerChannel,
                            public HeaderChannelTrait,
                            public MessageChannel::RecvCallback,
                            virtual public folly::DelayedDestruction {
  typedef ProtectionHandler::ProtectionState ProtectionState;
protected:
 ~HeaderServerChannel() override {}

 public:
  explicit HeaderServerChannel(
    const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport);

  explicit HeaderServerChannel(
    const std::shared_ptr<Cpp2Channel>& cpp2Channel);

  static std::unique_ptr<HeaderServerChannel,
                         folly::DelayedDestruction::Destructor>
  newChannel(
    const std::shared_ptr<
    apache::thrift::async::TAsyncTransport>& transport) {
    return std::unique_ptr<HeaderServerChannel,
      folly::DelayedDestruction::Destructor>(
      new HeaderServerChannel(transport));
  }

  // DelayedDestruction methods
  void destroy() override;

  apache::thrift::async::TAsyncTransport* getTransport() {
    return cpp2Channel_->getTransport();
  }

  void setTransport(
    std::shared_ptr<apache::thrift::async::TAsyncTransport> transport) {
    cpp2Channel_->setTransport(transport);
  }

  // Server interface from ResponseChannel
  void setCallback(ResponseChannel::Callback* callback) override {
    callback_ = callback;

    if (callback) {
      cpp2Channel_->setReceiveCallback(this);
    } else {
      cpp2Channel_->setReceiveCallback(nullptr);
    }
  }

  virtual void sendMessage(Cpp2Channel::SendCallback* callback,
                           std::unique_ptr<folly::IOBuf> buf,
                           apache::thrift::transport::THeader* header) {
    cpp2Channel_->sendMessage(callback, std::move(buf), header);
  }

  // Interface from MessageChannel::RecvCallback
  bool shouldSample() override;
  void messageReceived(std::unique_ptr<folly::IOBuf>&&,
                       std::unique_ptr<apache::thrift::transport::THeader>&&,
                       std::unique_ptr<sample>) override;
  void messageChannelEOF() override;
  void messageReceiveErrorWrapped(folly::exception_wrapper&&) override;

  folly::EventBase* getEventBase() {
      return cpp2Channel_->getEventBase();
  }

  void sendCatchupRequests(
    std::unique_ptr<folly::IOBuf> next_req,
    MessageChannel::SendCallback* cb,
    apache::thrift::transport::THeader* header);

  class HeaderRequest : public Request {
   public:
    HeaderRequest(HeaderServerChannel* channel,
                  std::unique_ptr<folly::IOBuf>&& buf,
                  std::unique_ptr<apache::thrift::transport::THeader>&& header,
                  std::unique_ptr<sample> sample);

    bool isActive() override { return active_; }
    void cancel() override { active_ = false; }

    bool isOneway() override {
      return header_->getSequenceNumber() == ONEWAY_REQUEST_ID;
    }

    void setInOrderRecvSequenceId(uint32_t seqId) { InOrderRecvSeqId_ = seqId; }

    apache::thrift::transport::THeader* getHeader() {return header_.get(); }

    void sendReply(std::unique_ptr<folly::IOBuf>&&,
                   MessageChannel::SendCallback* cb = nullptr) override;

    void serializeAndSendError(
      apache::thrift::transport::THeader& header,
      TApplicationException& tae,
      const std::string& methodName,
      int32_t protoSeqId,
      MessageChannel::SendCallback* cb);

    void sendErrorWrapped(folly::exception_wrapper ex,
                          std::string exCode,
                          MessageChannel::SendCallback* cb = nullptr) override;

    void sendErrorWrapped(folly::exception_wrapper ex,
                          std::string exCode,
                          const std::string& methodName,
                          int32_t protoSeqId,
                          MessageChannel::SendCallback* cb = nullptr);

    /* We differentiate between two types of timeouts:
       1) Task timeouts refer to timeouts that fire while the request is
       currently being proceesed
       2) Queue timeouts refer to timeouts that fire before processing
       of the request has begun
    */
    enum TimeoutResponseType {
      TASK,
      QUEUE
    };

    void sendTimeoutResponse(const std::string& methodName,
                             int32_t protoSeqId,
                             MessageChannel::SendCallback* cb,
                             const std::map<std::string, std::string>& headers,
                             TimeoutResponseType responseType);

   private:
    HeaderServerChannel* channel_;
    std::unique_ptr<apache::thrift::transport::THeader> header_;
    std::unique_ptr<apache::thrift::transport::THeader> timeoutHeader_;
    uint32_t InOrderRecvSeqId_{0}; // Used internally for in-order requests
    std::atomic<bool> active_;
  };

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
    if (getProtectionState() == ProtectionState::VALID) {
      return saslServer_->getClientIdentity();
    } else {
      return "";
    }
  }

  void setSampleRate(uint32_t sampleRate) {
    sampleRate_ = sampleRate;
  }

  void setQueueSends(bool queueSends) {
    cpp2Channel_->setQueueSends(queueSends);
  }

  void setDefaultWriteTransforms(std::vector<uint16_t>& writeTrans) {
    writeTrans_ = writeTrans;
  }

  std::vector<uint16_t>& getDefaultWriteTransforms() { return writeTrans_; }

  void closeNow() {
    cpp2Channel_->closeNow();
  }

  class ServerFramingHandler : public FramingHandler {
  public:
    explicit ServerFramingHandler(HeaderServerChannel& channel)
      : channel_(channel) {}

    std::tuple<std::unique_ptr<folly::IOBuf>,
               size_t,
               std::unique_ptr<apache::thrift::transport::THeader>>
    removeFrame(folly::IOBufQueue* q) override;

    std::unique_ptr<folly::IOBuf> addFrame(
        std::unique_ptr<folly::IOBuf> buf,
        apache::thrift::transport::THeader* header) override;
  private:
    HeaderServerChannel& channel_;
  };

  class ServerSaslNegotiationHandler : public SaslNegotiationHandler {
  public:
    explicit ServerSaslNegotiationHandler(HeaderServerChannel& channel)
      : channel_(channel) {}

    bool handleSecurityMessage(
        std::unique_ptr<folly::IOBuf>&& buf,
        std::unique_ptr<apache::thrift::transport::THeader>&& header) override;

   private:
    HeaderServerChannel& channel_;
  };

  class SaslServerCallback : public SaslServer::Callback {
   public:
    explicit SaslServerCallback(HeaderServerChannel& channel)
      : channel_(channel), header_(nullptr) {}
    void saslSendClient(std::unique_ptr<folly::IOBuf>&&) override;
    void saslError(folly::exception_wrapper&&) override;
    void saslComplete() override;

    void setHeader(
        std::unique_ptr<apache::thrift::transport::THeader>&& header) {
      header_ = std::move(header);
    }
   private:
    HeaderServerChannel& channel_;
    std::unique_ptr<apache::thrift::transport::THeader> header_;
  };

  SaslServerCallback* getSaslServerCallback() {
    return &saslServerCallback_;
  }


protected:
  void setPersistentAuthHeader(bool auth) override {
    setPersistentHeader("thrift_auth", auth ? "1" : "0");
  }

private:
  ProtectionState getProtectionState() {
    return cpp2Channel_->getProtectionHandler()->getProtectionState();
  }

  void setProtectionState(ProtectionState newState) {
    cpp2Channel_->getProtectionHandler()->setProtectionState(newState,
                                                             saslServer_.get());
  }

  static std::string getTHeaderPayloadString(folly::IOBuf* buf);
  static std::string getTransportDebugString(
      apache::thrift::async::TAsyncTransport *transport);

  ResponseChannel::Callback* callback_;
  std::unique_ptr<SaslServer> saslServer_;

  // For backwards-compatible in-order responses support
  std::unordered_map<
    uint32_t,
    std::tuple<
      MessageChannel::SendCallback*,
      std::unique_ptr<folly::IOBuf>,
      std::unique_ptr<apache::thrift::transport::THeader>>> inOrderRequests_;

  uint32_t arrivalSeqId_;
  uint32_t lastWrittenSeqId_;

  folly::Optional<bool> outOfOrder_;

  std::vector<uint16_t> writeTrans_;

  static const int MAX_REQUEST_SIZE = 2000;
  static std::atomic<uint32_t> sample_;
  uint32_t sampleRate_;

  uint32_t timeoutSASL_;

  SaslServerCallback saslServerCallback_;

  std::shared_ptr<Cpp2Channel> cpp2Channel_;
};

}} // apache::thrift

#endif // THRIFT_ASYNC_THEADERSERVERCHANNEL_H_
