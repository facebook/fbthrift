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

#include <folly/init/init.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>
#include "thrift/lib/cpp2/transport/http2/example/if/gen-cpp2/ChatRoomService.h"

using namespace apache::thrift;
using namespace facebook::tutorials::thrift::chatroomservice;

DEFINE_string(host, "localhost", "host to connect to");
DEFINE_int32(port, 7777, "Port for the ChatRoomService");

// ConnectionManager depends on this flag.
DECLARE_string(transport);

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  folly::ScopedEventBaseThread workerThread("ClientRnR::workerThread");

  FLAGS_transport = "rsocket";

  try {
    auto mgr = ConnectionManager::getInstance();
    auto connection = mgr->getConnection(FLAGS_host, FLAGS_port);
    auto channel = ThriftClient::Ptr(
        new ThriftClient(connection, workerThread.getEventBase()));
    channel->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);

    auto client =
        std::make_unique<ChatRoomServiceAsyncClient>(std::move(channel));

    // Send a message
    ChatRoomServiceSendMessageRequest sendRequest;
    sendRequest.message = "Tutorial!";
    sendRequest.sender = getenv("USER");
    client->sync_sendMessage(sendRequest);

    auto future = client->future_sendMessage(sendRequest);
    future.wait();

    // Get all the messages
    ChatRoomServiceGetMessagesRequest getRequest;
    ChatRoomServiceGetMessagesResponse response;
    client->sync_getMessages(response, getRequest);

    // Print all the messages so far
    for (auto& messagesList : response.messages) {
      LOG(INFO) << "Message: " << messagesList.message
                << " Sender: " << messagesList.sender;
    }
  } catch (apache::thrift::transport::TTransportException& ex) {
    LOG(ERROR) << "Request failed: " << ex.what();
  } catch (ChatRoomServiceException& ex) {
    LOG(ERROR) << "Request failed: " << ex.what();
  }

  return 0;
}
