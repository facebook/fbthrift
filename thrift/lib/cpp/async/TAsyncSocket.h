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

#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>
#include <folly/net/NetworkSocket.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

namespace apache {
namespace thrift {
namespace async {

// Wrapper around folly's AsyncSocket to maintain backwards compatibility:
// Converts exceptions to thrift's TTransportException type.
class TAsyncSocket : public virtual folly::AsyncSocket {
 public:
  typedef std::unique_ptr<TAsyncSocket, Destructor> UniquePtr;

  explicit TAsyncSocket(folly::EventBase* evb) : folly::AsyncSocket(evb) {}

  TAsyncSocket(
      folly::EventBase* evb,
      const folly::SocketAddress& address,
      uint32_t connectTimeout = 0,
      bool useZeroCopy = false)
      : folly::AsyncSocket(evb, address, connectTimeout, useZeroCopy) {}

  TAsyncSocket(folly::EventBase* evb, folly::NetworkSocket fd)
      : folly::AsyncSocket(evb, fd) {}

  TAsyncSocket(
      folly::EventBase* evb,
      const std::string& ip,
      uint16_t port,
      uint32_t connectTimeout = 0,
      bool useZeroCopy = false)
      : folly::AsyncSocket(evb, ip, port, connectTimeout, useZeroCopy) {}

  TAsyncSocket(folly::AsyncSocket::UniquePtr sock)
      : folly::AsyncSocket(std::move(sock)) {}

  static std::shared_ptr<TAsyncSocket> newSocket(folly::EventBase* evb) {
    return std::shared_ptr<TAsyncSocket>(new TAsyncSocket(evb), Destructor());
  }

  static std::shared_ptr<TAsyncSocket> newSocket(
      folly::EventBase* evb,
      const folly::SocketAddress& address,
      uint32_t connectTimeout = 0,
      bool useZeroCopy = false) {
    return std::shared_ptr<TAsyncSocket>(
        new TAsyncSocket(evb, address, connectTimeout, useZeroCopy),
        Destructor());
  }

  static std::shared_ptr<TAsyncSocket> newSocket(
      folly::EventBase* evb,
      const std::string& ip,
      uint16_t port,
      uint32_t connectTimeout = 0,
      bool useZeroCopy = false) {
    return std::shared_ptr<TAsyncSocket>(
        new TAsyncSocket(evb, ip, port, connectTimeout, useZeroCopy),
        Destructor());
  }

  static std::shared_ptr<TAsyncSocket> newSocket(
      folly::EventBase* evb,
      folly::NetworkSocket fd) {
    return std::shared_ptr<TAsyncSocket>(
        new TAsyncSocket(evb, fd), Destructor());
  }
};

} // namespace async
} // namespace thrift
} // namespace apache
