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

#include "thrift/lib/cpp2/example/ChatRoomService.h"
#include "thrift/lib/cpp2/example/EchoService.h"

#include <folly/init/Init.h>
#include <glog/logging.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>

int main(int argc, char** argv) {
  FLAGS_logtostderr = 1;
  folly::init(&argc, &argv);

  auto chatroom_handler =
      std::make_shared<tutorials::chatroom::ChatRoomServiceHandler>();
  auto chatroom_server = std::make_shared<apache::thrift::ThriftServer>();
  chatroom_server->setPort(7777);
  chatroom_server->setInterface(chatroom_handler);

  LOG(INFO) << "ChatRoom Server running on port: " << 7777;
  std::thread t([&] {
    chatroom_server->serve();
  });

  auto echo_handler =
      std::make_shared<tutorials::chatroom::EchoHandler>();
  auto echo_server = std::make_shared<apache::thrift::ThriftServer>();
  echo_server->setPort(7778);
  echo_server->setInterface(echo_handler);

  LOG(INFO) << "Echo Server running on port: " << 7778;

  echo_server->serve();

  return 0;
}
