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

#pragma once

#include <folly/Synchronized.h>
#include <vector>

#include "thrift/lib/cpp2/transport/http2/example/if/gen-cpp2/ChatRoomService.h"

namespace facebook {
namespace tutorials {
namespace thrift {
namespace chatroomservice {

class ChatRoomServiceHandler : virtual public ChatRoomServiceSvIf {
 public:
  ChatRoomServiceHandler();

  // Implement this constructor when you want all messages to have
  // the same fixed timestamp for testing purposes.
  explicit ChatRoomServiceHandler(int64_t /*currentTime*/)
      : ChatRoomServiceHandler() {}

  // Implement this constructor when you want to use a callback that
  // returns a timestamp for testing purposes.
  explicit ChatRoomServiceHandler(std::function<int64_t()> /*timeFn*/)
      : ChatRoomServiceHandler() {}

  void getMessages(
      ChatRoomServiceGetMessagesResponse& resp,
      std::unique_ptr<ChatRoomServiceGetMessagesRequest> req) override;

  void sendMessage(
      std::unique_ptr<ChatRoomServiceSendMessageRequest> req) override;

 private:
  folly::Synchronized<std::vector<ChatRoomServiceMessage>> messageBuffer_;
};
}
}
}
}
