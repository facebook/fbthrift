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

#include <folly/io/async/HHWheelTimer.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/async/HeaderChannel.h>
#include <thrift/lib/cpp2/async/SaslServer.h>
#include <thrift/lib/cpp2/async/Cpp2Channel.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TDelayedDestruction.h>
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
class HeaderServerChannel : public ResponseChannel,
                            public HeaderChannel,
                            public MessageChannel::RecvCallback,
                            virtual public async::TDelayedDestruction {
  typedef ProtectionHandler::ProtectionState ProtectionState;
protected:
 ~HeaderServerChannel() override {}

 public:
  explicit HeaderServerChannel(
    const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport);

  explicit HeaderServerChannel(
    const std::shared_ptr<Cpp2Channel>& cpp2Channel);

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

  apache::thrift::async::TEventBase* getEventBase() {
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
                  bool outOfOrder,
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

    void sendErrorWrapped(folly::exception_wrapper ex,
                          std::string exCode,
                          MessageChannel::SendCallback* cb = nullptr) override;

   private:
    HeaderServerChannel* channel_;
    std::unique_ptr<apache::thrift::transport::THeader> header_;
    bool outOfOrder_;
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

private:
  ProtectionState getProtectionState() {
    return cpp2Channel_->getProtectionHandler()->getProtectionState();
  }

  void setProtectionState(ProtectionState newState) {
    cpp2Channel_->getProtectionHandler()->setProtectionState(newState,
                                                             saslServer_.get());
  }

  std::unique_ptr<folly::IOBuf> handleSecurityMessage(
      std::unique_ptr<folly::IOBuf>&& buf,
      std::unique_ptr<apache::thrift::transport::THeader>&& header);

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

  static const int MAX_REQUEST_SIZE = 2000;
  static std::atomic<uint32_t> sample_;
  uint32_t sampleRate_;

  uint32_t timeoutSASL_;

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
  } saslServerCallback_;

  std::shared_ptr<Cpp2Channel> cpp2Channel_;

  folly::HHWheelTimer::UniquePtr timer_;
};

}} // apache::thrift

#endif // THRIFT_ASYNC_THEADERSERVERCHANNEL_H_
