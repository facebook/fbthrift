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

#include "thrift/lib/cpp/ClientUtil.h"

#include "thrift/tutorial/cpp/async/fetcher/gen-cpp/Fetcher.h"

using std::shared_ptr;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace tutorial::async::fetcher;

void usage(std::ostream& os, const char* progname) {
  os << "Usage: " << progname << " IP PATH" << std::endl;
}

int main(int argc, char* argv[]) {
  // It would be nicer to also parse the address of the thrift server
  // from the command line arguments.
  const char* thrift_host = "127.0.0.1";
  unsigned short thrift_port = 12345;

  // There should be exactly two arguments: the HTTP server's IP,
  // and the path to fetch
  if (argc != 3) {
    usage(std::cerr, argv[0]);
    return 1;
  }
  const char* http_ip = argv[1];
  const char* http_path = argv[2];

  // Create the FetcherClient
  typedef FetcherClientT< TBinaryProtocolT<TBufferBase> > FetcherClient;
  shared_ptr<FetcherClient> client =
    util::createClientPtr<FetcherClient>(thrift_host, thrift_port);

  // TODO: read IP and path from the command line
  std::cout << "fetchHttp(\"" << http_ip << "\", \"" << http_path <<
    "\")..." << std::endl;
  std::string ret;
  try {
    client->fetchHttp(ret, http_ip, http_path);
    std::cout << "  return:" << std::endl << std::endl << ret;
  } catch (HttpError& ex) {
    std::cout << "  HTTP error: \"" << ex.message << "\"" << std::endl;
  } catch (TException& ex) {
    std::cout << "  Thrift error: \"" << ex.what() << "\"" << std::endl;
  }

  return 0;
}
