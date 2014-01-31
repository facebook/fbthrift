/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <iostream>
#include <memory>

#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/lib/cpp/async/TEventServer.h"

#include "thrift/tutorial/cpp/async/sort/SortServerHandler.h"

using std::shared_ptr;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace tutorial::sort;

int main(int argc, char* argv[]) {
  // Parse the arguments
  uint16_t port = 12345;
  if (argc == 2) {
    if (util_parse_port(argv[1], &port) != 0) {
      std::cerr << "invalid port \"" << argv[1] << "\"" << std::endl;
      return 1;
    }
  }
  else if (argc > 2) {
    std::cerr << "trailing arguments" << std::endl;
    return 1;
  }

  // Create the handler, processor, and server
  shared_ptr<SortServerHandler> handler(new SortServerHandler());
  shared_ptr<TAsyncProcessor> processor(new SorterAsyncProcessor(handler));
  shared_ptr<TProtocolFactory> proto_factory(
      new TBinaryProtocolFactoryT<TBufferBase>());
  TEventServer server(processor, proto_factory, port);

  // Give the handler the pointer to the server, so it can get the TEventBase
  handler->setServer(&server);

  // server.serve() does all the work
  std::cout << "Serving requests on port " << port << "..." << std::endl;
  server.serve();

  return 0;
}
