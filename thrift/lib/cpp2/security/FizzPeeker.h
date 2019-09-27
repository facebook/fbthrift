/*
 * Copyright 2014-present Facebook, Inc.
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

#include <thrift/lib/cpp/async/TAsyncFizzServer.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/security/extensions/ThriftParametersContext.h>
#include <thrift/lib/cpp2/security/extensions/ThriftParametersServerExtension.h>
#include <wangle/acceptor/FizzAcceptorHandshakeHelper.h>

namespace apache {
namespace thrift {

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
  fizz::server::AsyncFizzServer::UniquePtr createFizzServer(
      folly::AsyncSSLSocket::UniquePtr sslSock,
      const std::shared_ptr<const fizz::server::FizzServerContext>& fizzContext,
      const std::shared_ptr<fizz::ServerExtensions>& extensions) override {
    folly::AsyncSocket::UniquePtr asyncSock(
        new apache::thrift::async::TAsyncSocket(std::move(sslSock)));
    asyncSock->cacheAddresses();
    return fizz::server::AsyncFizzServer::UniquePtr(
        new apache::thrift::async::TAsyncFizzServer(
            std::move(asyncSock), fizzContext, extensions));
  }

  folly::AsyncSSLSocket::UniquePtr createSSLSocket(
      const std::shared_ptr<folly::SSLContext>& sslContext,
      folly::EventBase* evb,
      int fd) override {
    return folly::AsyncSSLSocket::UniquePtr(
        new apache::thrift::async::TAsyncSSLSocket(sslContext, evb, fd));
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
