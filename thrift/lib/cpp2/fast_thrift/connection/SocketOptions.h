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

#include <chrono>
#include <cstdint>
#include <optional>

#include <folly/io/SocketOptionMap.h>
#include <folly/observer/Observer.h>

namespace apache::thrift::fast_thrift::connection {

/**
 * Listening-socket and accept-path tuning knobs. Pure data — set on the
 * embedder via the server's setSocketOptions and forwarded through
 * ConnectionManager to every IO thread's ConnectionHandler.
 */
struct SocketOptions {
  // Options added one at a time through FastThriftServer and applied to every
  // listening socket at the phase declared by each key.
  folly::SocketOptionMap listeningSocketOptions;

  // Accept queue depth (kernel listen backlog).
  uint32_t listenBacklog{1024};

  // TCP Fast Open. When enabled, the kernel may carry payload bytes in
  // the SYN, saving 1 RTT on connection setup. tfoQueueSize bounds the
  // kernel-side TFO pending-cookie queue.
  bool tfoEnabled{false};
  uint32_t tfoQueueSize{0};

  // Per-connection AsyncSocket::setMaxReadsPerEvent — caps how many
  // read() syscalls one socket does per EVB callback before yielding.
  // Matches legacy ThriftServer's socketMaxReadsPerEvent_ default.
  uint32_t maxReadsPerEvent{16};

  // Options applied to each accepted socket before it enters the connection
  // pipeline. A zero traffic class leaves IPV6_TCLASS unchanged.
  bool tcpNoDelay{false};
  int trafficClass{0};

  // Cap on pending connections per destination IO worker. Bounds the accepted
  // sockets queued for the worker (the same boundary AsyncServerSocket
  // provides to the classic Thrift server; keep the default in parity with
  // ThriftServerConfig) and the connections the worker is still processing
  // through its TLS pipeline: one slot across classification, handshake, and
  // StopTLS, released when established or dropped, with no effect under
  // SSLPolicy::DISABLED. Zero disables the TLS-pipeline cap.
  uint32_t maxPendingConnectionsPerWorker{4096};

  // Maximum time an accepted socket may wait for its destination IO worker.
  // Zero disables expiry.
  std::chrono::nanoseconds pendingConnectionQueueTimeout{
      std::chrono::milliseconds{100}};

  // Cap on the connections one IO thread will hold established at once. A
  // connection past the cap is closed once its handshake completes, rather
  // than being built — the same point the classic server enforces its own
  // limit. Unset, or an observed zero, disables it.
  //
  // Per IO thread, not process-wide: an embedder holding a global budget
  // divides it by the IO thread count before setting this.
  //
  // Loopback peers are exempt, so a local health probe still reaches a server
  // sitting at its limit. Observed rather than snapshotted because a
  // connection cap is worth changing on a running server.
  std::optional<folly::observer::Observer<uint32_t>> maxConnectionsPerIOThread;
};

} // namespace apache::thrift::fast_thrift::connection
