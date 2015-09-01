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

#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <thrift/tutorial/cpp/async/sort/SortServerHandler.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::tutorial::sort;

int main(int argc, char* argv[]) {
  // Parse the arguments
  uint16_t port = 12345;
  if (argc == 2) {
    if (util_parse_port(argv[1], &port) != 0) {
      cerr << "invalid port \"" << argv[1] << "\"" << endl;
      return 1;
    }
  }
  else if (argc > 2) {
    cerr << "trailing arguments" << endl;
    return 1;
  }

  auto handler = make_shared<SortServerHandler>();
  auto server = make_shared<ThriftServer>();
  server->setInterface(handler);
  server->setPort(port);

  // server.serve() does all the work
  cout << "Serving requests on port " << port << "..." << endl;
  server->serve();

  return 0;
}
