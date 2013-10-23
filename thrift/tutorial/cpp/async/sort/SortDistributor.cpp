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
#include <getopt.h>

#include <iostream>
#include <memory>

#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/lib/cpp/async/TEventServer.h"

#include "thrift/tutorial/cpp/async/sort/SortDistributorHandler.h"

using std::shared_ptr;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace tutorial::sort;

void usage(std::ostream &o, const char* progname) {
  o << "Usage: " << progname <<
    " [-p PORT] SERVER1[:PORT] SERVER2[:PORT] [...]" <<
    std::endl;
}

int main(int argc, char* argv[]) {
  uint16_t port = 12345;

  while (true) {
    int c = getopt(argc, argv, "p:");
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'p':
        // Parse the port to listen on
        if (util_parse_port(optarg, &port) != 0) {
          std::cerr << "error: invalid port \"" << argv[1] << "\"" <<
            std::endl;
          return 1;
        }
        break;
      case '?':
      default:
        usage(std::cerr, argv[0]);
        return 1;
    }
  }

  // The remaining arguments are the backend servers
  // There must be at least 2 backend servers
  if (optind + 2 > argc) {
    usage(std::cerr, argv[0]);
    std::cerr << "error: at least 2 backend servers must be specified" <<
      std::endl;
    return 1;
  }

  // Create the handler, as we add the server arguments
  // to the handler as we parse them.
  shared_ptr<SortDistributorHandler> handler(new SortDistributorHandler());

  // Remaining arguments indicate the sort servers
  // where we should distribute requests
  for (int n = optind; n < argc; ++n) {
    // Parse the hostname:port argument
    std::string host = "127.0.0.1";
    uint16_t port = 12345;
    if (util_parse_host_port(argv[n], &host, &port) != 0) {
      std::cerr << "error: invalid address \"" << argv[n] << "\"" << std::endl;
      return 1;
    }

    // Resolve hostnames to IP addresses
    //
    // Re-resolving the names for each request would be inefficient.
    // Furthermore, we don't have a library to asynchronously resolve
    // hostnames--this is currently a blocking operation.  The small downside
    // is that if we run for a very long time, we won't ever re-resolve the
    // hostnames in case they are ever changed in DNS.
    std::string ip;
    if (util_resolve_host(host, &ip) != 0) {
      std::cerr << "error: failed to resolve hostname \"" << host << "\"" <<
        std::endl;
      return 1;
    }

    std::cout << "Backend server: " << ip << ":" << port << std::endl;

    // Tell the handler about this server
    handler->addSortServer(ip, port);
  }

  // Done with argument parsing
  // Now create the processor and server
  shared_ptr<TAsyncProcessor> processor(new SorterAsyncProcessor(handler));
  shared_ptr<TProtocolFactory> proto_factory(
      new TBinaryProtocolFactoryT<TBufferBase>());
  TEventServer server(processor, proto_factory, port);

  // Give the handler the pointer to the server,
  // so it can get the TEventBase to use
  handler->setServer(&server);

  // server.serve() does all the work
  std::cout << "Serving requests on port " << port << "..." << std::endl;
  server.serve();

  return 0;
}
