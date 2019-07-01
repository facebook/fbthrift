/*
 * Copyright 2018-present Facebook, Inc.
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

#include <folly/io/async/SSLContext.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/py3/client.h>

namespace thrift {
namespace py3 {

using apache::thrift::async::TAsyncSocket;
using apache::thrift::async::TAsyncSSLSocket;

class ConnectHandler
    : public apache::thrift::async::TAsyncSocket::ConnectCallback,
      public folly::DelayedDestruction {
 protected:
  ~ConnectHandler() = default;

 public:
  using UniquePtr =
      std::unique_ptr<ConnectHandler, folly::DelayedDestruction::Destructor>;
  explicit ConnectHandler(
      const std::shared_ptr<folly::SSLContext>& ctx,
      folly::EventBase* evb)
      : socket_{new TAsyncSSLSocket(ctx, evb)} {}

  folly::Future<HeaderChannel_ptr> connect(
      const std::string& ip,
      uint16_t port,
      uint32_t timeout = 0,
      uint32_t ssl_timeout = 0) {
    folly::DelayedDestruction::DestructorGuard dg(this);
    uint64_t total_timeout = timeout + ssl_timeout;

    socket_->connect(
        this,
        folly::SocketAddress(ip, port),
        std::chrono::milliseconds(timeout),
        std::chrono::milliseconds(timeout + ssl_timeout));
    return promise_.getFuture();
  }

  void connectSuccess() noexcept override {
    UniquePtr p(this);
    promise_.setValue(apache::thrift::HeaderClientChannel::newChannel(
        std::shared_ptr<TAsyncSocket>{std::move(socket_)}));
  }

  void connectError(const apache::thrift::transport::TTransportException&
                        ex) noexcept override {
    UniquePtr p(this);
    // TAsyncSocket::connectErr explicitly creates a TTransportException
    //  so runtime exception type is always same as static type.
    promise_.setException(ex);
  }

 private:
  folly::Promise<HeaderChannel_ptr> promise_;
  TAsyncSSLSocket::UniquePtr socket_;
};

/**
 * Create a thrift channel by connecting to a host:port over TCP then SSL.
 */
folly::Future<RequestChannel_ptr> createThriftChannelTCP(
    const std::shared_ptr<folly::SSLContext>& ctx,
    std::string&& host,
    const uint16_t port,
    const uint32_t connect_timeout,
    const uint32_t ssl_timeout,
    CLIENT_TYPE client_t,
    apache::thrift::protocol::PROTOCOL_TYPES proto,
    std::string&& endpoint) {
  auto eb = folly::getEventBase();
  return folly::via(
             eb,
             [=, host{std::move(host)}] {
               ConnectHandler::UniquePtr handler{new ConnectHandler(ctx, eb)};
               auto future =
                   handler->connect(host, port, connect_timeout, ssl_timeout);
               handler.release();
               return future;
             })
      .then(eb, [=, endpoint{std::move(endpoint)}](HeaderChannel_ptr&& chan_) {
        auto chan = configureClientChannel(std::move(chan_), client_t, proto);
        if (client_t == THRIFT_HTTP_CLIENT_TYPE) {
          chan->useAsHttpClient(host, endpoint);
        }
        return std::move(chan);
      });
}
} // namespace py3
} // namespace thrift
