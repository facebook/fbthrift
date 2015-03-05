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

#ifndef THRIFT_ASYNC_CPP2CHANNEL_H_
#define THRIFT_ASYNC_CPP2CHANNEL_H_ 1

#include <thrift/lib/cpp2/async/SaslEndpoint.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp/async/TDelayedDestruction.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <folly/io/IOBufQueue.h>
#include <memory>

#include <folly/wangle/channel/OutputBufferingHandler.h>
#include <thrift/lib/cpp2/async/TAsyncTransportHandler.h>
#include <thrift/lib/cpp2/async/FramingHandler.h>
#include <thrift/lib/cpp2/async/ProtectionHandler.h>

#include <deque>
#include <vector>

namespace apache { namespace thrift {

class Cpp2Channel
  : public MessageChannel,
    public folly::wangle::BytesToBytesHandler {
 protected:
  virtual ~Cpp2Channel() {}

 public:

  explicit Cpp2Channel(
    const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport,
    std::unique_ptr<FramingHandler> framingHandler,
    std::unique_ptr<ProtectionHandler> protectionHandler = nullptr);

  static std::unique_ptr<Cpp2Channel,
                         apache::thrift::async::TDelayedDestruction::Destructor>
  newChannel(
      const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport,
      std::unique_ptr<FramingHandler> framingHandler) {
    return std::unique_ptr<Cpp2Channel,
      apache::thrift::async::TDelayedDestruction::Destructor>(
      new Cpp2Channel(transport, std::move(framingHandler)));
  }

  void closeNow();

  void setTransport(
      const std::shared_ptr<async::TAsyncTransport>& transport) {
    if (!transport || pipeline_->getTransport()) {
      pipeline_->detachTransport();
    }
    if (transport) {
      pipeline_->attachTransport(transport);
    }
  }

  folly::AsyncTransport* getTransport() {
    return pipeline_->getTransport().get();
  }

  // TDelayedDestruction methods
  void destroy() override;

  void processReadEOF() noexcept;

  // Interface from MessageChannel
  void sendMessage(SendCallback* callback,
                   std::unique_ptr<folly::IOBuf>&& buf) override;
  void setReceiveCallback(RecvCallback* callback) override;

  // event base methods
  virtual void attachEventBase(apache::thrift::async::TEventBase*);
  virtual void detachEventBase();
  apache::thrift::async::TEventBase* getEventBase();

  // Setter for queued sends mode.
  // Can only be set in quiescent state, otherwise
  // sendCallbacks_ would be called incorrectly.
  void setQueueSends(bool queueSends) {
    queueSends_ = queueSends;
  }

  ProtectionHandler* getProtectionHandler() const {
    return protectionHandler_.get();
  }

  void setReadBufferSize(uint32_t readBufferSize) {
    CHECK(remaining_ == readBufferSize_); // channel has not been used
    remaining_ = readBufferSize_ = std::max(readBufferSize,
                                            DEFAULT_BUFFER_SIZE);
  }

  // BytesToBytesHandler methods
  void read(Context* ctx, folly::IOBufQueue& q) override;
  void readEOF(Context* ctx) override;
  void readException(Context* ctx, folly::exception_wrapper e) override;
  folly::Future<void> close(Context* ctx) override;

private:
  std::unique_ptr<folly::IOBufQueue> queue_;

  static const uint32_t DEFAULT_BUFFER_SIZE = 2048;
  uint32_t readBufferSize_;
  uint32_t remaining_; // Used to attempt to allocate 'perfect' sized IOBufs

  RecvCallback* recvCallback_;
  bool closing_;
  bool eofInvoked_;

  std::unique_ptr<RecvCallback::sample> sample_;

  // Queued sends feature - optimizes by minimizing syscalls in high-QPS
  // loads for greater throughput, but at the expense of some
  // minor latency increase.
  bool queueSends_;

  std::shared_ptr<ProtectionHandler> protectionHandler_;

  typedef folly::wangle::ChannelPipeline<
    folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>,
    TAsyncTransportHandler,
    folly::wangle::OutputBufferingHandler,
    folly::wangle::ChannelHandlerPtr<ProtectionHandler>,
    folly::wangle::ChannelHandlerPtr<FramingHandler>,
    folly::wangle::ChannelHandlerPtr<Cpp2Channel, false>>
  Pipeline;
  std::unique_ptr<Pipeline, folly::DelayedDestruction::Destructor> pipeline_;
  TAsyncTransportHandler* transportHandler_;
};

}} // apache::thrift

#endif // THRIFT_ASYNC_CPP2CHANNEL_H_
