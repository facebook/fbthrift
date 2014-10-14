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

#pragma once

#include <folly/io/async/AsyncSSLServerSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>

namespace apache { namespace thrift { namespace async {

class TAsyncSSLServerSocket : public folly::AsyncSSLServerSocket {
 public:
  explicit TAsyncSSLServerSocket(
    const std::shared_ptr<folly::SSLContext>& ctx,
    folly::EventBase* eventBase = nullptr) :
      AsyncSSLServerSocket(ctx, eventBase) {}

void connectionAccepted(
  int fd,
  const folly::SocketAddress& clientAddr) noexcept override {
  std::shared_ptr<TAsyncSSLSocket> sslSock;
  try {
    // Create a AsyncSSLSocket object with the fd. The socket should be
    // added to the event base and in the state of accepting SSL connection.
    sslSock = TAsyncSSLSocket::newSocket(ctx_, eventBase_, fd);
  } catch (const std::exception &e) {
    LOG(ERROR) << "Exception %s caught while creating a AsyncSSLSocket "
      "object with socket " << e.what() << fd;
    ::close(fd);
    sslCallback_->acceptError(e);
    return;
  }

  // TODO: Perform the SSL handshake before invoking the callback
  sslCallback_->connectionAccepted(sslSock);
}};

}}}
