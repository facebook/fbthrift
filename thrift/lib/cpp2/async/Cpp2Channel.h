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

#include <deque>
#include <vector>

namespace apache { namespace thrift {

class ProtectionChannelHandler {
public:
  enum class ProtectionState {
    UNKNOWN,
    NONE,
    INPROGRESS,
    VALID,
    INVALID,
  };

  ProtectionChannelHandler()
    : protectionState_(ProtectionState::UNKNOWN)
    , saslEndpoint_(nullptr)
  {}

  void setProtectionState(ProtectionState protectionState,
                          SaslEndpoint* saslEndpoint = nullptr) {
    protectionState_ = protectionState;
    saslEndpoint_ = saslEndpoint;
  }

  ProtectionState getProtectionState() {
    return protectionState_;
  }

  /**
   * If q contains enough data, read it (removing it from q, but retaining
   * following data), decrypt it and return as result.first.
   * result.second is set to 0.
   *
   * If q doesn't contain enough data, return an empty unique_ptr in
   * result.first and return the requested amount of bytes in result.second.
   */
  std::pair<std::unique_ptr<folly::IOBuf>, size_t>
  decrypt(folly::IOBufQueue* q);

  /**
   * Encrypt an IOBuf
   */
  std::unique_ptr<folly::IOBuf> encrypt(std::unique_ptr<folly::IOBuf> buf);
private:
  ProtectionState protectionState_;
  SaslEndpoint* saslEndpoint_;
};

class FramingChannelHandler {
public:
  virtual ~FramingChannelHandler() {}

  /**
   * If q contains enough data, read it (removing it from q, but retaining
   * following data), unframe it and return as result.first.
   * result.second is set to 0.
   *
   * If q doesn't contain enough data, return an empty unique_ptr in
   * result.first and return the requested amount of bytes in result.second.
   */
  virtual std::pair<std::unique_ptr<folly::IOBuf>, size_t>
  removeFrame(folly::IOBufQueue* q) = 0;

  /**
   * Wrap and IOBuf in any headers/footers
   */
  virtual std::unique_ptr<folly::IOBuf>
  addFrame(std::unique_ptr<folly::IOBuf> buf) = 0;
};

class Cpp2Channel
  : public MessageChannel
  , protected apache::thrift::async::TEventBase::LoopCallback
  , protected apache::thrift::async::TAsyncTransport::ReadCallback
  , protected apache::thrift::async::TAsyncTransport::WriteCallback {
 protected:
  virtual ~Cpp2Channel() {}

 public:

  explicit Cpp2Channel(
    const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport,
    std::unique_ptr<FramingChannelHandler> framingHandler);

  static std::unique_ptr<Cpp2Channel,
                         apache::thrift::async::TDelayedDestruction::Destructor>
  newChannel(
      const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport,
      std::unique_ptr<FramingChannelHandler> framingHandler) {
    return std::unique_ptr<Cpp2Channel,
      apache::thrift::async::TDelayedDestruction::Destructor>(
      new Cpp2Channel(transport, std::move(framingHandler)));
  }

  void closeNow();

  void setTransport(
      const std::shared_ptr<async::TAsyncTransport>& transport) {
    transport_ = transport;
  }
  async::TAsyncTransport* getTransport() {
    return transport_.get();
  }

  // TDelayedDestruction methods
  void destroy();

  // callbacks from apache::thrift::async::TAsyncTransport
  void getReadBuffer(void** bufReturn, size_t* lenReturn);
  void readDataAvailable(size_t len) noexcept;
  void readEOF() noexcept;
  void readError(const apache::thrift::transport::TTransportException& ex)
    noexcept;

  void writeSuccess() noexcept;
  void writeError(size_t bytesWritten,
                  const apache::thrift::transport::TTransportException& ex)
    noexcept;

  void processReadEOF() noexcept;

  // Interface from MessageChannel
  void sendMessage(SendCallback* callback,
                   std::unique_ptr<folly::IOBuf>&& buf);
  void setReceiveCallback(RecvCallback* callback);

  // event base methods
  virtual void attachEventBase(apache::thrift::async::TEventBase*);
  virtual void detachEventBase();
  apache::thrift::async::TEventBase* getEventBase();

  // callback from TEventBase::LoopCallback.  Used for sends
  virtual void runLoopCallback() noexcept;

  // Setter for queued sends mode.
  // Can only be set in quiescent state, otherwise
  // sendCallbacks_ would be called incorrectly.
  void setQueueSends(bool queueSends) {
    CHECK(sends_ == nullptr);
    queueSends_ = queueSends;
  }

  ProtectionChannelHandler* getProtectionHandler() const {
    return protectionHandler_.get();
  }

  FramingChannelHandler* getChannelHandler() const {
    return framingHandler_.get();
  }

  void setReadBufferSize(uint32_t readBufferSize) {
    CHECK(remaining_ == readBufferSize_); // channel has not been used
    remaining_ = readBufferSize_ = std::max(readBufferSize,
                                            DEFAULT_BUFFER_SIZE);
  }

private:
  std::shared_ptr<apache::thrift::async::TAsyncTransport> transport_;
  std::unique_ptr<folly::IOBufQueue> queue_;
  std::deque<std::vector<SendCallback*>> sendCallbacks_;

  static const uint32_t DEFAULT_BUFFER_SIZE = 2048;
  uint32_t readBufferSize_;
  uint32_t remaining_; // Used to attempt to allocate 'perfect' sized IOBufs

  RecvCallback* recvCallback_;
  bool closing_;
  bool eofInvoked_;

  std::unique_ptr<folly::IOBuf> sends_; // buffer of data to send.

  std::unique_ptr<RecvCallback::sample> sample_;

  // Queued sends feature - optimizes by minimizing syscalls in high-QPS
  // loads for greater throughput, but at the expense of some
  // minor latency increase.
  bool queueSends_;

  std::unique_ptr<ProtectionChannelHandler> protectionHandler_;
  std::unique_ptr<FramingChannelHandler> framingHandler_;
};

}} // apache::thrift

#endif // THRIFT_ASYNC_CPP2CHANNEL_H_
