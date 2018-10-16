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
#include <wangle/acceptor/FizzAcceptorHandshakeHelper.h>

namespace apache {
namespace thrift {

class ThriftFizzAcceptorHandshakeHelper
    : public wangle::FizzAcceptorHandshakeHelper {
 public:
  using FizzAcceptorHandshakeHelper::FizzAcceptorHandshakeHelper;

 protected:
  fizz::server::AsyncFizzServer::UniquePtr createFizzServer(
      folly::AsyncSSLSocket::UniquePtr sslSock,
      const std::shared_ptr<fizz::server::FizzServerContext>& fizzContext,
      const std::shared_ptr<fizz::ServerExtensions>& /*extensions*/) override {
    folly::AsyncSocket::UniquePtr asyncSock(
      new apache::thrift::async::TAsyncSocket(std::move(sslSock)));
    asyncSock->cacheAddresses();
    return fizz::server::AsyncFizzServer::UniquePtr(
        new apache::thrift::async::TAsyncFizzServer(
            std::move(asyncSock), fizzContext, extension_));
  }

  folly::AsyncSSLSocket::UniquePtr createSSLSocket(
      const std::shared_ptr<folly::SSLContext>& sslContext,
      folly::EventBase* evb,
      int fd) override {
    return folly::AsyncSSLSocket::UniquePtr(
        new apache::thrift::async::TAsyncSSLSocket(
          sslContext,
          evb,
          fd));
  }
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
        new ThriftFizzAcceptorHandshakeHelper(context_,
                                              clientAddr,
                                              acceptTime,
                                              tinfo,
                                              loggingCallback_,
                                              tokenBindingContext_));
  }
};
} // namespace thrift
} // namespace apache
