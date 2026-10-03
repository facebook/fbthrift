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

#include <gtest/gtest.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/net/NetworkSocket.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp2/fast_thrift/connection/ConnectionAcceptor.h>
#include <thrift/lib/cpp2/fast_thrift/connection/SocketOptions.h>

namespace apache::thrift::fast_thrift::connection {
namespace {

TEST(SocketOptionsTest, PendingConnectionLimitMatchesClassicThriftDefault) {
  EXPECT_EQ(SocketOptions{}.maxPendingConnectionsPerWorker, 4096);
}

class TestAcceptCallback final
    : public folly::AsyncServerSocket::AcceptCallback {
 public:
  explicit TestAcceptCallback(folly::EventBase& evb) : evb_(evb) {}

  void connectionAccepted(
      folly::NetworkSocket fd,
      const folly::SocketAddress&,
      AcceptInfo) noexcept override {
    acceptedOnWorker_.store(
        evb_.isInEventBaseThread(), std::memory_order_relaxed);
    folly::netops::close(fd);
    if (acceptedCount_.fetch_add(1, std::memory_order_relaxed) == 0) {
      accepted_.post();
    }
  }

  void acceptError(folly::exception_wrapper) noexcept override {
    acceptError_.store(true, std::memory_order_relaxed);
    accepted_.post();
  }

  void acceptStarted() noexcept override { started_.post(); }
  void acceptStopped() noexcept override { stopped_.post(); }

  folly::Baton<> started_;
  folly::Baton<> accepted_;
  folly::Baton<> stopped_;
  std::atomic<size_t> acceptedCount_{0};
  std::atomic<bool> acceptedOnWorker_{false};
  std::atomic<bool> acceptError_{false};

 private:
  folly::EventBase& evb_;
};

class TestConnectionEventCallback final
    : public folly::AsyncServerSocket::ConnectionEventCallback {
 public:
  void onConnectionAccepted(
      folly::NetworkSocket, const folly::SocketAddress&) noexcept override {
    accepted.fetch_add(1, std::memory_order_relaxed);
  }

  void onConnectionAcceptError(int) noexcept override {
    errors.fetch_add(1, std::memory_order_relaxed);
  }

  void onConnectionDropped(
      folly::NetworkSocket,
      const folly::SocketAddress&,
      const std::string&) noexcept override {
    dropped.fetch_add(1, std::memory_order_relaxed);
  }

  void onConnectionEnqueuedForAcceptorCallback(
      folly::NetworkSocket, const folly::SocketAddress&) noexcept override {
    enqueued.fetch_add(1, std::memory_order_relaxed);
  }

  void onConnectionDequeuedByAcceptorCallback(
      folly::NetworkSocket, const folly::SocketAddress&) noexcept override {
    dequeued.fetch_add(1, std::memory_order_relaxed);
  }

  void onBackoffStarted() noexcept override {}
  void onBackoffEnded() noexcept override {}
  void onBackoffError() noexcept override {}

  std::atomic<size_t> accepted{0};
  std::atomic<size_t> errors{0};
  std::atomic<size_t> dropped{0};
  std::atomic<size_t> enqueued{0};
  std::atomic<size_t> dequeued{0};
};

TEST(ConnectionAcceptorTest, DispatchesAcceptedFdToWorker) {
  folly::ScopedEventBaseThread acceptorThread;
  folly::ScopedEventBaseThread workerThread;
  auto* acceptorEvb = acceptorThread.getEventBase();
  auto* workerEvb = workerThread.getEventBase();
  TestAcceptCallback callback(*workerEvb);
  auto eventCallback = std::make_shared<TestConnectionEventCallback>();

  ConnectionAcceptor::Ptr acceptor;
  folly::SocketAddress address;
  acceptorEvb->runInEventBaseThreadAndWait([&] {
    std::vector<ConnectionWorkerTarget> workers;
    workers.reserve(1);
    workers.push_back({*workerEvb, callback});
    acceptor = std::make_unique<ConnectionAcceptor>(
        *acceptorEvb,
        folly::SocketAddress("::1", 0),
        SocketOptions{},
        /*enableReusePortBpfSpread=*/false,
        std::move(workers));
    acceptor->setConnectionEventCallback(eventCallback);
    acceptor->start();
    address = acceptor->getAddress();
  });

  folly::Baton<> connected;
  struct ConnectCallback final : folly::AsyncSocket::ConnectCallback {
    explicit ConnectCallback(folly::Baton<>& baton) : baton_(baton) {}

    void connectSuccess() noexcept override { baton_.post(); }
    void connectErr(const folly::AsyncSocketException&) noexcept override {
      baton_.post();
    }

    folly::Baton<>& baton_;
  } connectCallback(connected);

  std::shared_ptr<folly::AsyncSocket> client;
  workerEvb->runInEventBaseThreadAndWait([&] {
    client = folly::AsyncSocket::newSocket(workerEvb);
    client->connect(&connectCallback, address, 1000);
  });
  connected.wait();
  callback.accepted_.wait();

  EXPECT_FALSE(callback.acceptError_.load(std::memory_order_relaxed));
  EXPECT_TRUE(callback.acceptedOnWorker_.load(std::memory_order_relaxed));
  EXPECT_EQ(eventCallback->accepted.load(std::memory_order_relaxed), 1);
  EXPECT_EQ(eventCallback->enqueued.load(std::memory_order_relaxed), 1);
  EXPECT_EQ(eventCallback->dequeued.load(std::memory_order_relaxed), 1);
  EXPECT_EQ(eventCallback->dropped.load(std::memory_order_relaxed), 0);
  EXPECT_EQ(eventCallback->errors.load(std::memory_order_relaxed), 0);

  acceptor->stop();
  EXPECT_TRUE(callback.stopped_.ready());
  acceptor.reset();
  workerEvb->runInEventBaseThreadAndWait(
      [client = std::move(client)]() mutable { client.reset(); });
}

TEST(ConnectionAcceptorTest, ReportsDirectAcceptWithoutQueueEvents) {
  folly::ScopedEventBaseThread acceptorThread;
  folly::ScopedEventBaseThread clientThread;
  auto* acceptorEvb = acceptorThread.getEventBase();
  auto* clientEvb = clientThread.getEventBase();
  TestAcceptCallback callback(*acceptorEvb);
  auto eventCallback = std::make_shared<TestConnectionEventCallback>();

  ConnectionAcceptor::Ptr acceptor;
  folly::SocketAddress address;
  acceptorEvb->runInEventBaseThreadAndWait([&] {
    std::vector<ConnectionWorkerTarget> workers;
    workers.reserve(1);
    workers.push_back({*acceptorEvb, callback});
    acceptor = std::make_unique<ConnectionAcceptor>(
        *acceptorEvb,
        folly::SocketAddress("::1", 0),
        SocketOptions{},
        /*enableReusePortBpfSpread=*/false,
        std::move(workers));
    acceptor->setConnectionEventCallback(eventCallback);
    acceptor->start();
    address = acceptor->getAddress();
  });

  folly::Baton<> connected;
  struct ConnectCallback final : folly::AsyncSocket::ConnectCallback {
    explicit ConnectCallback(folly::Baton<>& baton) : baton_(baton) {}

    void connectSuccess() noexcept override { baton_.post(); }
    void connectErr(const folly::AsyncSocketException&) noexcept override {
      baton_.post();
    }

    folly::Baton<>& baton_;
  } connectCallback(connected);

  std::shared_ptr<folly::AsyncSocket> client;
  clientEvb->runInEventBaseThreadAndWait([&] {
    client = folly::AsyncSocket::newSocket(clientEvb);
    client->connect(&connectCallback, address, 1000);
  });
  connected.wait();
  callback.accepted_.wait();

  EXPECT_TRUE(callback.acceptedOnWorker_.load(std::memory_order_relaxed));
  EXPECT_EQ(eventCallback->accepted.load(std::memory_order_relaxed), 1);
  EXPECT_EQ(eventCallback->enqueued.load(std::memory_order_relaxed), 0);
  EXPECT_EQ(eventCallback->dequeued.load(std::memory_order_relaxed), 0);
  EXPECT_EQ(eventCallback->dropped.load(std::memory_order_relaxed), 0);
  EXPECT_EQ(eventCallback->errors.load(std::memory_order_relaxed), 0);

  acceptor->stop();
  acceptor.reset();
  clientEvb->runInEventBaseThreadAndWait(
      [client = std::move(client)]() mutable { client.reset(); });
}

TEST(ConnectionAcceptorTest, BoundsPendingFdsWhenWorkerStalls) {
  folly::ScopedEventBaseThread acceptorThread;
  folly::ScopedEventBaseThread workerThread;
  auto* acceptorEvb = acceptorThread.getEventBase();
  auto* workerEvb = workerThread.getEventBase();
  TestAcceptCallback callback(*workerEvb);

  SocketOptions socketOptions;
  socketOptions.maxPendingConnectionsPerWorker = 1;
  socketOptions.pendingConnectionQueueTimeout =
      std::chrono::nanoseconds::zero();

  ConnectionAcceptor::Ptr acceptor;
  folly::SocketAddress address;
  acceptorEvb->runInEventBaseThreadAndWait([&] {
    std::vector<ConnectionWorkerTarget> workers;
    workers.reserve(1);
    workers.push_back({*workerEvb, callback});
    acceptor = std::make_unique<ConnectionAcceptor>(
        *acceptorEvb,
        folly::SocketAddress("127.0.0.1", 0),
        socketOptions,
        /*enableReusePortBpfSpread=*/false,
        std::move(workers));
    acceptor->start();
    address = acceptor->getAddress();
  });
  callback.started_.wait();

  folly::Baton<> workerBlocked;
  folly::Baton<> unblockWorker;
  workerEvb->runInEventBaseThread([&] {
    workerBlocked.post();
    unblockWorker.wait();
  });
  workerBlocked.wait();

  std::vector<folly::NetworkSocket> clients;
  sockaddr_in serverAddress{};
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(address.getPort());
  serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  for (size_t i = 0; i < 8; ++i) {
    auto fd = folly::netops::socket(AF_INET, SOCK_STREAM, 0);
    if (fd == folly::NetworkSocket()) {
      ADD_FAILURE() << "failed to create client socket";
      break;
    }
    if (folly::netops::connect(
            fd,
            reinterpret_cast<const sockaddr*>(&serverAddress),
            sizeof(serverAddress)) != 0) {
      folly::netops::close(fd);
      ADD_FAILURE() << "failed to connect client socket";
      break;
    }
    clients.push_back(fd);
  }

  int64_t pending = 0;
  size_t dropped = 0;
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  do {
    acceptorEvb->runInEventBaseThreadAndWait([&] {
      pending = acceptor->getNumPendingConnections();
      dropped = acceptor->getNumDroppedConnections();
    });
    if (dropped > 0) {
      break;
    }
    folly::Baton<> retryDelay;
    retryDelay.try_wait_for(std::chrono::milliseconds(5));
  } while (std::chrono::steady_clock::now() < deadline);

  EXPECT_LE(pending, 1);
  EXPECT_GT(dropped, 0);

  unblockWorker.post();
  callback.accepted_.wait();
  acceptor->stop();
  EXPECT_TRUE(callback.stopped_.ready());
  acceptor.reset();
  for (auto fd : clients) {
    folly::netops::close(fd);
  }
}

} // namespace
} // namespace apache::thrift::fast_thrift::connection
