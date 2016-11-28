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

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <folly/init/Init.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <thrift/tutorial/gen-cpp2/shared_types.h>
#include <thrift/tutorial/gen-cpp2/Calculator.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::tutorial;

int main(int argc, char** argv) {
  folly::init(&argc, &argv, true);

  EventBase eb;
  auto client = folly::make_unique<CalculatorAsyncClient>(
      HeaderClientChannel::newChannel(
        async::TAsyncSocket::newSocket(
          &eb, {"localhost", 9090})));

  try {

    client->sync_ping();
    printf("ping()\n");

    int32_t sum = client->sync_add(1,1);
    printf("1+1=%d\n", sum);

    Work work;
    work.op = Operation::DIVIDE;
    work.num1 = 1;
    work.num2 = 0;

    try {
      int32_t quotient = client->sync_calculate(1, work);
      printf("Whoa? We can divide by zero!\n");
    } catch (InvalidOperation &io) {
      printf("InvalidOperation: %s\n", io.why.c_str());
    }

    work.op = Operation::SUBTRACT;
    work.num1 = 15;
    work.num2 = 10;
    int32_t diff = client->sync_calculate(1, work);
    printf("15-10=%d\n", diff);

    // Note that C++ uses return by reference for complex types to avoid
    // costly copy construction
    SharedStruct ss;
    client->sync_getStruct(ss, 1);
    printf("Check log: %s\n", ss.value.c_str());

  } catch (TException &tx) {
    printf("ERROR: %s\n", tx.what());
  }

}
