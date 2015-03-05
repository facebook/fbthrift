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

#ifndef THRIFT_ASYNC_THEADERSERVERCHANNEL_H_
#define THRIFT_ASYNC_THEADERSERVERCHANNEL_H_ 1

#include <thrift/lib/cpp/async/HHWheelTimer.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/async/SaslServer.h>
#include <thrift/lib/cpp2/async/Cpp2Channel.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TDelayedDestruction.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ProtectionHandler.h>
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
                            virtual public async::TDelayedDestruction {
  typedef ProtectionHandler::ProtectionState ProtectionState;
protected:
  virtual ~HeaderServerChannel(){}

 public:
  explicit HeaderServerChannel(
    const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport);

  explicit HeaderServerChannel(
    const std::shared_ptr<Cpp2Channel>& cpp2Channel,
    std::shared_ptr<apache::thrift::async::TAsyncTransport> transport =
      nullptr);

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

  folly::AsyncTransport* getTransport() {
    return cpp2Channel_->getTransport();
  }

  void setTransport(
    std::shared_ptr<apache::thrift::async::TAsyncTransport> transport) {
    cpp2Channel_->setTransport(transport);
  }

  // Server interface from ResponseChannel
  void setCallback(ResponseChannel::Callback* callback) {
    callback_ = callback;

    if (callback) {
      cpp2Channel_->setReceiveCallback(this);
    } else {
      cpp2Channel_->setReceiveCallback(nullptr);
    }
  }

  virtual void sendMessage(Cpp2Channel::SendCallback* callback,
                   std::unique_ptr<folly::IOBuf> buf) {
    cpp2Channel_->sendMessage(callback, std::move(buf));
  }

  // Interface from MessageChannel::RecvCallback
  bool shouldSample();
  void messageReceived(std::unique_ptr<folly::IOBuf>&&,
                       std::unique_ptr<sample>);
  void messageChannelEOF();
  void messageReceiveErrorWrapped(folly::exception_wrapper&&);

  apache::thrift::async::TEventBase* getEventBase() {
      return cpp2Channel_->getEventBase();
  }

  void sendCatchupRequests(
    std::unique_ptr<folly::IOBuf> next_req,
    MessageChannel::SendCallback* cb,
    std::vector<uint16_t> transforms,
    apache::thrift::transport::THeader::StringToStringMap&&);

  class HeaderRequest : public Request {
   public:
    HeaderRequest(uint32_t seqId,
                  HeaderServerChannel* channel,
                  std::unique_ptr<folly::IOBuf>&& buf,
                  const std::map<std::string, std::string>& headers,
                  const std::vector<uint16_t>& trans,
                  bool outOfOrder,
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
    void sendErrorWrapped(folly::exception_wrapper ex,
                          std::string exCode,
                          MessageChannel::SendCallback* cb = nullptr) {
      apache::thrift::transport::THeader::StringToStringMap headers;
      sendErrorWrapped(ex, exCode, cb, std::move(headers));
    }
    void sendErrorWrapped(
      folly::exception_wrapper ex,
      std::string exCode,
      MessageChannel::SendCallback* cb,
      apache::thrift::transport::THeader::StringToStringMap&& headers);

   private:
    HeaderServerChannel* channel_;
    uint32_t seqId_;
    std::map<std::string, std::string> headers_;
    std::vector<uint16_t> transforms_;
    bool outOfOrder_;
    std::atomic<bool> active_;
  };

  apache::thrift::transport::THeader* getHeader() {
    return header_.get();
  }

  std::shared_ptr<apache::thrift::transport::THeader> getHeaderShared() {
    return header_;
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
    explicit ServerFramingHandler(
        std::shared_ptr<apache::thrift::transport::THeader> header)
      : header_(header) {}

    std::pair<std::unique_ptr<folly::IOBuf>, size_t>
    removeFrame(folly::IOBufQueue* q) override;

    std::unique_ptr<folly::IOBuf> addFrame(std::unique_ptr<folly::IOBuf> buf) override;
  private:
    std::shared_ptr<apache::thrift::transport::THeader> header_;
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
      std::unique_ptr<folly::IOBuf>&& buf);

  static std::string getTransportDebugString(
      apache::thrift::async::TAsyncTransport *transport);

  std::shared_ptr<apache::thrift::transport::THeader> header_;
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

  uint32_t arrivalSeqId_;
  uint32_t lastWrittenSeqId_;

  // Save seqIds from inorder requests so they can be written back later
  std::deque<uint32_t> inorderSeqIds_;
  static const int MAX_REQUEST_SIZE = 2000;
  static std::atomic<uint32_t> sample_;
  uint32_t sampleRate_;

  uint32_t timeoutSASL_;

  class SaslServerCallback : public SaslServer::Callback {
   public:
    explicit SaslServerCallback(HeaderServerChannel& channel)
      : channel_(channel) {}
    virtual void saslSendClient(std::unique_ptr<folly::IOBuf>&&);
    virtual void saslError(folly::exception_wrapper&&);
    virtual void saslComplete();
   private:
    HeaderServerChannel& channel_;
  } saslServerCallback_;

  std::shared_ptr<Cpp2Channel> cpp2Channel_;

  apache::thrift::async::HHWheelTimer::UniquePtr timer_;
};

}} // apache::thrift

#endif // THRIFT_ASYNC_THEADERSERVERCHANNEL_H_
