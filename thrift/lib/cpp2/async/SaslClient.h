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

#ifndef THRIFT_SASLCLIENT_H_
#define THRIFT_SASLCLIENT_H_ 1

#include "thrift/lib/cpp2/async/SaslEndpoint.h"
#include "thrift/lib/cpp/async/HHWheelTimer.h"
#include "thrift/lib/cpp2/async/MessageChannel.h"
#include "thrift/lib/cpp2/async/RequestChannel.h"

namespace apache { namespace thrift {

class SaslClient : public SaslEndpoint {
public:
  class Callback : public apache::thrift::async::HHWheelTimer::Callback {
   public:
    virtual ~Callback() {}

    // Invoked when a new message should be sent to the server.
    virtual void saslSendServer(std::unique_ptr<folly::IOBuf>&&) = 0;

    // Invoked when the most recently consumed message results in an
    // error.  Continuation is impossible at this point.
    virtual void saslError(std::exception_ptr&&) = 0;

    // Invoked when the most recently consumed message completes the
    // SASL exchange successfully.
    virtual void saslComplete() = 0;

    void timeoutExpired() noexcept {
      using apache::thrift::transport::TTransportException;
      TTransportException ex(
          TTransportException::TIMED_OUT,
          "SASL handshake timed out");
      saslError(std::make_exception_ptr(ex));
    }

  };

  virtual void setClientIdentity(const std::string& identity) = 0;
  virtual void setServiceIdentity(const std::string& identity) = 0;

  // This will create the initial message, and pass it to
  // cb->saslSendServer().  If there is an error, cb->saslError() will
  // be invoked.
  virtual void start(Callback*) = 0;

  // This will consume the provided message.  If a message should be
  // sent in reply, it will be passed to cb->saslServerMessage().  If
  // the authentication completes successfully, cb->saslComplete()
  // will be invoked.  If there is an error, cb->saslError() will be
  // invoked.
  virtual void consumeFromServer(
    Callback*, std::unique_ptr<folly::IOBuf>&&) = 0;

  virtual const std::string* getErrorString() const = 0;

  virtual void setErrorString(const std::string& str) = 0;

};

}} // apache::thrift

#endif // THRIFT_SASLCLIENT_H_
