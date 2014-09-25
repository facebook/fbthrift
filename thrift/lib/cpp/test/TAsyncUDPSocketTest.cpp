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

#include <thrift/lib/cpp/async/TAsyncUDPSocket.h>
#include <thrift/lib/cpp/async/TAsyncUDPServerSocket.h>
#include <thrift/lib/cpp/async/TAsyncTimeout.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/test/ScopedEventBaseThread.h>

#include <folly/io/IOBuf.h>

#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <thread>
using namespace boost;

using apache::thrift::async::TAsyncUDPSocket;
using apache::thrift::async::TAsyncUDPServerSocket;
using apache::thrift::async::TAsyncTimeout;
using apache::thrift::async::TEventBase;
using folly::SocketAddress;
using apache::thrift::transport::TTransportException;
using apache::thrift::test::ScopedEventBaseThread;
using folly::IOBuf;

class UDPAcceptor
    : public TAsyncUDPServerSocket::Callback {
 public:
  UDPAcceptor(TEventBase* evb, int n): evb_(evb), n_(n) {
  }

  void onListenStarted() noexcept {
  }

  void onListenStopped() noexcept {
  }

  void onDataAvailable(const folly::SocketAddress& client,
                       std::unique_ptr<folly::IOBuf> data,
                       bool truncated) noexcept {

    lastClient_ = client;
    lastMsg_ = data->moveToFbString().toStdString();

    auto len = data->computeChainDataLength();
    VLOG(4) << "Worker " << n_ << " read " << len << " bytes "
            << "(trun:" << truncated << ") from " << client.describe()
            << " - " << lastMsg_;

    sendPong();
  }

  void sendPong() noexcept {
    try {
      TAsyncUDPSocket socket(evb_);
      socket.bind(folly::SocketAddress("127.0.0.1", 0));
      socket.write(lastClient_, folly::IOBuf::copyBuffer(lastMsg_));
    } catch (const std::exception& ex) {
      VLOG(4) << "Failed to send PONG " << ex.what();
    }
  }

 private:
  TEventBase* const evb_{nullptr};
  const int n_{-1};

  folly::SocketAddress lastClient_;
  std::string lastMsg_;
};

class UDPServer {
 public:
  UDPServer(TEventBase* evb, folly::SocketAddress addr, int n)
      : evb_(evb), addr_(addr), evbs_(n) {
  }

  void start() {
    CHECK(evb_->isInEventBaseThread());

    socket_ = folly::make_unique<TAsyncUDPServerSocket>(
        evb_,
        1500);

    try {
      socket_->bind(addr_);
      VLOG(4) << "Server listening on " << socket_->address().describe();
    } catch (const std::exception& ex) {
      LOG(FATAL) << ex.what();
    }

    acceptors_.reserve(evbs_.size());
    threads_.reserve(evbs_.size());

    // Add numWorkers thread
    int i = 0;
    for (auto& evb: evbs_) {
      acceptors_.emplace_back(&evb, i);

      std::thread t([&] () {
        evb.loopForever();
      });

      auto r = std::make_shared<boost::barrier>(2);
      evb.runInEventBaseThread([r] () {
        r->wait();
      });
      r->wait();

      socket_->addListener(&evb, &acceptors_[i]);
      threads_.emplace_back(std::move(t));
      ++i;
    }

    socket_->listen();
  }

  folly::SocketAddress address() const {
    return socket_->address();
  }

  void shutdown() {
    CHECK(evb_->isInEventBaseThread());
    socket_->close();
    socket_.reset();

    for (auto& evb: evbs_) {
      evb.terminateLoopSoon();
    }

    for (auto& t: threads_) {
      t.join();
    }
  }

 private:
  TEventBase* const evb_{nullptr};
  const folly::SocketAddress addr_;

  std::unique_ptr<TAsyncUDPServerSocket> socket_;
  std::vector<std::thread> threads_;
  std::vector<apache::thrift::async::TEventBase> evbs_;
  std::vector<UDPAcceptor> acceptors_;
};

