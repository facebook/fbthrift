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

#ifndef THRIFT_ASYNC_THTTPCLIENTCHANNEL_H_
#define THRIFT_ASYNC_THTTPCLIENTCHANNEL_H_ 1

#include <folly/io/async/HHWheelTimer.h>
#include <proxygen/lib/http/codec/HTTPCodec.h>
#include <thrift/lib/cpp2/async/ChannelCallbacks.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/HeaderChannel.h>
#include <thrift/lib/cpp2/async/Cpp2Channel.h>
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
class HTTPClientChannel : public RequestChannel,
                          public HeaderCapableChannel,
                          public MessageChannel::RecvCallback,
                          public ChannelCallbacks,
                          virtual public folly::DelayedDestruction {
 protected:
  ~HTTPClientChannel() override {}

 public:
  explicit HTTPClientChannel(
      const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport,
      const std::string& host,
      const std::string& url,
      std::unique_ptr<proxygen::HTTPCodec> codec);

  explicit HTTPClientChannel(
      const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport,
      const std::string& host,
      const std::string& url);

  explicit HTTPClientChannel(const std::shared_ptr<Cpp2Channel>& cpp2Channel,
                             const std::string& host,
                             const std::string& url);

  explicit HTTPClientChannel(const std::shared_ptr<Cpp2Channel>& cpp2Channel,
                             const std::string& host,
                             const std::string& url,
                             std::unique_ptr<proxygen::HTTPCodec> codec);

  typedef std::unique_ptr<
      HTTPClientChannel,
      folly::DelayedDestruction::Destructor> Ptr;

  static Ptr newChannel(
      const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport,
      const std::string& host,
      const std::string& url) {
    return Ptr(new HTTPClientChannel(transport, host, url));
  }

  virtual void sendMessage(Cpp2Channel::SendCallback* callback,
                           std::unique_ptr<folly::IOBuf> buf,
                           apache::thrift::transport::THeader* header) {
    cpp2Channel_->sendMessage(callback, std::move(buf), header);
  }

  void closeNow();

  // DelayedDestruction methods
  void destroy() override;

  apache::thrift::async::TAsyncTransport* getTransport() {
    return cpp2Channel_->getTransport();
  }

  void setReadBufferSize(uint32_t readBufferSize) {
    cpp2Channel_->setReadBufferSize(readBufferSize);
  }

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

  // Interface from MessageChannel::RecvCallback
  void messageReceived(
      std::unique_ptr<folly::IOBuf>&&,
      std::unique_ptr<apache::thrift::transport::THeader>&&,
      std::unique_ptr<MessageChannel::RecvCallback::sample>) override;
  void messageChannelEOF() override;
  void messageReceiveErrorWrapped(folly::exception_wrapper&&) override;

  // Client timeouts for read, write.
  // Servers should use timeout methods on underlying transport.
  void setTimeout(uint32_t ms);
  uint32_t getTimeout() { return getTransport()->getSendTimeout(); }

  // If a Close Callback is set, should we reregister callbacks for it
  // alone?  Basically, this means that loop() will return if the only thing
  // outstanding is close callbacks.
  void setKeepRegisteredForClose(bool keepRegisteredForClose) {
    keepRegisteredForClose_ = keepRegisteredForClose;
    setBaseReceivedCallback();
  }

  bool getKeepRegisteredForClose() { return keepRegisteredForClose_; }

  folly::EventBase* getEventBase() override {
    return cpp2Channel_->getEventBase();
  }

  // event base methods
  void attachEventBase(folly::EventBase*);
  void detachEventBase();
  bool isDetachable();

  uint16_t getProtocolId() override { return protocolId_; }
  void setProtocolId(uint16_t protocolId) { protocolId_ = protocolId; }

  bool expireCallback(uint32_t seqId);

  bool isSecurityActive() { return false; }

  class ClientFramingHandler : public FramingHandler {
   public:
    explicit ClientFramingHandler(HTTPClientChannel& channel)
        : channel_(channel) {}

    std::tuple<std::unique_ptr<folly::IOBuf>,
               size_t,
               std::unique_ptr<apache::thrift::transport::THeader>>
    removeFrame(folly::IOBufQueue* q) override;

    std::unique_ptr<folly::IOBuf> addFrame(
        std::unique_ptr<folly::IOBuf> buf,
        apache::thrift::transport::THeader* header) override;

   private:
    HTTPClientChannel& channel_;
  };

  // Remove a callback from the recvCallbacks_ map.
  void eraseCallback(uint32_t seqId, TwowayCallback<HTTPClientChannel>* cb);

 private:
  class HTTPCodecCallback : public proxygen::HTTPCodec::Callback {
   public:
    explicit HTTPCodecCallback(HTTPClientChannel* channel)
        : channel_(channel) {}

    virtual void onMessageBegin(proxygen::HTTPCodec::StreamID stream,
                                proxygen::HTTPMessage* msg) override {}

    virtual void onHeadersComplete(
        proxygen::HTTPCodec::StreamID stream,
        std::unique_ptr<proxygen::HTTPMessage> msg) override {
      channel_->streamIDToMsg_[stream] = std::move(msg);
      channel_->streamIDToBody_[stream] =
          folly::make_unique<folly::IOBufQueue>();
    }

    virtual void onBody(proxygen::HTTPCodec::StreamID stream,
                        std::unique_ptr<folly::IOBuf> chain,
                        uint16_t padding) override {
      channel_->streamIDToBody_[stream]->append(std::move(chain));
    }

    virtual void onTrailersComplete(
        proxygen::HTTPCodec::StreamID stream,
        std::unique_ptr<proxygen::HTTPHeaders> trailers) override {
      channel_->streamIDToMsg_[stream]->setTrailers(std::move(trailers));
    }

    virtual void onMessageComplete(proxygen::HTTPCodec::StreamID stream,
                                   bool upgrade) override {
      channel_->completedStreamIDs_.push_back(stream);
    }

    virtual void onError(proxygen::HTTPCodec::StreamID stream,
                         const proxygen::HTTPException& error,
                         bool newTxn = false) override {
      // TODO
    }

   private:
    HTTPClientChannel* channel_;
  };

  void setRequestHeaderOptions(apache::thrift::transport::THeader* header);

  // Set the base class callback based on current state.
  void setBaseReceivedCallback();

  std::unique_ptr<HTTPCodecCallback> httpCallback_;
  const std::unique_ptr<proxygen::HTTPCodec> httpCodec_;
  const std::string httpHost_;
  const std::string httpUrl_;

  uint32_t sendSeqId_;

  std::unordered_map<proxygen::HTTPCodec::StreamID, uint32_t> streamIDToSeqId_;
  std::unordered_map<proxygen::HTTPCodec::StreamID,
                     std::unique_ptr<proxygen::HTTPMessage>> streamIDToMsg_;
  std::unordered_map<proxygen::HTTPCodec::StreamID,
                     std::unique_ptr<folly::IOBufQueue>> streamIDToBody_;
  std::deque<proxygen::HTTPCodec::StreamID> completedStreamIDs_;

  std::unordered_map<uint32_t, TwowayCallback<HTTPClientChannel>*>
      recvCallbacks_;
  std::deque<uint32_t> recvCallbackOrder_;
  CloseCallback* closeCallback_;

  uint32_t timeout_;

  bool keepRegisteredForClose_;

  std::shared_ptr<Cpp2Channel> cpp2Channel_;

  folly::HHWheelTimer::UniquePtr timer_;

  uint16_t protocolId_;
};
}
} // apache::thrift

#endif // THRIFT_ASYNC_THTTPCLIENTCHANNEL_H_
