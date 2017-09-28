/*
 * Copyright 2004-present Facebook, Inc.
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

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <thrift/example/if/gen-cpp2/ChatRoomService.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = 1;
  folly::init(&argc, &argv);
  folly::EventBase eventBase;

  try {
    // Create a client to the service
    auto socket = apache::thrift::async::TAsyncSocket::newSocket(
        &eventBase, "::1", 7777);
    auto connection = apache::thrift::HeaderClientChannel::newChannel(socket);
    auto client = std::make_unique<
        tutorials::chatroom::ChatRoomServiceAsyncClient>(std::move(connection));

    // Send a message
    tutorials::chatroom::SendMessageRequest sendRequest;
    sendRequest.message = "This is an example!";
    sendRequest.sender = getenv("USER");
    client->sync_sendMessage(sendRequest);

    // Get all the messages
    tutorials::chatroom::GetMessagesRequest getRequest;
    tutorials::chatroom::GetMessagesResponse response;
    client->sync_getMessages(response, getRequest);

    // Print all the messages so far
    for (auto& messagesList : response.messages) {
      LOG(INFO) << "Message: " << messagesList.message
                << " Sender: " << messagesList.sender;
    }
  } catch (apache::thrift::transport::TTransportException& ex) {
    LOG(ERROR) << "Request failed " << ex.what();
  } catch (tutorials::chatroom::Exception& ex) {
    LOG(ERROR) << "Request failed " << ex.what();
  }

  return 0;
}
