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

#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <thrift/tutorial/cpp/async/sort/util.h>
#include <thrift/tutorial/cpp/async/sort/gen-cpp2/Sorter.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::tutorial::sort;

int main(int argc, char* argv[]) {
  // Parse the arguments
  string host = "127.0.0.1";
  uint16_t port = 12345;
  if (argc == 2) {
    if (util_parse_host_port(argv[1], &host, &port) != 0) {
      cerr << "invalid address \"" << argv[1] << "\"" << endl;
      return 1;
    }
  }
  else if (argc > 2) {
    cerr << "trailing arguments" << endl;
    return 1;
  }

  // Read the list of integers to sort.
  cout << "Reading list of integers from stdin..." << endl;
  vector<int32_t> values;
  while (true) {
    int32_t i;
    cin >> i;
    if (cin.eof()) {
      break;
    }
    if (cin.fail()) {
      cerr << "error: input must be integers" << endl;
      return 1;
    }

    values.push_back(i);
  }

  try {
    cout << "Connecting to " << host << ":" << port << "..." << endl;
    EventBase eb;
    auto client = std::make_unique<SorterAsyncClient>(
        HeaderClientChannel::newChannel(
          async::TAsyncSocket::newSocket(
            &eb, {host, port})));

    // Now make the call to the server.
    //
    // client->sort() will block, not returning until it has sent the request
    // to the server and then received the response.  (We could have created a
    // SorterCobClient instead of a plain SorterClient if we wanted to perform
    // the operations in a non-blocking fashion.)
    vector<int32_t> sorted;

    try {
      client->sync_sort(sorted, values);
    } catch (SortError& ex) {
      cerr << "SortError: " << ex.msg << endl;
      return 2;
    }

    // Print the results.
    for (auto v : sorted) {
      cout << v << endl;
    }
  } catch (TException& ex) {
    cerr << "TException: " << ex.what() << endl;
    return 2;
  }

  return 0;
}
