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

#include <folly/portability/GTest.h>

#include <folly/futures/Future.h>
#include <folly/synchronization/Baton.h>
#include <thrift/example/cpp2/server/EchoService.h>
#include <thrift/example/if/gen-cpp2/Echo.h>
#include <thrift/lib/cpp2/transport/core/testutil/ServerConfigsMock.h>
#include <thrift/perf/cpp2/util/Util.h>

using example::chatroom::EchoAsyncClient;
using example::chatroom::EchoHandler;

namespace apache {
namespace thrift {

class AsyncCallback : public RequestCallback {
 public:
  AsyncCallback(ClientReceiveState* result, folly::Baton<>* baton)
      : result_(result), baton_(baton) {}

  void requestSent() override {}

  void replyReceived(ClientReceiveState&& state) override {
    *result_ = std::move(state);
    baton_->post();
  }

  void requestError(ClientReceiveState&& state) override {
    *result_ = std::move(state);
    baton_->post();
  }

 private:
  ClientReceiveState* result_;
  folly::Baton<>* baton_;
};

class EchoTest : public testing::Test {
 public:
  EchoTest() {
    handler_ = std::make_shared<EchoHandler>();
    client_ = newInMemoryClient<EchoAsyncClient, EchoHandler>(
        handler_, serverConfigs_);
  }

  std::unique_ptr<EchoAsyncClient> client_;

 private:
  ServerConfigsMock serverConfigs_;
  std::shared_ptr<EchoHandler> handler_;
};

TEST_F(EchoTest, AsyncCall) {
  ClientReceiveState result;
  folly::Baton<> baton;
  auto cb = std::make_unique<AsyncCallback>(&result, &baton);
  std::string echo = "Echo Message";
  client_->echo(std::move(cb), echo);
  baton.wait();

  std::string response;
  client_->recv_echo(response, result);
  EXPECT_EQ(echo, response);
}

TEST_F(EchoTest, FuturesCall) {
  std::string echo = "Echo Message";
  EXPECT_EQ(echo, client_->future_echo(echo).get());
}

} // namespace thrift
} // namespace apache
