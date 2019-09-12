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

#pragma once

#include <gtest/gtest.h>

#include <folly/SocketAddress.h>
#include <folly/experimental/coro/Task.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include <thrift/lib/cpp2/async/tests/util/TestSinkService.h>
#include <thrift/lib/cpp2/async/tests/util/gen-cpp2/TestSinkServiceAsyncClient.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

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

class TestSetup : public testing::Test {
 protected:
  void initServer();

  void connectToServer(
      folly::Function<folly::coro::Task<void>(
          testutil::testservice::TestSinkServiceAsyncClient&)> callMe);

  void SetUp() override {
    initServer();
  }

  std::unique_ptr<ThriftServer> server_;
  std::shared_ptr<testutil::testservice::TestSinkService> handler_;

  void TearDown() override {
    if (server_) {
      server_->cleanUp();
      server_.reset();
    }
  }

  int numIOThreads_{1};
  int numWorkerThreads_{1};
  int32_t serverPort_{0};
  std::shared_ptr<folly::IOExecutor> ioThread_{
      std::make_shared<folly::ScopedEventBaseThread>()};
};

} // namespace thrift
} // namespace apache
