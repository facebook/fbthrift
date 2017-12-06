/*
 * Copyright 2017-present Facebook, Inc.
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

#include <gtest/gtest.h>

#include <folly/futures/Future.h>
#include <folly/synchronization/Baton.h>
#include <thrift/example/cpp2/server/ChatRoomService.h>
#include <thrift/example/if/gen-cpp2/ChatRoomService.h>
#include <thrift/lib/cpp2/transport/core/testutil/ServerConfigsMock.h>
#include <thrift/perf/cpp2/util/Util.h>

using example::chatroom::ChatRoomServiceAsyncClient;
using example::chatroom::ChatRoomServiceHandler;
using example::chatroom::GetMessagesRequest;
using example::chatroom::GetMessagesResponse;
using example::chatroom::SendMessageRequest;

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

class ChatRoomTest : public testing::Test {
 public:
  ChatRoomTest() {
    handler_ = std::make_shared<ChatRoomServiceHandler>();
    client_ =
        newInMemoryClient<ChatRoomServiceAsyncClient, ChatRoomServiceHandler>(
            handler_, serverConfigs_);
  }

  std::unique_ptr<ChatRoomServiceAsyncClient> client_;

 private:
  ServerConfigsMock serverConfigs_;
  std::shared_ptr<ChatRoomServiceHandler> handler_;
};

TEST_F(ChatRoomTest, SyncCall) {
  // Send RPC to Server
  SendMessageRequest sendRequest;
  sendRequest.message = "This is an example!";
  sendRequest.sender = "UnitTest";
  client_->sync_sendMessage(sendRequest);

  // Send RPC to get Results
  GetMessagesRequest getRequest;
  GetMessagesResponse response;
  client_->sync_getMessages(response, getRequest);
  EXPECT_EQ(response.messages.size(), 1);
  EXPECT_EQ(response.messages.front().message, sendRequest.message);
  EXPECT_EQ(response.messages.front().sender, sendRequest.sender);

  // Repeat
  client_->sync_sendMessage(sendRequest);
  client_->sync_getMessages(response, getRequest);
  EXPECT_EQ(response.messages.size(), 2);
}

TEST_F(ChatRoomTest, AsyncCall) {
  // Send RPC to Server
  ClientReceiveState sendResult;
  folly::Baton<> sendBaton;
  auto sendCb = std::make_unique<AsyncCallback>(&sendResult, &sendBaton);
  SendMessageRequest sendRequest;
  sendRequest.message = "This is an example!";
  sendRequest.sender = "UnitTest";
  client_->sendMessage(std::move(sendCb), sendRequest);
  sendBaton.wait();

  // Send RPC to get Results
  GetMessagesRequest getRequest;
  GetMessagesResponse response;
  ClientReceiveState getResult;
  folly::Baton<> getBaton;
  auto getCb = std::make_unique<AsyncCallback>(&getResult, &getBaton);
  client_->getMessages(std::move(getCb), getRequest);
  getBaton.wait();

  // Receive Result
  client_->recv_getMessages(response, getResult);
  EXPECT_EQ(response.messages.size(), 1);

  // Repeat
  ClientReceiveState sendResult2;
  folly::Baton<> sendBaton2;
  auto sendCb2 = std::make_unique<AsyncCallback>(&sendResult2, &sendBaton2);
  ClientReceiveState getResult2;
  folly::Baton<> getBaton2;
  auto getCb2 = std::make_unique<AsyncCallback>(&getResult2, &getBaton2);
  client_->sendMessage(std::move(sendCb2), sendRequest);
  sendBaton2.wait();
  client_->getMessages(std::move(getCb2), getRequest);
  getBaton2.wait();

  // Receive Result
  client_->recv_getMessages(response, getResult2);
  EXPECT_EQ(response.messages.size(), 2);
}

TEST_F(ChatRoomTest, FuturesCall) {
  // Send RPC to Server
  SendMessageRequest sendRequest;
  sendRequest.message = "This is an example!";
  sendRequest.sender = "UnitTest";
  client_->future_sendMessage(sendRequest).get();

  // Send RPC to get Results
  GetMessagesRequest getRequest;
  auto response = client_->future_getMessages(getRequest).get();
  EXPECT_EQ(response.messages.size(), 1);
  EXPECT_EQ(response.messages.front().message, sendRequest.message);
  EXPECT_EQ(response.messages.front().sender, sendRequest.sender);

  // Repeat
  client_->future_sendMessage(sendRequest).get();
  response = client_->future_getMessages(getRequest).get();
  EXPECT_EQ(response.messages.size(), 2);
}

} // namespace thrift
} // namespace apache
