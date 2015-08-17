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
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include "thrift/tutorial/cpp/async/fetcher/gen-cpp2/Fetcher.h"

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::tutorial::fetcher;

void usage(ostream& os, const char* progname) {
  os << "Usage: " << progname << " IP PORT PATH" << endl;
}

int main(int argc, char* argv[]) {
  // It would be nicer to also parse the address of the thrift server
  // from the command line arguments.
  const char* thrift_host = "127.0.0.1";
  unsigned short thrift_port = 12345;

  // There should be exactly two arguments: the HTTP server's IP,
  // and the path to fetch
  if (argc != 4) {
    usage(cerr, argv[0]);
    return 1;
  }
  const char* http_ip = argv[1];
  const char* http_port = argv[2];
  const char* http_path = argv[3];

  // Create the FetcherClient
  EventBase eb;
  auto client = make_unique<FetcherAsyncClient>(
      HeaderClientChannel::newChannel(
        async::TAsyncSocket::newSocket(
          &eb, {thrift_host, thrift_port})));

  // TODO: read IP and path from the command line
  cout << "fetchHttp(\"" << http_ip << "\", \"" << http_path <<
    "\")..." << endl;
  string ret;
  try {
    FetchHttpRequest request;
    request.addr = http_ip;
    request.port = to<int32_t>(http_port);
    request.path = http_path;
    client->sync_fetchHttp(ret, request);
    cout << "  return:" << endl << endl << ret;
  } catch (HttpError& ex) {
    cout << "  HTTP error: \"" << ex.message << "\"" << endl;
  } catch (TException& ex) {
    cout << "  Thrift error: \"" << ex.what() << "\"" << endl;
  }

  return 0;
}
