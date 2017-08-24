/*
 * Copyright 2017-present Facebook, Inc.
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

#pragma once

#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>

#include <proxygen/lib/http/codec/HTTPCodec.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include <chrono>
#include <string>

namespace apache {
namespace thrift {

/**
 * HTTP/2 implementation of ClientConnectionIf.
 */
class H2ClientConnection : public ClientConnectionIf {
 public:
  static std::unique_ptr<ClientConnectionIf> newHTTP1xConnection(
      async::TAsyncTransport::UniquePtr transport,
      const std::string& httpHost,
      const std::string& httpUrl);

  static std::unique_ptr<ClientConnectionIf> newHTTP2Connection(
      async::TAsyncTransport::UniquePtr transport);

  virtual ~H2ClientConnection() override;

  H2ClientConnection(const H2ClientConnection&) = delete;
  H2ClientConnection& operator=(const H2ClientConnection&) = delete;

  std::shared_ptr<ThriftChannelIf> getChannel() override;
  void setMaxPendingRequests(uint32_t num) override;
  folly::EventBase* getEventBase() const override;

  apache::thrift::async::TAsyncTransport* getTransport() override;
  bool good() override;
  ClientChannel::SaturationStatus getSaturationStatus() override;
  void attachEventBase(folly::EventBase* evb) override;
  void detachEventBase() override;
  bool isDetachable() override;
  bool isSecurityActive() override;
  uint32_t getTimeout() override;
  void setTimeout(uint32_t ms) override;
  void closeNow() override;
  CLIENT_TYPE getClientType() override;

  // TODO: HTTPClientChannel subclasses InfoCallback::onDestroy and
  // its implementation closes itself after calling the CloseCallback.
  // With H2ClientConnection, we could have multiple ThriftClient's
  // active and therefore, we need to perform the equivalent actions
  // from H2TransactionCallback.

 private:
  static const std::chrono::milliseconds kDefaultTransactionTimeout;

  H2ClientConnection(
      async::TAsyncTransport::UniquePtr transport,
      std::unique_ptr<proxygen::HTTPCodec> codec);

  proxygen::HTTPUpstreamSession* httpSession_;
  folly::EventBase* evb_{nullptr};
  std::string httpHost_;
  std::string httpUrl_;
  std::chrono::milliseconds timeout_{kDefaultTransactionTimeout};
};

} // namespace thrift
} // namespace apache
