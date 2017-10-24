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

#include <folly/init/Init.h>
#include <glog/logging.h>
#include <thrift/lib/cpp2/server/BaseThriftServer.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/core/testutil/ServerConfigsMock.h>
#include <thrift/lib/cpp2/transport/inmemory/InMemoryConnection.h>
#include <thrift/lib/cpp2/transport/inmemory/example/ChatRoomService.h>
#include <thrift/lib/cpp2/transport/inmemory/example/if/gen-cpp2/ChatRoomService.h>
#include <memory>

using apache::thrift::InMemoryConnection;
using apache::thrift::ThriftClient;
using apache::thrift::ThriftServerAsyncProcessorFactory;
using facebook::tutorials::thrift::chatroomservice::ChatRoomServiceAsyncClient;
using facebook::tutorials::thrift::chatroomservice::ChatRoomServiceException;
using facebook::tutorials::thrift::chatroomservice::
    ChatRoomServiceGetMessagesRequest;
using facebook::tutorials::thrift::chatroomservice::
    ChatRoomServiceGetMessagesResponse;
using facebook::tutorials::thrift::chatroomservice::ChatRoomServiceHandler;
using facebook::tutorials::thrift::chatroomservice::
    ChatRoomServiceSendMessageRequest;

int main(int argc, char** argv) {
  folly::init(&argc, &argv);
  try {
    auto handler = std::make_shared<ChatRoomServiceHandler>();
    auto pFac = std::make_shared<
        ThriftServerAsyncProcessorFactory<ChatRoomServiceHandler>>(handler);
    apache::thrift::server::ServerConfigsMock serverConfigs;
    auto connection = std::make_shared<InMemoryConnection>(pFac, serverConfigs);
    auto thriftClient = ThriftClient::Ptr(new ThriftClient(connection));
    thriftClient->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);
    auto chatRoomClient =
        std::make_unique<ChatRoomServiceAsyncClient>(std::move(thriftClient));

    // Send a message
    ChatRoomServiceSendMessageRequest sendRequest;
    sendRequest.message = "Tutorial!";
    sendRequest.sender = getenv("USER");
    chatRoomClient->sync_sendMessage(sendRequest);

    // Get all the messages
    ChatRoomServiceGetMessagesRequest getRequest;
    ChatRoomServiceGetMessagesResponse response;
    chatRoomClient->sync_getMessages(response, getRequest);

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
