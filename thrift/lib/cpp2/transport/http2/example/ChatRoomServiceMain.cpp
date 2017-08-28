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

#include "thrift/lib/cpp2/transport/http2/example/ChatRoomService.h"

#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.h>
#include <thrift/lib/cpp2/transport/http2/server/ThriftRequestHandlerFactory.h>
#include <proxygen/httpserver/HTTPServerOptions.h>

DEFINE_int32(port, 7777, "Port for the thrift server");

using namespace apache::thrift;
using namespace facebook::tutorials::thrift::chatroomservice;
using proxygen::RequestHandlerChain;

std::unique_ptr<apache::thrift::HTTP2RoutingHandler> getHTTP2RoutingHandler(
    std::shared_ptr<ThriftServer> server) {
  auto h2_options = std::make_unique<proxygen::HTTPServerOptions>();
  h2_options->threads = static_cast<size_t>(server->getNumIOWorkerThreads());
  h2_options->idleTimeout = server->getIdleTimeout();
  h2_options->shutdownOn = {SIGINT, SIGTERM};
  h2_options->handlerFactories =
      RequestHandlerChain()
        .addThen<ThriftRequestHandlerFactory>(server->getThriftProcessor())
        .build();

  return std::make_unique<apache::thrift::HTTP2RoutingHandler>(
      std::move(h2_options));
}

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  auto handler = std::make_shared<ChatRoomServiceHandler>();
  auto cpp2PFac = std::make_shared<
      ThriftServerAsyncProcessorFactory<ChatRoomServiceHandler>>(handler);

  auto server = std::make_shared<ThriftServer>();
  server->setPort(FLAGS_port);
  server->setProcessorFactory(cpp2PFac);

  auto http2_transport_handler = getHTTP2RoutingHandler(server);
  server->addRoutingHandler(http2_transport_handler.get());

  LOG(INFO) << "ChatRoomService running on port: " << FLAGS_port;

  server->serve();

  return 0;
}
