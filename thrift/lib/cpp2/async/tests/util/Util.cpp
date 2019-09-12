/*
 * Copyright 2018-present Facebook, Inc.
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

#include "thrift/lib/cpp2/async/tests/util/Util.h"

#include <folly/experimental/coro/BlockingWait.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>

namespace apache {
namespace thrift {

using namespace testutil::testservice;

void TestSetup::initServer() {
  server_ = std::make_unique<ThriftServer>();
  server_->setPort(0);
  server_->setNumIOWorkerThreads(numIOThreads_);
  server_->setNumCPUWorkerThreads(numWorkerThreads_);
  server_->enableRocketServer(true);
  server_->setQueueTimeout(std::chrono::milliseconds(0));
  server_->setIdleTimeout(std::chrono::milliseconds(0));
  server_->setTaskExpireTime(std::chrono::milliseconds(0));
  server_->setStreamExpireTime(std::chrono::milliseconds(0));

  handler_ = std::make_shared<TestSinkService>();
  server_->setProcessorFactory(
      std::make_shared<ThriftServerAsyncProcessorFactory<TestSinkService>>(
          handler_));

  server_->addRoutingHandler(
      std::make_unique<apache::thrift::RSRoutingHandler>());
  auto eventHandler = std::make_shared<TestEventHandler>();
  server_->setServerEventHandler(eventHandler);
  server_->setup();

  // Get the port that the server has bound to
  serverPort_ = eventHandler->waitForPortAssignment();
}

void TestSetup::connectToServer(
    folly::Function<folly::coro::Task<void>(
        testutil::testservice::TestSinkServiceAsyncClient&)> callMe) {
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
    testutil::testservice::TestSinkServiceAsyncClient client(
        std::move(channel));
    co_await callMe(client);
  }());
}

} // namespace thrift
} // namespace apache
