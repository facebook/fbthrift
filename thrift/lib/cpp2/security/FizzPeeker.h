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
#include <thrift/lib/cpp2/security/extensions/ThriftParametersContext.h>
#include <thrift/lib/cpp2/security/extensions/ThriftParametersServerExtension.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <wangle/acceptor/FizzAcceptorHandshakeHelper.h>
#include <wangle/acceptor/SSLAcceptorHandshakeHelper.h>

namespace apache {
namespace thrift {

/**
 * A struct containing the TLS handshake negotiated parameters.
 */
struct NegotiatedParams {
  CompressionAlgorithm compression{CompressionAlgorithm::NONE};
};

class ThriftFizzAcceptorHandshakeHelper
    : public wangle::FizzAcceptorHandshakeHelper {
 public:
  ThriftFizzAcceptorHandshakeHelper(
      std::shared_ptr<const fizz::server::FizzServerContext> context,
      const folly::SocketAddress& clientAddr,
      std::chrono::steady_clock::time_point acceptTime,
      wangle::TransportInfo& tinfo,
      LoggingCallback* loggingCallback,
      const std::shared_ptr<fizz::extensions::TokenBindingContext>&
          tokenBindingContext,
      const std::shared_ptr<apache::thrift::ThriftParametersContext>&
          thriftParametersContext,
      NegotiatedParams* negotiatedParams)
      : wangle::FizzAcceptorHandshakeHelper::FizzAcceptorHandshakeHelper(
            context,
            clientAddr,
            acceptTime,
            tinfo,
            loggingCallback,
            tokenBindingContext),
        thriftParametersContext_(thriftParametersContext),
        negotiatedParams_(negotiatedParams) {}

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
  fizz::server::AsyncFizzServer::UniquePtr createFizzServer(
      folly::AsyncSSLSocket::UniquePtr sslSock,
      const std::shared_ptr<const fizz::server::FizzServerContext>& fizzContext,
      const std::shared_ptr<fizz::ServerExtensions>& extensions) override {
    folly::AsyncSocket::UniquePtr asyncSock(
        new folly::AsyncSocket(std::move(sslSock)));
    asyncSock->cacheAddresses();
    return fizz::server::AsyncFizzServer::UniquePtr(
        new fizz::server::AsyncFizzServer(
            std::move(asyncSock), fizzContext, extensions));
  }

  folly::AsyncSSLSocket::UniquePtr createSSLSocket(
      const std::shared_ptr<folly::SSLContext>& sslContext,
      folly::AsyncTransport::UniquePtr transport) override {
    auto socket = transport->getUnderlyingTransport<folly::AsyncSocket>();
    auto sslSocket = folly::AsyncSSLSocket::UniquePtr(
        new apache::thrift::async::TAsyncSSLSocket(
            sslContext, CHECK_NOTNULL(socket)));
    transport.reset();
    return sslSocket;
  }

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

    if (thriftExtension_ &&
        thriftExtension_->getThriftCompressionAlgorithm().has_value()) {
      setNegotiatedCompressionAlgorithm(
          *thriftExtension_->getThriftCompressionAlgorithm());
    }

    auto* handshakeLogging = transport->getState().handshakeLogging();
    if (handshakeLogging && handshakeLogging->clientSni) {
      tinfo_.sslServerName =
          std::make_shared<std::string>(*handshakeLogging->clientSni);
    }

    auto appProto = transport->getApplicationProtocol();

    if (loggingCallback_) {
      loggingCallback_->logFizzHandshakeSuccess(*transport, &tinfo_);
    }

    callback_->connectionReady(
        std::move(transport_),
        std::move(appProto),
        SecureTransportType::TLS,
        wangle::SSLErrorEnum::NO_ERROR);
  }

  // AsyncSSLSocket::HandshakeCallback API
  void handshakeSuc(folly::AsyncSSLSocket* sock) noexcept override {
    auto appProto = sock->getApplicationProtocol();
    if (!appProto.empty()) {
      VLOG(3) << "Client selected next protocol " << appProto;
    } else {
      VLOG(3) << "Client did not select a next protocol";
    }

    // fill in SSL-related fields from TransportInfo
    // the other fields like RTT are filled in the Acceptor
    tinfo_.acceptTime = acceptTime_;
    tinfo_.sslSetupTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - acceptTime_);

    if (thriftExtension_ &&
        thriftExtension_->getThriftCompressionAlgorithm().has_value()) {
      setNegotiatedCompressionAlgorithm(
          *thriftExtension_->getThriftCompressionAlgorithm());
    }
    wangle::SSLAcceptorHandshakeHelper::fillSSLTransportInfoFields(
        sock, tinfo_);

    // The callback will delete this.
    callback_->connectionReady(
        std::move(sslSocket_),
        std::move(appProto),
        SecureTransportType::TLS,
        wangle::SSLErrorEnum::NO_ERROR);
  }

  void setNegotiatedCompressionAlgorithm(CompressionAlgorithm algo) {
    if (algo != CompressionAlgorithm::NONE) {
      negotiatedParams_->compression = algo;
    }
  }

  std::shared_ptr<apache::thrift::ThriftParametersContext>
      thriftParametersContext_;
  std::shared_ptr<apache::thrift::ThriftParametersServerExtension>
      thriftExtension_;
  NegotiatedParams* negotiatedParams_;
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
            thriftParametersContext_,
            &negotiatedParams_));
  }

  void setThriftParametersContext(
      std::shared_ptr<apache::thrift::ThriftParametersContext> context) {
    thriftParametersContext_ = std::move(context);
  }

  NegotiatedParams& getNegotiatedParameters() {
    return negotiatedParams_;
  }

  std::shared_ptr<apache::thrift::ThriftParametersContext>
      thriftParametersContext_;

  /*
   * Stores the parameters that was negotiatied during the TLS handshake.
   */
  NegotiatedParams negotiatedParams_;
};
} // namespace thrift
} // namespace apache
