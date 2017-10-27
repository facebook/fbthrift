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

#include <folly/SocketAddress.h>
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <thrift/example/cpp2/server/ChatRoomService.h>
#include <thrift/example/if/gen-cpp2/ChatRoomService.h>
#include <thrift/lib/cpp2/transport/core/testutil/ServerConfigsMock.h>
#include <thrift/perf/cpp2/util/Util.h>

DEFINE_string(host, "::1", "ChatroomServer host");
DEFINE_int32(port, 7777, "ChatRoomServer port");
DEFINE_string(
    transport,
    "header",
    "Transport to use: header, rsocket, http2, or inmemory");

using apache::thrift::server::ServerConfigsMock;
using example::chatroom::ChatRoomServiceAsyncClient;
using example::chatroom::ChatRoomServiceHandler;

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = true;
  folly::init(&argc, &argv);

  // Create a thrift client
  auto addr = folly::SocketAddress(FLAGS_host, FLAGS_port);
  auto ct = std::make_shared<ConnectionThread<ChatRoomServiceAsyncClient>>();
  auto client = ct->newSyncClient(addr, FLAGS_transport);

  // For header transport
  folly::EventBase evb;
  if (FLAGS_transport == "header") {
    client = newHeaderClient<ChatRoomServiceAsyncClient>(&evb, addr);
  }

  // For inmemory transport
  auto handler = std::make_shared<ChatRoomServiceHandler>();
  ServerConfigsMock serverConfigs;
  if (FLAGS_transport == "inmemory") {
    client =
        newInMemoryClient<ChatRoomServiceAsyncClient, ChatRoomServiceHandler>(
            handler, serverConfigs);
  }

  // Prepare thrift request
  example::chatroom::SendMessageRequest sendRequest;
  sendRequest.message = "This is an example!";
  sendRequest.sender = getenv("USER");

  // Prepare thrift response
  example::chatroom::GetMessagesRequest getRequest;
  example::chatroom::GetMessagesResponse response;

  // Send and receive a message
  try {
    client->sync_sendMessage(sendRequest);
    client->sync_getMessages(response, getRequest);

    // Print all the messages so far
    for (auto& messagesList : response.messages) {
      LOG(INFO) << "Message: " << messagesList.message
                << " Sender: " << messagesList.sender;
    }
  } catch (apache::thrift::transport::TTransportException& ex) {
    LOG(ERROR) << "Request failed " << ex.what();
  } catch (example::chatroom::Exception& ex) {
    LOG(ERROR) << "Request failed " << ex.what();
  }

  return 0;
}
