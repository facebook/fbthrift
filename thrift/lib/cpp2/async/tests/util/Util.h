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
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/async/tests/util/TestSinkService.h>
#include <thrift/lib/cpp2/async/tests/util/TestStreamService.h>
#include <thrift/lib/cpp2/async/tests/util/gen-cpp2/TestSinkServiceAsyncClient.h>
#include <thrift/lib/cpp2/async/tests/util/gen-cpp2/TestStreamServiceAsyncClient.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>

namespace apache {
namespace thrift {

// Event handler to attach to the Thrift server so we know when it is
// ready to serve and also so we can determine the port it is
// listening on.
class TestEventHandler : public server::TServerEventHandler {
 public:
  // This is a callback that is called when the Thrift server has
  // initialized and is ready to serve RPCs.
  void preServe(const folly::SocketAddress* address) override {
    port_ = address->getPort();
    baton_.post();
  }

  int32_t waitForPortAssignment() {
    baton_.wait();
    return port_;
  }

 private:
  folly::Baton<> baton_;
  int32_t port_;
};

template <class Handler, class Client>
class TestSetup {
 protected:
  TestSetup() {
    server_ = std::make_unique<ThriftServer>();
    server_->setPort(0);
    server_->setNumIOWorkerThreads(numIOThreads_);
    server_->setNumCPUWorkerThreads(numWorkerThreads_);
    server_->enableRocketServer(true);
    server_->setQueueTimeout(std::chrono::milliseconds(0));
    server_->setIdleTimeout(std::chrono::milliseconds(0));
    server_->setTaskExpireTime(std::chrono::milliseconds(0));
    server_->setStreamExpireTime(std::chrono::milliseconds(0));

    handler_ = std::make_shared<Handler>();
    server_->setProcessorFactory(
        std::make_shared<ThriftServerAsyncProcessorFactory<Handler>>(handler_));

    server_->addRoutingHandler(
        std::make_unique<apache::thrift::RSRoutingHandler>());
    auto eventHandler = std::make_shared<TestEventHandler>();
    server_->setServerEventHandler(eventHandler);
    server_->setup();

    // Get the port that the server has bound to
    serverPort_ = eventHandler->waitForPortAssignment();
  }

  ~TestSetup() {
    server_->cleanUp();
    server_.reset();
  }

  void connectToServer(
      folly::Function<folly::coro::Task<void>(Client&)> callMe) {
    folly::coro::blockingWait([this, &callMe]() -> folly::coro::Task<void> {
      CHECK_GT(serverPort_, 0) << "Check if the server has started already";
      folly::Executor* executor = co_await folly::coro::co_current_executor;
      auto channel = PooledRequestChannel::newChannel(
          executor, ioThread_, [&](folly::EventBase& evb) {
            return apache::thrift::RocketClientChannel::newChannel(
                apache::thrift::async::TAsyncSocket::UniquePtr(
                    new apache::thrift::async::TAsyncSocket(
                        &evb, "::1", serverPort_)));
          });
      Client client(std::move(channel));
      co_await callMe(client);
    }());
  }

 private:
  int numIOThreads_{1};
  int numWorkerThreads_{1};
  int32_t serverPort_{0};
  std::shared_ptr<folly::IOExecutor> ioThread_{
      std::make_shared<folly::ScopedEventBaseThread>()};
  std::unique_ptr<ThriftServer> server_;
  std::shared_ptr<Handler> handler_;
};

} // namespace thrift
} // namespace apache
