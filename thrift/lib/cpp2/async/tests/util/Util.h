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

#include <folly/SocketAddress.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/async/tests/util/TestSinkService.h>
#include <thrift/lib/cpp2/async/tests/util/TestStreamService.h>
#include <thrift/lib/cpp2/async/tests/util/gen-cpp2/TestSinkServiceAsyncClient.h>
#include <thrift/lib/cpp2/async/tests/util/gen-cpp2/TestStreamServiceAsyncClient.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/core/testutil/TAsyncSocketIntercepted.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/TestUtil.h>

namespace apache {
namespace thrift {

template <class Handler, class Client>
class AsyncTestSetup : public TestSetup {
 protected:
  void SetUp() override {
    handler_ = std::make_shared<Handler>();
    setNumIOThreads(numIOThreads_);
    setNumWorkerThreads(numWorkerThreads_);
    setQueueTimeout(std::chrono::milliseconds(0));
    setIdleTimeout(std::chrono::milliseconds(0));
    setTaskExpireTime(std::chrono::milliseconds(0));
    setStreamExpireTime(std::chrono::milliseconds(0));
    server_ = createServer(
        std::make_shared<ThriftServerAsyncProcessorFactory<Handler>>(handler_),
        serverPort_);
  }

  void TearDown() override {
    if (server_) {
      server_->cleanUp();
      server_.reset();
      handler_.reset();
    }
  }

  void connectToServer(
      folly::Function<folly::coro::Task<void>(Client&)> callMe,
      bool enableCompressionForRocketClient = false) {
    folly::coro::blockingWait(
        [this,
         &callMe,
         enableCompressionForRocketClient]() -> folly::coro::Task<void> {
          CHECK_GT(serverPort_, 0) << "Check if the server has started already";
          folly::Executor* executor = co_await folly::coro::co_current_executor;
          auto newChannel = PooledRequestChannel::newChannel(
              executor, ioThread_, [&](folly::EventBase& evb) {
                socket_ = new apache::thrift::async::TAsyncSocketIntercepted(
                    &evb, "::1", serverPort_);
                auto socket = folly::AsyncSocket::UniquePtr(socket_);
                auto channel =
                    [&]() -> std::unique_ptr<
                              ClientChannel,
                              folly::DelayedDestruction::Destructor> {
                  return apache::thrift::RocketClientChannel::newChannel(
                      std::move(socket));
                }();

                if (enableCompressionForRocketClient) {
                  auto rocketChannel =
                      dynamic_cast<RocketClientChannel*>(channel.get());
                  rocketChannel->setNegotiatedCompressionAlgorithm(
                      CompressionAlgorithm::ZSTD);
                  rocketChannel->setAutoCompressSizeLimit(0);
                }
                return channel;
              });
          Client client(std::move(newChannel));
          co_await callMe(client);
        }());
  }

 protected:
  int numIOThreads_{1};
  int numWorkerThreads_{1};
  uint16_t serverPort_{0};
  std::shared_ptr<folly::IOExecutor> ioThread_{
      std::make_shared<folly::ScopedEventBaseThread>()};
  std::unique_ptr<ThriftServer> server_;
  std::shared_ptr<Handler> handler_;
  // store pointer to socket in order to check the total number of bytes
  // read/written
  apache::thrift::async::TAsyncSocketIntercepted* socket_;
};

} // namespace thrift
} // namespace apache
