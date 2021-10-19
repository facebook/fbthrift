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

#pragma once

#include <fizz/server/AsyncFizzServer.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/net/NetworkSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp2/security/SSLUtil.h>
#include <thrift/lib/cpp2/security/extensions/ThriftParametersContext.h>
#include <thrift/lib/cpp2/security/extensions/ThriftParametersServerExtension.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <wangle/acceptor/FizzAcceptorHandshakeHelper.h>
#include <wangle/acceptor/SSLAcceptorHandshakeHelper.h>

namespace apache {
namespace thrift {

class ThriftFizzAcceptorHandshakeHelper
    : public wangle::FizzAcceptorHandshakeHelper,
      public fizz::AsyncFizzBase::EndOfTLSCallback {
 public:
  ThriftFizzAcceptorHandshakeHelper(
      std::shared_ptr<const fizz::server::FizzServerContext> context,
      const folly::SocketAddress& clientAddr,
      std::chrono::steady_clock::time_point acceptTime,
      wangle::TransportInfo& tinfo,
      wangle::FizzLoggingCallback* loggingCallback,
      const std::shared_ptr<fizz::extensions::TokenBindingContext>&
          tokenBindingContext,
      const std::shared_ptr<apache::thrift::ThriftParametersContext>&
          thriftParametersContext)
      : wangle::FizzAcceptorHandshakeHelper::FizzAcceptorHandshakeHelper(
            context,
            clientAddr,
            acceptTime,
            tinfo,
            loggingCallback,
            tokenBindingContext),
        thriftParametersContext_(thriftParametersContext) {}

  void start(
      folly::AsyncSSLSocket::UniquePtr sock,
      wangle::AcceptorHandshakeHelper::Callback* callback) noexcept override {
    callback_ = callback;
    sslContext_ = sock->getSSLContext();

    if (thriftParametersContext_) {
      thriftExtension_ =
          std::make_shared<apache::thrift::ThriftParametersServerExtension>(
              thriftParametersContext_);
    }
    transport_ = createFizzServer(std::move(sock), context_, thriftExtension_);
    transport_->accept(this);
  }

 protected:
  // AsyncFizzServer::HandshakeCallback API
  void fizzHandshakeSuccess(
      fizz::server::AsyncFizzServer* transport) noexcept override {
    VLOG(3) << "Fizz handshake success";

    tinfo_.acceptTime = acceptTime_;
    tinfo_.secure = true;
    tinfo_.sslVersion = 0x0304;
    tinfo_.securityType = transport->getSecurityProtocol();
    tinfo_.sslSetupTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - acceptTime_);

    auto* handshakeLogging = transport->getState().handshakeLogging();
    if (handshakeLogging && handshakeLogging->clientSni) {
      tinfo_.sslServerName =
          std::make_shared<std::string>(*handshakeLogging->clientSni);
    }

    auto appProto = transport->getApplicationProtocol();

    if (loggingCallback_) {
      loggingCallback_->logFizzHandshakeSuccess(*transport, tinfo_);
    }

    if (thriftExtension_ && thriftExtension_->getNegotiatedStopTLS()) {
      transport->setEndOfTLSCallback(this);
      transport->tlsShutdown();
    } else {
      callback_->connectionReady(
          std::move(transport_),
          std::move(appProto),
          SecureTransportType::TLS,
          wangle::SSLErrorEnum::NO_ERROR);
    }
  }

  void endOfTLS(
      fizz::AsyncFizzBase* transport,
      std::unique_ptr<folly::IOBuf> endOfData) override {
    auto appProto = transport->getApplicationProtocol();
    auto plaintextTransport = moveToPlaintext(transport);
    // The server initiates the close, which means the client will be the first
    // to successfully terminate tls and return the socket back to the caller.
    // What this means for us is we clearly don't know if our fizz transport
    // will only read the close notify and not additionally read any data the
    // application decided to send when it got back the socket. Fizz already
    // exposes any post close notify data and we shove it back into the socket
    // here.
    plaintextTransport->setPreReceivedData(std::move(endOfData));
    plaintextTransport->cacheAddresses();
    // kill the fizz socket unique ptr
    transport_.reset();
    callback_->connectionReady(
        std::move(plaintextTransport),
        std::move(appProto),
        SecureTransportType::TLS,
        wangle::SSLErrorEnum::NO_ERROR);
  }

  std::shared_ptr<apache::thrift::ThriftParametersContext>
      thriftParametersContext_;
  std::shared_ptr<apache::thrift::ThriftParametersServerExtension>
      thriftExtension_;
};

class FizzPeeker : public wangle::DefaultToFizzPeekingCallback {
 public:
  using DefaultToFizzPeekingCallback::DefaultToFizzPeekingCallback;

  wangle::AcceptorHandshakeHelper::UniquePtr getHelper(
      const std::vector<uint8_t>& /* bytes */,
      const folly::SocketAddress& clientAddr,
      std::chrono::steady_clock::time_point acceptTime,
      wangle::TransportInfo& tinfo) override {
    if (!context_) {
      return nullptr;
    }
    return wangle::AcceptorHandshakeHelper::UniquePtr(
        new ThriftFizzAcceptorHandshakeHelper(
            context_,
            clientAddr,
            acceptTime,
            tinfo,
            loggingCallback_,
            tokenBindingContext_,
            thriftParametersContext_));
  }

  void setThriftParametersContext(
      std::shared_ptr<apache::thrift::ThriftParametersContext> context) {
    thriftParametersContext_ = std::move(context);
  }

  std::shared_ptr<apache::thrift::ThriftParametersContext>
      thriftParametersContext_;
};
} // namespace thrift
} // namespace apache
