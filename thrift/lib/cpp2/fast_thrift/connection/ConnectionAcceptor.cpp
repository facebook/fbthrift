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

#include <thrift/lib/cpp2/fast_thrift/connection/ConnectionAcceptor.h>

#include <array>
#include <cerrno>
#include <utility>

#if defined(__linux__)
#include <linux/filter.h>
#include <sys/socket.h>
#endif

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <folly/net/NetworkSocket.h>

namespace apache::thrift::fast_thrift::connection {

ConnectionAcceptor::ConnectionAcceptor(
    folly::EventBase& evb,
    folly::SocketAddress address,
    SocketOptions socketOptions,
    bool enableReusePortBpfSpread,
    std::vector<ConnectionWorkerTarget> workers)
    : evb_(folly::getKeepAliveToken(&evb)),
      address_(std::move(address)),
      socketOptions_(std::move(socketOptions)),
      enableReusePortBpfSpread_(enableReusePortBpfSpread),
      workers_(std::move(workers)),
      socket_(new folly::AsyncServerSocket(evb_.get())),
      boundAddress_(address_) {
  DCHECK(evb_->isInEventBaseThread());
  CHECK(!workers_.empty()) << "ConnectionAcceptor requires a worker";
}

ConnectionAcceptor::~ConnectionAcceptor() {
  if (evb_->inRunningEventBaseThread()) {
    stopOnEvb();
  } else {
    stop();
  }
}

void ConnectionAcceptor::start() {
  DCHECK(evb_->isInEventBaseThread());
  CHECK(socket_) << "ConnectionAcceptor::start called after stop";
  DCHECK(!started_);

  socket_->setReusePortEnabled(true);
  socket_->setConnectionEventCallback(connectionEventCallback_.get());
  socket_->setMaxNumMessagesInQueue(
      socketOptions_.maxPendingConnectionsPerWorker);
  socket_->setQueueTimeout(socketOptions_.pendingConnectionQueueTimeout);
  if (socketOptions_.tfoEnabled) {
    socket_->setTFOEnabled(true, socketOptions_.tfoQueueSize);
  }
  socket_->bind(address_, socketOptions_.listeningSocketOptions);
  for (auto fd : socket_->getNetworkSockets()) {
    for (const auto& [key, value] : folly::validateSocketOptions(
             socketOptions_.listeningSocketOptions,
             address_.getFamily(),
             folly::SocketOptionKey::ApplyPos::POST_BIND)) {
      if (key.apply(fd, value) != 0) {
        const int savedErrno = errno;
        XLOGF_EVERY_MS(
            ERR,
            60000,
            "Failed to apply listening socket option on fd {}: errno={} ({})",
            fd.toFd(),
            savedErrno,
            folly::errnoStr(savedErrno));
      }
    }
  }
  socket_->listen(static_cast<int>(socketOptions_.listenBacklog));
  if (enableReusePortBpfSpread_) {
    attachReusePortBpfSpread();
  }
  for (const auto& target : workers_) {
    socket_->addAcceptCallback(
        &target.callback, &target.evb == evb_.get() ? nullptr : &target.evb);
  }
  socket_->startAccepting();
  socket_->getAddress(&boundAddress_);
  started_ = true;
}

void ConnectionAcceptor::stop() {
  if (!socket_) {
    return;
  }
  DCHECK(!evb_->inRunningEventBaseThread())
      << "ConnectionAcceptor::stop must not run on its EventBase";
  evb_->runImmediatelyOrRunInEventBaseThreadAndWait([this] { stopOnEvb(); });
}

void ConnectionAcceptor::stopOnEvb() {
  if (!socket_) {
    return;
  }
  if (started_) {
    socket_->pauseAccepting();
    for (const auto& target : workers_) {
      socket_->removeAcceptCallback(
          &target.callback, &target.evb == evb_.get() ? nullptr : &target.evb);
    }
    // RemoteAcceptor::stop and these barriers are submitted from this same
    // thread, so the ordering guarantee drains every queued fd before the
    // corresponding ConnectionHandler can be destroyed.
    for (const auto& target : workers_) {
      if (&target.evb != evb_.get()) {
        target.evb.runInEventBaseThreadAndWait([] {});
      }
    }
    started_ = false;
  }
  socket_.reset();
}

folly::SocketAddress ConnectionAcceptor::getAddress() const {
  return boundAddress_;
}

int64_t ConnectionAcceptor::getNumPendingConnections() const {
  CHECK(socket_);
  return socket_->getNumPendingMessagesInQueue();
}

size_t ConnectionAcceptor::getNumDroppedConnections() const {
  CHECK(socket_);
  return socket_->getNumDroppedConnections();
}

void ConnectionAcceptor::attachReusePortBpfSpread() noexcept {
  // Replace the kernel's default 4-tuple-hash REUSEPORT selection with a
  // 2-instruction cBPF program returning a random u32. A failed attach leaves
  // the kernel's default selector in place.
#if defined(__linux__)
  auto code = std::to_array<sock_filter>({
      BPF_STMT(
          BPF_LD | BPF_W | BPF_ABS,
          static_cast<uint32_t>(SKF_AD_OFF + SKF_AD_RANDOM)),
      BPF_STMT(BPF_RET | BPF_A, 0),
  });
  struct sock_fprog prog = {
      .len = static_cast<unsigned short>(code.size()),
      .filter = code.data(),
  };
  for (auto fd : socket_->getNetworkSockets()) {
    if (::setsockopt(
            fd.toFd(),
            SOL_SOCKET,
            SO_ATTACH_REUSEPORT_CBPF,
            &prog,
            sizeof(prog)) != 0) {
      const int savedErrno = errno;
      XLOGF_EVERY_MS(
          ERR,
          1000,
          "SO_ATTACH_REUSEPORT_CBPF failed on fd {}: errno={} ({})",
          fd.toFd(),
          savedErrno,
          folly::errnoStr(savedErrno));
    }
  }
#endif
}

} // namespace apache::thrift::fast_thrift::connection
