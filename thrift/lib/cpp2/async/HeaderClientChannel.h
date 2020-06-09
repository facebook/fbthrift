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

#ifndef THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_
#define THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_ 1

#include <deque>
#include <limits>
#include <memory>
#include <unordered_map>

#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/Request.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp/util/THttpParser.h>
#include <thrift/lib/cpp2/async/ChannelCallbacks.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>
#include <thrift/lib/cpp2/async/Cpp2Channel.h>
#include <thrift/lib/cpp2/async/HeaderChannel.h>
#include <thrift/lib/cpp2/async/HeaderChannelTrait.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

namespace apache {
namespace thrift {

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
 protected:
  ~HeaderClientChannel() override {}

 public:
  explicit HeaderClientChannel(
      const std::shared_ptr<folly::AsyncTransport>& transport);

  explicit HeaderClientChannel(const std::shared_ptr<Cpp2Channel>& cpp2Channel);

  typedef std::
      unique_ptr<HeaderClientChannel, folly::DelayedDestruction::Destructor>
          Ptr;

  static Ptr newChannel(
      const std::shared_ptr<folly::AsyncTransport>& transport) {
    return Ptr(new HeaderClientChannel(transport));
  }

  virtual void sendMessage(
      Cpp2Channel::SendCallback* callback,
      std::unique_ptr<folly::IOBuf> buf,
      apache::thrift::transport::THeader* header) {
    cpp2Channel_->sendMessage(callback, std::move(buf), header);
  }

  void closeNow() override;

  // DelayedDestruction methods
  void destroy() override;

  folly::AsyncTransport* getTransport() override {
    return cpp2Channel_->getTransport();
  }

  void setReadBufferSize(uint32_t readBufferSize) {
    cpp2Channel_->setReadBufferSize(readBufferSize);
  }

  // Client interface from RequestChannel
  using RequestChannel::sendRequestNoResponse;
  using RequestChannel::sendRequestResponse;

  void sendRequestResponse(
      const RpcOptions&,
      folly::StringPiece,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) override;

  void sendRequestNoResponse(
      const RpcOptions&,
      folly::StringPiece,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) override;

  void setCloseCallback(CloseCallback*) override;

  // Interface from MessageChannel::RecvCallback
  void messageReceived(
      std::unique_ptr<folly::IOBuf>&&,
      std::unique_ptr<apache::thrift::transport::THeader>&&) override;
  void messageChannelEOF() override;
  void messageReceiveErrorWrapped(folly::exception_wrapper&&) override;

  // Client timeouts for read, write.
  // Servers should use timeout methods on underlying transport.
  void setTimeout(uint32_t ms) override;
  uint32_t getTimeout() override {
    return getTransport()->getSendTimeout();
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

  folly::EventBase* getEventBase() const override {
    return cpp2Channel_->getEventBase();
  }

  /**
   * Set the channel up in HTTP CLIENT mode. host can be an empty string
   */
  void useAsHttpClient(const std::string& host, const std::string& uri);

  bool good() override;

  SaturationStatus getSaturationStatus() override {
    return SaturationStatus(0, std::numeric_limits<uint32_t>::max());
  }

  // event base methods
  void attachEventBase(folly::EventBase*) override;
  void detachEventBase() override;
  bool isDetachable() override;

  uint16_t getProtocolId() override;
  void setProtocolId(uint16_t protocolId) {
    protocolId_ = protocolId;
  }

  bool expireCallback(uint32_t seqId);

  CLIENT_TYPE getClientType() override {
    return HeaderChannelTrait::getClientType();
  }

  class ClientFramingHandler : public FramingHandler {
   public:
    explicit ClientFramingHandler(HeaderClientChannel& channel)
        : channel_(channel) {}

    std::tuple<
        std::unique_ptr<folly::IOBuf>,
        size_t,
        std::unique_ptr<apache::thrift::transport::THeader>>
    removeFrame(folly::IOBufQueue* q) override;

    std::unique_ptr<folly::IOBuf> addFrame(
        std::unique_ptr<folly::IOBuf> buf,
        apache::thrift::transport::THeader* header) override;

   private:
    HeaderClientChannel& channel_;
  };

  // Remove a callback from the recvCallbacks_ map.
  void eraseCallback(uint32_t seqId, TwowayCallback<HeaderClientChannel>* cb);

 protected:
  bool clientSupportHeader() override;

 private:
  void setRequestHeaderOptions(apache::thrift::transport::THeader* header);

  std::shared_ptr<apache::thrift::util::THttpClientParser> httpClientParser_;

  // Set the base class callback based on current state.
  void setBaseReceivedCallback();

  uint32_t sendSeqId_;

  std::unordered_map<uint32_t, TwowayCallback<HeaderClientChannel>*>
      recvCallbacks_;
  std::deque<uint32_t> recvCallbackOrder_;
  CloseCallback* closeCallback_;

  uint32_t timeout_;

  bool keepRegisteredForClose_;

  std::shared_ptr<Cpp2Channel> cpp2Channel_;

  uint16_t protocolId_;
};

} // namespace thrift
} // namespace apache

#endif // THRIFT_ASYNC_THEADERCLIENTCHANNEL_H_
