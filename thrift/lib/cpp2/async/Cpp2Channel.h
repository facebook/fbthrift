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

#include "thrift/lib/cpp2/async/SaslEndpoint.h"
#include "thrift/lib/cpp2/async/MessageChannel.h"
#include "thrift/lib/cpp/async/TDelayedDestruction.h"
#include "thrift/lib/cpp/async/TAsyncTransport.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "folly/io/IOBufQueue.h"
#include <memory>

#include <deque>
#include <vector>

namespace apache { namespace thrift {

class Cpp2Channel
  : public MessageChannel
  , protected apache::thrift::async::TEventBase::LoopCallback
  , protected apache::thrift::async::TAsyncTransport::ReadCallback
  , protected apache::thrift::async::TAsyncTransport::WriteCallback {
 protected:
  virtual ~Cpp2Channel() {}

 public:

  explicit Cpp2Channel(
    const std::shared_ptr<apache::thrift::async::TAsyncTransport>& transport);

  static std::unique_ptr<Cpp2Channel,
                         apache::thrift::async::TDelayedDestruction::Destructor>
  newChannel(
      const std::shared_ptr<apache::thrift::async::TAsyncTransport>&
      transport) {
    return std::unique_ptr<Cpp2Channel,
      apache::thrift::async::TDelayedDestruction::Destructor>(
      new Cpp2Channel(transport));
  }

  void closeNow();

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

  // Message framing methods.  Defaults to 4-byte
  // size, override for a different method.

  /**
   * Given an IOBuf of data, wrap it in any headers/footers
   */
  virtual std::unique_ptr<folly::IOBuf>
    frameMessage(std::unique_ptr<folly::IOBuf>&&);

  /**
   * Given an IOBuf Chain, remove the framing.
   *
   * @param IOBufQueue - queue to try to read message from.
   *
   * @param needed - if the return is NULL (i.e. we didn't read a full
   *                 message), needed is set to the number of bytes needed
   *                 before you should call removeFrame again.
   *
   * @return IOBuf - the message chain.  May be shared, may be chained.
   *                 If NULL, we didn't get enough data for a whole message,
   *                 call removeFrame again after reading needed more bytes.
   */
  virtual std::unique_ptr<folly::IOBuf>
    removeFrame(folly::IOBufQueue*, size_t& remaining_);

  // callback from TEventBase::LoopCallback.  Used for sends
  virtual void runLoopCallback() noexcept;

  // Setter for queued sends mode.
  // Can only be set in quiescent state, otherwise
  // sendCallbacks_ would be called incorrectly.
  void setQueueSends(bool queueSends) {
    CHECK(sends_ == nullptr);
    queueSends_ = queueSends;
  }

protected:
  void setSaslEndpoint(SaslEndpoint* saslEndpoint) {
    saslEndpoint_ = saslEndpoint;
  }

  std::shared_ptr<apache::thrift::async::TAsyncTransport> transport_;

  enum class ProtectionState {
    UNKNOWN,
    NONE,
    INPROGRESS,
    VALID,
    INVALID,
  } protectionState_;

private:
  std::unique_ptr<folly::IOBufQueue> queue_;
  std::deque<std::vector<SendCallback*>> sendCallbacks_;
  uint32_t remaining_; // Used to attempt to allocate 'perfect' sized IOBufs
  RecvCallback* recvCallback_;
  // This is a pointer to a unique_ptr<> in the derived class, so the
  // object's lifetime, will exceed our own.
  SaslEndpoint* saslEndpoint_;
  std::unique_ptr<folly::IOBufQueue> pendingPlaintext_;
  bool closing_;
  bool eofInvoked_;

  std::unique_ptr<folly::IOBuf> sends_; // buffer of data to send.

  std::unique_ptr<RecvCallback::sample> sample_;

  static const uint32_t DEFAULT_BUFFER_SIZE = 2048;

  // Queued sends feature - optimizes by minimizing syscalls in high-QPS
  // loads for greater throughput, but at the expense of some
  // minor latency increase.
  bool queueSends_;
};

}} // apache::thrift

#endif // THRIFT_ASYNC_CPP2CHANNEL_H_
