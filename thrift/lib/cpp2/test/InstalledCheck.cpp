/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>

#include <folly/io/async/EventBase.h>

#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/test/util/TestInterface.h>
#include <thrift/lib/cpp2/test/util/TestThriftServerFactory.h>
#include <thrift/lib/cpp2/util/ScopedServerThread.h>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using apache::thrift::test::cpp2::TestServiceAsyncClient;

int SyncClientTest() {
  apache::thrift::TestThriftServerFactory<TestInterface> factory;
  ScopedServerThread sst(factory.create());
  auto port = sst.getAddress()->getPort();

  folly::EventBase base;

  auto socket = folly::AsyncSocket::newSocket(&base, "127.0.0.1", port);

  TestServiceAsyncClient client(
      HeaderClientChannel::newChannel(std::move(socket)));

  std::string response;
  client.sync_sendResponse(response, 64);
  assert(response == "test64");
  return 0;
}

int main(int argc, char** argv) {
  return SyncClientTest();
}
