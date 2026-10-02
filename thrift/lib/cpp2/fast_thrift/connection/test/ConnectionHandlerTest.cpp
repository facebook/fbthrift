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

#include <sys/socket.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include <folly/SocketAddress.h>
#include <folly/init/Init.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/net/NetworkSocket.h>
#include <folly/observer/SimpleObservable.h>

#include <thrift/lib/cpp2/fast_thrift/connection/ConnectionHandler.h>

namespace apache::thrift::fast_thrift::connection {
namespace {

// Test connection — keeps the transport alive, counts close calls via a
// shared atomic so tests can verify teardown, and treats drain() as an
// immediate close (firing the close callback so the handler can erase
// the entry).
//
// `closeOnStart` exists to exercise the synchronous-close-during-start
// invariant: when set, start() immediately invokes close(), which fires
// the close callback under the same installer-lambda frame that just
// registered this connection.
struct TestConnection {
  folly::AsyncTransport::UniquePtr transport;
  std::shared_ptr<std::atomic<size_t>> closeCount;
  std::function<void()> closeCb;
  bool closed{false};
  bool closeOnStart{false};

  void setCloseCallback(std::function<void()> cb) { closeCb = std::move(cb); }

  void start() noexcept {
    if (closeOnStart) {
      close();
    }
  }

  void drain() noexcept { close(); }

  void close() noexcept {
    if (closed) {
      return;
    }
    closed = true;
    if (transport) {
      transport->closeNow();
    }
    if (closeCount) {
      closeCount->fetch_add(1, std::memory_order_relaxed);
    }
    if (closeCb) {
      auto cb = std::move(closeCb);
      cb();
    }
  }
};

// Factory satisfying connection::ConnectionFactory.
class TestConnectionFactory {
 public:
  explicit TestConnectionFactory(
      std::shared_ptr<std::atomic<size_t>> closeCount = nullptr,
      bool closeOnStart = false) noexcept
      : closeCount_(std::move(closeCount)), closeOnStart_(closeOnStart) {}

  TestConnection getConnection(
      folly::AsyncTransport::UniquePtr socket,
      const folly::SocketAddress& clientAddr,
      const std::shared_ptr<const PeerSecurityInfo>& peerSecurity) {
    lastClientAddr = clientAddr;
    lastPeerSecurity = peerSecurity;
    return TestConnection{
        .transport = std::move(socket),
        .closeCount = closeCount_,
        .closeCb = {},
        .closeOnStart = closeOnStart_};
  }

  // Written on the data EVB; read back through that EVB.
  folly::SocketAddress lastClientAddr;
  std::shared_ptr<const PeerSecurityInfo> lastPeerSecurity;

 private:
  std::shared_ptr<std::atomic<size_t>> closeCount_;
  bool closeOnStart_;
};

} // namespace

class ConnectionHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    evbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    evb_ = evbThread_->getEventBase();
  }

  void TearDown() override {
    for (auto fd : clientFds_) {
      folly::netops::close(fd);
    }
    evbThread_.reset();
  }

  std::unique_ptr<ConnectionHandler> createConnectionHandler() {
    return std::make_unique<ConnectionHandler>(
        *evb_,
        fast_security::SSLPolicy::DISABLED,
        folly::observer::SimpleObservable<
            std::shared_ptr<const fast_security::TLSParams>>(
            std::shared_ptr<const fast_security::TLSParams>{})
            .getObserver(),
        SocketOptions{},
        std::nullopt);
  }

  // Wires the factory + starts the installation pipeline. Runs on the
  // owning EVB because setConnectionFactory builds + activates the pipeline.
  // Factory is stored on the test fixture so it outlives the handler.
  void wireFactory(ConnectionHandler& h, bool closeOnStart = false) {
    factory_ =
        std::make_unique<TestConnectionFactory>(closeCount_, closeOnStart);
    evb_->runInEventBaseThreadAndWait(
        [&] { h.setConnectionFactory(*factory_); });
  }

  void install(ConnectionHandler& handler) {
    std::array<int, 2> fds{};
    ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM, 0, fds.data()), 0);
    clientFds_.emplace_back(fds[0]);
    evb_->runInEventBaseThreadAndWait([&] {
      handler.listener().connectionAccepted(
          folly::NetworkSocket{fds[1]},
          folly::SocketAddress{"127.0.0.1", 5001},
          folly::AsyncServerSocket::AcceptCallback::AcceptInfo{});
    });
  }

  std::unique_ptr<folly::ScopedEventBaseThread> evbThread_;
  folly::EventBase* evb_{nullptr};
  std::shared_ptr<std::atomic<size_t>> closeCount_ =
      std::make_shared<std::atomic<size_t>>(0);
  std::unique_ptr<TestConnectionFactory> factory_;
  std::vector<folly::NetworkSocket> clientFds_;
};

