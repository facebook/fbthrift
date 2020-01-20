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

#ifndef THRIFT_ASYNC_MESSAGECHANNEL_H_
#define THRIFT_ASYNC_MESSAGECHANNEL_H_ 1

#include <memory>

#include <folly/ExceptionWrapper.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>
#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>

namespace folly {
class IOBuf;
}

using SamplingStatus = apache::thrift::server::TServerObserver::SamplingStatus;

namespace apache {
namespace thrift {

/**
 * MessageChannel defines an asynchronous API for message-based I/O.
 */
class MessageChannel : virtual public folly::DelayedDestruction {
 protected:
  ~MessageChannel() override {}

 public:
  class SendCallback {
   public:
    virtual ~SendCallback() {}
    virtual void sendQueued() = 0;
    virtual void messageSent() = 0;
    virtual void messageSendError(folly::exception_wrapper&&) = 0;
  };

  class RecvCallback {
   public:
    struct sample {
     public:
      uint64_t readBegin;
      uint64_t readEnd;

      sample(SamplingStatus status) : status_(status) {}
      SamplingStatus getStatus() const {
        return status_;
      }

     private:
      SamplingStatus status_;
    };

    virtual ~RecvCallback() {}
    virtual SamplingStatus shouldSample(
        const apache::thrift::transport::THeader* /*header*/) const {
      return SamplingStatus();
    }
    virtual void messageReceived(
        std::unique_ptr<folly::IOBuf>&&,
        std::unique_ptr<apache::thrift::transport::THeader>&&,
        std::unique_ptr<sample>) = 0;
    virtual void messageChannelEOF() = 0;
    virtual void messageReceiveErrorWrapped(folly::exception_wrapper&&) = 0;
  };

  virtual void sendMessage(
      SendCallback*,
      std::unique_ptr<folly::IOBuf>&&,
      apache::thrift::transport::THeader*) = 0;

  /**
   * RecvCallback will be invoked whenever a message is received.
   * It will remain installed until it is explicitly uninstalled with
   * setReceiveCallback(NULL), or until msgChannelEOF() or msgReceiveError()
   * is called.
   */
  virtual void setReceiveCallback(RecvCallback*) = 0;

  folly::AsyncTransportWrapper* getTransport() {
    return nullptr;
  }
};

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_ASYNC_MESSAGECHANNEL_H_
