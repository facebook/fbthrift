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

#ifndef THRIFT_ASYNC_MESSAGECHANNEL_H_
#define THRIFT_ASYNC_MESSAGECHANNEL_H_ 1

#include <memory>
#include "thrift/lib/cpp/async/TDelayedDestruction.h"
#include "thrift/lib/cpp/Thrift.h"

namespace folly {
class IOBuf;
}

namespace apache { namespace thrift { namespace async {
class TAsyncTransport;
}}}

namespace apache { namespace thrift {

/**
 * MessageChannel defines an asynchronous API for message-based I/O.
 */
class MessageChannel :
      virtual public apache::thrift::async::TDelayedDestruction {
 protected:
  virtual ~MessageChannel() {}

 public:
  class SendCallback {
   public:
    virtual ~SendCallback() {}
    virtual void sendQueued() = 0;
    virtual void messageSent() = 0;
    virtual void messageSendError(std::exception_ptr&&) = 0;
  };

  class RecvCallback {
   public:
    struct sample {
      uint64_t readBegin;
      uint64_t readEnd;
    };

    virtual ~RecvCallback() {}
    virtual bool shouldSample() {
      return false;
    }
    virtual void messageReceived(std::unique_ptr<folly::IOBuf>&&,
                                 std::unique_ptr<sample>) = 0;
    virtual void messageChannelEOF() = 0;
    virtual void messageReceiveError(std::exception_ptr&&) = 0;
  };

  virtual void sendMessage(SendCallback*,
                           std::unique_ptr<folly::IOBuf>&&) = 0;

  /**
   * RecvCallback will be invoked whenever a message is received.
   * It will remain installed until it is explicitly uninstalled with
   * setReceiveCallback(NULL), or until msgChannelEOF() or msgReceiveError()
   * is called.
   */
  virtual void setReceiveCallback(RecvCallback*) = 0;

  apache::thrift::async::TAsyncTransport* getTransport() { return nullptr;}
};

}} // apache::thrift

#endif // #ifndef THRIFT_ASYNC_MESSAGECHANNEL_H_