TEST_F(ConnectionHandlerTest, ConstructDoesNotStart) {
  auto handler = createConnectionHandler();
  EXPECT_NE(handler, nullptr);
  EXPECT_EQ(handler->connectionCount(), 0);
}

TEST_F(ConnectionHandlerTest, AcceptsSingleConnection) {
  auto handler = createConnectionHandler();
  wireFactory(*handler);

  install(*handler);

  EXPECT_EQ(handler->connectionCount(), 1);
}

TEST_F(ConnectionHandlerTest, AcceptsMultipleConnections) {
  auto handler = createConnectionHandler();
  wireFactory(*handler);

  for (int i = 0; i < 3; ++i) {
    install(*handler);
  }

  EXPECT_EQ(handler->connectionCount(), 3);
}

TEST_F(ConnectionHandlerTest, StopDrainsAllConnections) {
  auto handler = createConnectionHandler();
  wireFactory(*handler);

  for (int i = 0; i < 3; ++i) {
    install(*handler);
  }
  ASSERT_EQ(handler->connectionCount(), 3);

  // stop() is off-EVB — bounces internally for the synchronous phases.
  handler->stop();

  EXPECT_EQ(handler->connectionCount(), 0);
  EXPECT_EQ(closeCount_->load(), 3);
}

TEST_F(ConnectionHandlerTest, StopWithNoConnections) {
  auto handler = createConnectionHandler();
  wireFactory(*handler);
  ASSERT_EQ(handler->connectionCount(), 0);

  handler->stop();

  EXPECT_EQ(handler->connectionCount(), 0);
}

TEST_F(ConnectionHandlerTest, DestroyWithActiveConnections) {
  auto handler = createConnectionHandler();
  wireFactory(*handler);
  install(*handler);
  ASSERT_EQ(handler->connectionCount(), 1);

  // Destructor drives stop() — should release live connections cleanly.
  handler.reset();
  EXPECT_EQ(closeCount_->load(), 1);
}

// Regression: the installer lambda must register the connection in
// connections_ BEFORE calling start(). A factory-built connection whose
// start() synchronously closes (modelling a post-StopTLS handoff that
// dispatches and tears down inline) would otherwise fire its close callback
// against a not-yet-registered entry, missing the erase + count decrement.
TEST_F(ConnectionHandlerTest, SynchronousCloseDuringStartIsSafe) {
  auto handler = createConnectionHandler();
  wireFactory(*handler, /*closeOnStart=*/true);
  install(*handler);
  EXPECT_EQ(handler->connectionCount(), 0)
      << "synchronous close during start() must remove the registered entry";
  EXPECT_EQ(closeCount_->load(), 1);
}

// The factory is handed the peer address observed at accept, so it never has
// to re-derive one from a transport whose peer may already be gone.
TEST_F(ConnectionHandlerTest, FactoryReceivesPeerAddress) {
  auto handler = createConnectionHandler();
  wireFactory(*handler);

  install(*handler);

  folly::SocketAddress seen;
  evb_->runInEventBaseThreadAndWait([&] { seen = factory_->lastClientAddr; });

  EXPECT_EQ(seen, folly::SocketAddress("127.0.0.1", 5001));
}

} // namespace apache::thrift::fast_thrift::connection

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  folly::Init init(&argc, &argv);
  return RUN_ALL_TESTS();
}
