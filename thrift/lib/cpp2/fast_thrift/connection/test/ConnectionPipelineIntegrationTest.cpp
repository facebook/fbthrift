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

/**
 * AcceptancePipelineIntegrationTest
 *
 * End-to-end exercise of the acceptance pipeline head:
 *   ConnectionListener (head) → MockTailHandler
 *
 * Drives connectionAccepted() directly with an accepted IPv6 TCP fd and
 * asserts the mock tail receives a configured AsyncSocket. Bypasses the
 * listener's bind/listen path.
 */

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <memory>

#include <gtest/gtest.h>

#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/net/NetworkSocket.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockAdapters.h>
#include <thrift/lib/cpp2/fast_thrift/connection/SocketOptions.h>
#include <thrift/lib/cpp2/fast_thrift/connection/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/connection/endpoint/ConnectionListener.h>

namespace apache::thrift::fast_thrift::connection {

using channel_pipeline::PipelineBuilder;
using channel_pipeline::PipelineImpl;
using channel_pipeline::Result;
using channel_pipeline::SimpleBufferAllocator;
using channel_pipeline::TypeErasedBox;
using channel_pipeline::test::MockTailHandler;

namespace {

std::pair<folly::NetworkSocket, folly::NetworkSocket> makeTcpSocketPair() {
  const auto listener =
      folly::NetworkSocket(::socket(AF_INET6, SOCK_STREAM, 0));
  PCHECK(listener.toFd() >= 0);

  sockaddr_in6 address{};
  address.sin6_family = AF_INET6;
  address.sin6_addr = in6addr_loopback;
  PCHECK(
      ::bind(
          listener.toFd(),
          reinterpret_cast<const sockaddr*>(&address),
          sizeof(address)) == 0);
  PCHECK(::listen(listener.toFd(), 1) == 0);

  socklen_t addressLength = sizeof(address);
  PCHECK(
      ::getsockname(
          listener.toFd(),
          reinterpret_cast<sockaddr*>(&address),
          &addressLength) == 0);

  const auto client = folly::NetworkSocket(::socket(AF_INET6, SOCK_STREAM, 0));
  PCHECK(client.toFd() >= 0);
  PCHECK(
      ::connect(
          client.toFd(),
          reinterpret_cast<const sockaddr*>(&address),
          sizeof(address)) == 0);

  const auto server =
      folly::NetworkSocket(::accept(listener.toFd(), nullptr, nullptr));
  PCHECK(server.toFd() >= 0);
  folly::netops::close(listener);
  return {client, server};
}

} // namespace

class AcceptancePipelineIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    evbThread_ = std::make_unique<folly::ScopedEventBaseThread>();
    evb_ = evbThread_->getEventBase();
  }
  void TearDown() override { evbThread_.reset(); }

  std::unique_ptr<folly::ScopedEventBaseThread> evbThread_;
  folly::EventBase* evb_{nullptr};
  SimpleBufferAllocator allocator_;
};

// Listener → tail delivers the configured AsyncSocket synchronously.
TEST_F(AcceptancePipelineIntegrationTest, DeliversConfiguredSocket) {
  MockTailHandler tail;
  ConnectionMessage captured;
  tail.setOnReadCallback([&captured](TypeErasedBox&& msg) {
    captured = msg.take<ConnectionMessage>();
    return Result::Success;
  });

  SocketOptions socketOptions;
  socketOptions.tcpNoDelay = true;
  socketOptions.trafficClass = 72;
  ConnectionListener::Ptr listener(new ConnectionListener(
      evb_,
      folly::SocketAddress("::1", 0),
      socketOptions,
      /*enableReusePortBpfSpread=*/false));
  auto pipeline = PipelineBuilder<
                      ConnectionListener,
                      MockTailHandler,
                      SimpleBufferAllocator>()
                      .setEventBase(evb_)
                      .setHead(listener.get())
                      .setTail(&tail)
                      .setAllocator(&allocator_)
                      .build();
  listener->setPipeline(pipeline.get());
  // Activate the pipeline directly without start(); we don't need to
  // bind a real listening socket for this test.
  evb_->runInEventBaseThreadAndWait([&] { pipeline->activate(); });

  auto sp = makeTcpSocketPair();
  folly::SocketAddress clientAddr("::1", 4001);
  evb_->runInEventBaseThreadAndWait([&] {
    listener->connectionAccepted(
        sp.second,
        clientAddr,
        folly::AsyncServerSocket::AcceptCallback::AcceptInfo{});
  });

  EXPECT_EQ(tail.readCount(), 1);
  EXPECT_EQ(captured.clientAddr, clientAddr);
  auto* socket = dynamic_cast<folly::AsyncSocket*>(captured.transport.get());
  ASSERT_NE(socket, nullptr);

  int tcpNoDelay = 0;
  socklen_t optionLength = sizeof(tcpNoDelay);
  ASSERT_EQ(
      ::getsockopt(
          socket->getNetworkSocket().toFd(),
          IPPROTO_TCP,
          TCP_NODELAY,
          &tcpNoDelay,
          &optionLength),
      0);
  EXPECT_NE(tcpNoDelay, 0);

  int trafficClass = 0;
  optionLength = sizeof(trafficClass);
  ASSERT_EQ(
      ::getsockopt(
          socket->getNetworkSocket().toFd(),
          IPPROTO_IPV6,
          IPV6_TCLASS,
          &trafficClass,
          &optionLength),
      0);
  EXPECT_EQ(trafficClass, socketOptions.trafficClass);

  evb_->runInEventBaseThreadAndWait([&] {
    captured.transport.reset();
    listener->resetPipeline();
    pipeline.reset();
    listener.reset();
  });
  folly::netops::close(sp.first);
}

} // namespace apache::thrift::fast_thrift::connection
