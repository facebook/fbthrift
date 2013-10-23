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
#ifndef THRIFT_ASYNC_THEADERASYNCCHANNEL_H_
#define THRIFT_ASYNC_THEADERASYNCCHANNEL_H_ 1

#include "thrift/lib/cpp/async/TUnframedAsyncChannel.h"

namespace apache { namespace thrift { namespace async {

namespace detail {

/**
 * A class to determine the end of a THeaderProtocol message.
 */
class THeaderACProtocolTraits {
 public:

  THeaderACProtocolTraits()
      : maxMessageSize_(0x7ffffff)
      , strictRead_(true) {}

  // Methods required by TUnframedACReadState
  bool getMessageLength(uint8_t* buffer,
                        uint32_t bufferLength,
                        uint32_t* messageLength);

  void setMaxMessageSize(uint32_t maxMessageSize) {
    maxMessageSize_ = maxMessageSize;
  }

  void setStrictRead(bool strictRead) {
    strictRead_ = strictRead;
  }
  bool getStrictRead() const {
    return strictRead_;
  }

 private:
  uint32_t maxMessageSize_;
  bool strictRead_;
};

} // namespace detail

/**
 * THeaderAsyncChannel
 *
 * This is a TAsyncChannel implementation that reads and writes
 * messages encoded using THeaderProtocol.
 */
class THeaderAsyncChannel :
  public TUnframedAsyncChannel<detail::THeaderACProtocolTraits> {
 private:
  typedef TUnframedAsyncChannel<detail::THeaderACProtocolTraits> Parent;

 public:
  explicit THeaderAsyncChannel(
    const std::shared_ptr<TAsyncTransport>& transport)
    : Parent(transport) {}

  /**
   * Helper function to create a shared_ptr<THeaderAsyncChannel>.
   *
   * This passes in the correct destructor object, since THeaderAsyncChannel's
   * destructor is protected and cannot be invoked directly.
   */
  static std::shared_ptr<THeaderAsyncChannel> newChannel(
      const std::shared_ptr<TAsyncTransport>& transport) {
    return std::shared_ptr<THeaderAsyncChannel>(
        new THeaderAsyncChannel(transport), Destructor());
  }

  void setMaxMessageSize(uint32_t size) {
    Parent::setMaxMessageSize(size);
    readState_.getProtocolTraits()->setMaxMessageSize(size);
  }

  void setStrictRead(bool strictRead) {
    readState_.getProtocolTraits()->setStrictRead(strictRead);
  }
  bool getStrictRead() const {
    return readState_.getProtocolTraits()->getStrictRead();
  }

  // Note that we inherit getMaxMessageSize() from TUnframedAsyncChannel.

 protected:
  /**
   * Protected destructor.
   *
   * Users of THeaderAsyncChannel must never delete it directly.  Instead,
   * invoke destroy().
   */
  virtual ~THeaderAsyncChannel() { }
};

class THeaderAsyncChannelFactory : public TStreamAsyncChannelFactory {
 public:
  THeaderAsyncChannelFactory()
    : maxMessageSize_(0x7fffffff)
    , recvTimeout_(0)
    , sendTimeout_(0)
    , strictRead_(true) {}

  void setMaxMessageSize(uint32_t bytes) {
    maxMessageSize_ = bytes;
  }

  void setRecvTimeout(uint32_t milliseconds) {
    recvTimeout_ = milliseconds;
  }

  void setSendTimeout(uint32_t milliseconds) {
    sendTimeout_ = milliseconds;
  }

  void setStrictRead(bool strict) {
    strictRead_ = strict;
  }

  virtual std::shared_ptr<TAsyncEventChannel> newChannel(
      const std::shared_ptr<TAsyncTransport>& transport) {
    std::shared_ptr<THeaderAsyncChannel> channel(
        THeaderAsyncChannel::newChannel(transport));
    transport->setSendTimeout(sendTimeout_);
    channel->setMaxMessageSize(maxMessageSize_);
    channel->setRecvTimeout(recvTimeout_);
    channel->setStrictRead(strictRead_);
    return channel;
  }

 private:
  uint32_t maxMessageSize_;
  uint32_t recvTimeout_;
  uint32_t sendTimeout_;
  bool strictRead_;
};

}}} // apache::thrift::async

#endif // THRIFT_ASYNC_THEADERASYNCCHANNEL_H_
