/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <folly/Executor.h>
#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/fast_thrift/connection/SocketOptions.h>

namespace apache::thrift::fast_thrift::connection {

struct ConnectionWorkerTarget {
  folly::EventBase& evb;
  folly::AsyncServerSocket::AcceptCallback& callback;
};

/**
 * Per-acceptor-EventBase listening-socket owner. AsyncServerSocket dispatches
 * accepted fds through bounded queues to data EventBases, where socket
 * construction, TLS, StopTLS, and the application connection pipeline run.
 */
class ConnectionAcceptor {
 public:
  using Ptr = std::unique_ptr<ConnectionAcceptor>;

  ConnectionAcceptor(
      folly::EventBase& evb,
      folly::SocketAddress address,
      SocketOptions socketOptions,
      bool enableReusePortBpfSpread,
      std::vector<ConnectionWorkerTarget> workers);

  ~ConnectionAcceptor();

  ConnectionAcceptor(const ConnectionAcceptor&) = delete;
  ConnectionAcceptor& operator=(const ConnectionAcceptor&) = delete;
  ConnectionAcceptor(ConnectionAcceptor&&) = delete;
  ConnectionAcceptor& operator=(ConnectionAcceptor&&) = delete;

  void start();
  void stop();
  folly::SocketAddress getAddress() const;

  int64_t getNumPendingConnections() const;
  size_t getNumDroppedConnections() const;

 private:
  void stopOnEvb();
  void attachReusePortBpfSpread() noexcept;

  folly::Executor::KeepAlive<folly::EventBase> evb_;
  folly::SocketAddress address_;
  SocketOptions socketOptions_;
  bool enableReusePortBpfSpread_;
  std::vector<ConnectionWorkerTarget> workers_;
  folly::AsyncServerSocket::UniquePtr socket_;
  folly::SocketAddress boundAddress_;
  bool started_{false};
};

} // namespace apache::thrift::fast_thrift::connection
