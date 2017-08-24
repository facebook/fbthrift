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
//
// @author Eddie Elizondo (eelizondo@fb.com)
// @author Sriram Sankar (sankar@fb.com)

#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HTTPClientChannel.h>
#include "thrift/lib/cpp2/transport/http2/example/if/gen-cpp2/ChatRoomService.h"

using namespace facebook::tutorials::thrift::chatroomservice;

DEFINE_int32(port, 7777, "Port for the ChatRoomService");

int main(int /*argc*/, char** argv) {
  google::InitGoogleLogging(argv[0]);

  folly::EventBase eventBase;

  try {
    // Create a client to the service
    apache::thrift::async::TAsyncTransport::UniquePtr transport(
        new apache::thrift::async::TAsyncSocket(&eventBase, "::1", FLAGS_port));
    auto channel = apache::thrift::HTTPClientChannel::newHTTP2Channel(
        std::move(transport));
    channel->setHTTPHost("localhost6");
    channel->setHTTPUrl("/");
    channel->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);
    channel->setTimeout(5000);
    auto client =
        std::make_unique<ChatRoomServiceAsyncClient>(std::move(channel));

    // Send a message
    ChatRoomServiceSendMessageRequest sendRequest;
    sendRequest.message = "Tutorial!";
    sendRequest.sender = getenv("USER");
    client->sync_sendMessage(sendRequest);

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
