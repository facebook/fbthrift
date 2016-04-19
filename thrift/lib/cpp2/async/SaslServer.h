/*
 * Copyright 2014 Facebook, Inc.
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

#ifndef THRIFT_SASLSERVER_H_
#define THRIFT_SASLSERVER_H_ 1

#include <thrift/lib/cpp2/async/SaslEndpoint.h>
#include <folly/io/async/HHWheelTimer.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

#include <folly/ExceptionWrapper.h>

namespace apache { namespace thrift {

class SaslServer : public SaslEndpoint {
 public:
  explicit SaslServer(folly::EventBase* evb = nullptr)
    : SaslEndpoint(evb) {}

  class Callback : public folly::HHWheelTimer::Callback {
   public:
    ~Callback() override {}

    // Invoked when a new message should be sent to the client.
    virtual void saslSendClient(std::unique_ptr<folly::IOBuf>&&) = 0;

    // Invoked when the most recently consumed message results in an
    // error.  Continuation is impossible at this point.  The
    // implementation will call saslSendClient() first with a message
    // for the client which indicates mechanism failure.
    virtual void saslError(folly::exception_wrapper&&) = 0;

    // Invoked after the most recently consumed message completes the
    // SASL exchange successfully.
    virtual void saslComplete() = 0;

    void timeoutExpired() noexcept override {
      using apache::thrift::transport::TTransportException;
      auto ex = folly::make_exception_wrapper<TTransportException>(
          TTransportException::TIMED_OUT,
          "SASL handshake timed out");
      saslError(std::move(ex));
    }
  };

  virtual void setServiceIdentity(const std::string& identity) = 0;

  // This will consume the provided message.  A reply will be passed
  // to cb->saslSendClient().  This is true even if there is an error;
  // the message will indicate to the peer that the mechanism failed.
  // If the authentication completes successfully, cb->saslComplete()
  // will also be invoked.  If there is an error, cb->saslError() will
  // be invoked.
  virtual void consumeFromClient(
    Callback *cb, std::unique_ptr<folly::IOBuf>&& message) = 0;
};

}} // apache::thrift

#endif // THRIFT_SASLSERVER_H_
