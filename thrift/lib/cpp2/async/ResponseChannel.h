/*
 * Copyright 2004-present Facebook, Inc.
 *
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

#ifndef THRIFT_ASYNC_RESPONSECHANNEL_H_
#define THRIFT_ASYNC_RESPONSECHANNEL_H_ 1

#include <chrono>
#include <limits>
#include <memory>

#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/SemiStream.h>

namespace folly {
class IOBuf;
}

extern const std::string kOverloadedErrorCode;
extern const std::string kTaskExpiredErrorCode;
extern const std::string kProxyTransportExceptionErrorCode;
extern const std::string kProxyClientProtocolExceptionErrorCode;
extern const std::string kQueueOverloadedErrorCode;
extern const std::string kProxyHeaderParseExceptionErrorCode;
extern const std::string kProxyAuthExceptionErrorCode;
extern const std::string kProxyLookupTransportExceptionErrorCode;
extern const std::string kProxyLookupAppExceptionErrorCode;
extern const std::string kProxyWhitelistExceptionErrorCode;
extern const std::string kProxyClientAppExceptionErrorCode;
extern const std::string kProxyProtocolMismatchExceptionErrorCode;
extern const std::string kProxyQPSThrottledExceptionErrorCode;
extern const std::string kProxyResponseSizeThrottledExceptionErrorCode;
extern const std::string kInjectedFailureErrorCode;
extern const std::string kServerQueueTimeoutErrorCode;
extern const std::string kResponseTooBigErrorCode;
extern const std::string kProxyAclCheckExceptionErrorCode;
extern const std::string kProxyOverloadedErrorCode;
extern const std::string kProxyLoopbackErrorCode;

namespace apache {
namespace thrift {

/**
 * ResponseChannel defines an asynchronous API for servers.
 */
class ResponseChannel : virtual public folly::DelayedDestruction {
 public:
  static const uint32_t ONEWAY_REQUEST_ID =
      std::numeric_limits<uint32_t>::max();

  class Request {
   public:
    folly::IOBuf* getBuf() {
      return buf_.get();
    }
    std::unique_ptr<folly::IOBuf> extractBuf() {
      return std::move(buf_);
    }

    SemiStream<std::unique_ptr<folly::IOBuf>> extractStream() {
      return std::move(stream_);
    }

    virtual bool isActive() = 0;

    virtual void cancel() = 0;

    virtual bool isOneway() = 0;

    virtual void sendReply(
        std::unique_ptr<folly::IOBuf>&&,
        MessageChannel::SendCallback* cb = nullptr) = 0;

    virtual void sendStreamReply(
        ResponseAndSemiStream<
            std::unique_ptr<folly::IOBuf>,
            std::unique_ptr<folly::IOBuf>>&&,
        MessageChannel::SendCallback* = nullptr) {
      throw std::logic_error("unimplemented");
    }

    virtual void sendErrorWrapped(
        folly::exception_wrapper ex,
        std::string exCode,
        MessageChannel::SendCallback* cb = nullptr) = 0;

    virtual ~Request() {}

    virtual apache::thrift::server::TServerObserver::CallTimestamps&
    getTimestamps() {
      return timestamps_;
    }

    apache::thrift::server::TServerObserver::CallTimestamps timestamps_;

   protected:
    std::unique_ptr<folly::IOBuf> buf_;
    SemiStream<std::unique_ptr<folly::IOBuf>> stream_;
  };

  class Callback {
   public:
    virtual void requestReceived(std::unique_ptr<Request>&&) = 0;

    /**
     * reason is empty if closed due to EOF, or a pointer to an exception
     * if closed due to some sort of error.
     */
    virtual void channelClosed(folly::exception_wrapper&&) = 0;

    virtual ~Callback() {}
  };

  /**
   * The callback will be invoked on each new request.
   * It will remain installed until explicitly uninstalled, or until
   * channelClosed() is called.
   */
  virtual void setCallback(Callback*) = 0;

 protected:
  ~ResponseChannel() override {}
};

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_ASYNC_RESPONSECHANNEL_H_
