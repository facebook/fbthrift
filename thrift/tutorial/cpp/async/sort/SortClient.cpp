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
#include "thrift/lib/cpp/transport/TSocket.h"
#include "thrift/lib/cpp/ClientUtil.h"

#include "thrift/tutorial/cpp/async/sort/util.h"
#include "thrift/tutorial/cpp/async/sort/gen-cpp/Sorter.h"

using std::shared_ptr;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace tutorial::sort;

int main(int argc, char* argv[]) {
  // Parse the arguments
  std::string host = "127.0.0.1";
  uint16_t port = 12345;
  if (argc == 2) {
    if (util_parse_host_port(argv[1], &host, &port) != 0) {
      std::cerr << "invalid address \"" << argv[1] << "\"" << std::endl;
      return 1;
    }
  }
  else if (argc > 2) {
    std::cerr << "trailing arguments" << std::endl;
    return 1;
  }

  // Read the list of integers to sort.
  std::cout << "Reading list of integers from stdin..." << std::endl;
  typedef std::vector<int32_t> IntVector;
  IntVector values;
  while (true) {
    int32_t i;
    std::cin >> i;
    if (std::cin.eof()) {
      break;
    }
    if (std::cin.fail()) {
      std::cerr << "error: input must be integers" << std::endl;
      return 1;
    }

    values.push_back(i);
  }

  try {
    std::cout << "Connecting to " << host << ":" << port << "..." << std::endl;
    typedef SorterClientT< TBinaryProtocolT<TBufferBase> > SorterClient;
    shared_ptr<SorterClient> client =
      util::createClientPtr<SorterClient>(host, port);

    // Now make the call to the server.
    //
    // client->sort() will block, not returning until it has sent the request
    // to the server and then received the response.  (We could have created a
    // SorterCobClient instead of a plain SorterClient if we wanted to perform
    // the operations in a non-blocking fashion.)
    IntVector sorted;

    try {
      client->sort(sorted, values);
    } catch (SortError& ex) {
      std::cerr << "SortError: " << ex.msg << std::endl;
      return 2;
    }

    // Print the results.
    for (IntVector::const_iterator it = sorted.begin();
         it != sorted.end(); ++it) {
      std::cout << *it << std::endl;
    }
  } catch (TException& ex) {
    std::cerr << "TException: " << ex.what() << std::endl;
    return 2;
  }

  return 0;
}
