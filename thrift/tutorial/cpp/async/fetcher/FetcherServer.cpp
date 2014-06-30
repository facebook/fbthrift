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

#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/async/TEventServer.h>

#include "thrift/tutorial/cpp/async/fetcher/FetcherHandler.h"

using std::shared_ptr;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace tutorial::async::fetcher;

void usage(std::ostream& os, const char* progname) {
  os << "Usage: " << progname << " [PORT]" << std::endl;
}

int main(int argc, char* argv[]) {
  // If an argument was supplied, it is the port number to listen on
  unsigned short port = 12345;
  if (argc == 2) {
    char* endptr;
    long val = strtol(argv[1], &endptr, 0);
    if (endptr == argv[1] || *endptr != '\0') {
      std::cerr << "error: port number must be an integer" << std::endl;
      return 1;
    }
    if (val < 0 || val > 0xffff) {
      std::cerr << "error: illegal port number " << val << std::endl;
      return 1;
    }
    port = val;
  } else if (argc > 2) {
    usage(std::cerr, argv[0]);
    std::cerr << "error: too many arguments" << std::endl;
    return 1;
  }

  // Create the handler, processor, and server
  shared_ptr<FetcherHandler> handler(new FetcherHandler());
  shared_ptr<TAsyncProcessor> processor(new FetcherAsyncProcessor(handler));
  shared_ptr<TProtocolFactory> proto_factory(
      new TBinaryProtocolFactoryT<TBufferBase>());
  TEventServer server(processor, proto_factory, port);

  // Give the handler a pointer to the TEventServer, so it will be able
  // to get the correct TEventBase to use.
  handler->setServer(&server);

  // Serve requests
  std::cout << "Serving requests on port " << port << "..." << std::endl;
  server.serve();

  return 0;
}
