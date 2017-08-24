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

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thrift/lib/cpp2/transport/http2/server/H2ThriftServer.h>

#include "thrift/lib/cpp2/transport/http2/example/ChatRoomService.h"

DEFINE_int32(port, 7777, "Port for the thrift server");

using namespace apache::thrift;
using namespace facebook::tutorials::thrift::chatroomservice;

int main(int /*argc*/, char** argv) {
  google::InitGoogleLogging(argv[0]);

  auto handler = std::make_shared<ChatRoomServiceHandler>();
  auto cpp2PFac = std::make_shared<
      ThriftServerAsyncProcessorFactory<ChatRoomServiceHandler>>(handler);

  auto server = std::make_shared<H2ThriftServer>();
  server->setPort(FLAGS_port);
  server->setProcessorFactory(cpp2PFac);

  LOG(INFO) << "ChatRoomService running on port: " << FLAGS_port;

  server->serve();

  return 0;
}
