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

#include "thrift/lib/cpp2/transport/inmemory/example/ChatRoomService.h"

#include <gflags/gflags.h>
#include <time.h>
#include <vector>

using namespace apache::thrift;
using namespace facebook::tutorials::thrift::chatroomservice;

DEFINE_int32(max_messages_per_get, 1000, "Maximum number of messages to fetch");

ChatRoomServiceHandler::ChatRoomServiceHandler() : messageBuffer_() {}

void ChatRoomServiceHandler::getMessages(
    ChatRoomServiceGetMessagesResponse& resp,
    std::unique_ptr<ChatRoomServiceGetMessagesRequest> req) {
  int64_t idx = 0;
  if (req->__isset.token) {
    idx = req->token.index;
  }

  size_t i = 0;
  messageBuffer_.withWLock([&](auto& messageBuffer) {
    int32_t count = 0;
    for (i = idx;
         i < messageBuffer.size() && count < FLAGS_max_messages_per_get;
         ++i, ++count) {
      resp.messages.push_back(messageBuffer[i]);
    }
  });

  ChatRoomServiceIndexToken token;
  token.index = i;
  resp.token = token;
}

void ChatRoomServiceHandler::sendMessage(
    std::unique_ptr<ChatRoomServiceSendMessageRequest> req) {
  ChatRoomServiceMessage msg;
  msg.message = req->message;
  msg.sender = req->sender;

  // Avoid using the actual time of the day in unit tests.
  msg.timestamp = (int64_t)time(nullptr);

  messageBuffer_.withWLock(
      [&](auto& messageBuffer) { messageBuffer.push_back(msg); });
}
