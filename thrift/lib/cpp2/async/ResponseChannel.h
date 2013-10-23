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

#ifndef THRIFT_ASYNC_RESPONSECHANNEL_H_
#define THRIFT_ASYNC_RESPONSECHANNEL_H_ 1

#include <memory>
#include <limits>
#include <chrono>
#include "thrift/lib/cpp2/async/MessageChannel.h"
#include "thrift/lib/cpp2/async/Stream.h"
#include "thrift/lib/cpp/server/TServerObserver.h"
#include "thrift/lib/cpp/Thrift.h"

namespace folly {
class IOBuf;
}

const std::string kOverloadedErrorCode = "1";
const std::string kTaskExpiredErrorCode = "2";

namespace apache { namespace thrift {

/**
 * ResponseChannel defines an asynchronous API for servers.
 */
class ResponseChannel : virtual public TDelayedDestruction {
 public:

  static const uint32_t ONEWAY_REQUEST_ID =
    std::numeric_limits<uint32_t>::max();

  class Request {
   public:
    folly::IOBuf* getBuf() { return buf_.get(); }
    std::unique_ptr<folly::IOBuf> extractBuf() { return std::move(buf_); }

    virtual bool isActive() = 0;

    virtual void cancel() = 0;

    virtual bool isOneway() = 0;

    virtual void sendReply(std::unique_ptr<folly::IOBuf>&&,
                           MessageChannel::SendCallback* cb = nullptr) = 0;

    virtual void sendError(std::exception_ptr ex,
                           std::string exCode,
                           MessageChannel::SendCallback* cb = nullptr) = 0;

    virtual void sendReplyWithStreams(
        std::unique_ptr<folly::IOBuf>&&,
        std::unique_ptr<StreamManager>&&,
        MessageChannel::SendCallback* cb = nullptr) = 0;

    virtual void setStreamTimeout(const std::chrono::milliseconds& timeout) = 0;

    virtual ~Request() {}

    apache::thrift::server::TServerObserver::CallTimestamps timestamps_;
   protected:
    std::unique_ptr<folly::IOBuf> buf_;
  };

  class Callback {
   public:
    virtual void requestReceived(std::unique_ptr<Request>&&) = 0;

    /**
     * reason is empty if closed due to EOF, or a pointer to an exception
     * if closed due to some sort of error.
     */
    virtual void channelClosed(std::exception_ptr&&) = 0;

    virtual ~Callback() {}
  };

  /**
   * The callback will be invoked on each new request.
   * It will remain installed until explicitly uninstalled, or until
   * channelClosed() is called.
   */
  virtual void setCallback(Callback*) = 0;

 protected:
  virtual ~ResponseChannel() {}
};

}} // apache::thrift

#endif // #ifndef THRIFT_ASYNC_RESPONSECHANNEL_H_