class UDPClient
    : private TAsyncUDPSocket::ReadCallback,
      private TAsyncTimeout {
 public:
  explicit UDPClient(TEventBase* evb)
      : TAsyncTimeout(evb),
        evb_(evb) {
  }

  void start(const folly::SocketAddress& server, int n) {
    CHECK(evb_->isInEventBaseThread());

    server_ = server;
    socket_ = folly::make_unique<TAsyncUDPSocket>(evb_);

    try {
      socket_->bind(folly::SocketAddress("127.0.0.1", 0));
      VLOG(4) << "Client bound to " << socket_->address().describe();
    } catch (const std::exception& ex) {
      LOG(FATAL) << ex.what();
    }

    socket_->resumeRead(this);

    n_ = n;

    // Start playing ping pong
    sendPing();
  }

  void shutdown() {
    CHECK(evb_->isInEventBaseThread());
    socket_->pauseRead();
    socket_->close();
    socket_.reset();
    evb_->terminateLoopSoon();
  }

  void sendPing() {
    if (n_ == 0) {
      shutdown();
      return;
    }

    --n_;
    scheduleTimeout(5);
    socket_->write(
        server_,
        folly::IOBuf::copyBuffer(folly::to<std::string>("PING ", n_)));
  }

  void getReadBuffer(void** buf, size_t* len) noexcept {
    *buf = buf_;
    *len = 1024;
  }

  void onDataAvailable(const folly::SocketAddress& client,
                       size_t len,
                       bool truncated) noexcept {
    VLOG(4) << "Read " << len << " bytes (trun:" << truncated << ") from "
              << client.describe() << " - " << std::string(buf_, len);
    VLOG(4) << n_ << " left";

    ++pongRecvd_;

    sendPing();
  }

  void onReadError(const TTransportException& ex) noexcept {
    VLOG(4) << ex.what();

    // Start listening for next PONG
    socket_->resumeRead(this);
  }

  void onReadClosed() noexcept {
    CHECK(false) << "We unregister reads before closing";
  }

  void timeoutExpired() noexcept {
    VLOG(4) << "Timeout expired";
    sendPing();
  }

  int pongRecvd() const {
    return pongRecvd_;
  }

 private:
  TEventBase* const evb_{nullptr};

  folly::SocketAddress server_;
  std::unique_ptr<TAsyncUDPSocket> socket_;

  int pongRecvd_{0};

  int n_{0};
  char buf_[1024];
};

BOOST_AUTO_TEST_CASE(PingPong) {
  apache::thrift::async::TEventBase sevb;
  UDPServer server(&sevb, folly::SocketAddress("127.0.0.1", 0), 4);
  boost::barrier barrier(2);

  // Start event loop in a separate thread
  auto serverThread = std::thread([&sevb] () {
    sevb.loopForever();
  });

  // Wait for event loop to start
  sevb.runInEventBaseThread([&] () { barrier.wait(); });
  barrier.wait();

  // Start the server
  sevb.runInEventBaseThread([&] () { server.start(); barrier.wait(); });
  barrier.wait();

  apache::thrift::async::TEventBase cevb;
  UDPClient client(&cevb);

  // Start event loop in a separate thread
  auto clientThread = std::thread([&cevb] () {
    cevb.loopForever();
  });

  // Wait for event loop to start
  cevb.runInEventBaseThread([&] () { barrier.wait(); });
  barrier.wait();

  // Send ping
  cevb.runInEventBaseThread([&] () { client.start(server.address(), 1000); });

  // Wait for client to finish
  clientThread.join();

  // Check that some PING/PONGS were exchanged. Out of 1000 transactions
  // at least 1 should succeed
  BOOST_CHECK_GT(client.pongRecvd(), 0);

  // Shutdown server
  sevb.runInEventBaseThread([&] () {
    server.shutdown();
    sevb.terminateLoopSoon();
  });

  // Wait for server thread to joib
  serverThread.join();
}

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  unit_test::framework::master_test_suite().p_name.value = "TAsyncUDPSockeTest";
  return nullptr;
}
