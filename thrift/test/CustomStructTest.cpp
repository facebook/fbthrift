/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <unordered_map>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "thrift/test/gen-cpp2/CustomStruct.h"

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;
using namespace thrift::test;

Container createContainer() {
  Container c;

  c.myStruct.data_ = "Goodnight Moon";
  c.myUnion1.data_ = "Llama llama";
  c.myUnion2.data_ = "36";

  c.myStructList.emplace_back("Ship");
  c.myStructList.emplace_back("The Cat in the Hat");

  c.myUnionList.emplace_back("4601");
  c.myUnionList.emplace_back("-1");
  c.myUnionList.emplace_back("Alice's Adventures in Wonderland");

  c.myStructMap.emplace(10, MyCustomStruct("When the Elephant Walks"));
  c.myUnionMap.emplace(20, c.myUnion2);

  c.myRevStructMap[c.myStruct] = "Margaret Wise Brown";
  c.myRevUnionMap[c.myUnion1] = "Anna Dewdney";

  return c;
}

class CustomStructHandler : public CustomStructSvIf {
 public:
  void echoStruct(MyCustomStruct& out, const MyCustomStruct& in) {
    out = in;
  }
  void echoUnion(MyCustomUnion& out, const MyCustomUnion& in) {
    out = in;
  }
};


std::shared_ptr<ThriftServer> getServer() {
  auto server = std::make_shared<ThriftServer>();
  auto threadFactory = std::make_shared<PosixThreadFactory>();
  std::shared_ptr<apache::thrift::concurrency::ThreadManager>
    threadManager(
        apache::thrift::concurrency::ThreadManager::newSimpleThreadManager(1,
          5,
          false,
          2));
  threadManager->threadFactory(threadFactory);
  threadManager->start();
  server->setThreadManager(threadManager);
  server->setPort(0);
  server->setInterface(std::unique_ptr<CustomStructSvIf>(
      new CustomStructHandler));
  return server;
}


TEST(CustomStructs, RoundTripContainer) {
  Container expected = createContainer();

  BinaryProtocolWriter protWriter;
  size_t bufSize = Cpp2Ops<Container>::serializedSize(&protWriter, &expected);
  folly::IOBufQueue queue;
  protWriter.setOutput(&queue, bufSize);
  Cpp2Ops<Container>::write(&protWriter, &expected);

  auto buf = queue.move();
  BinaryProtocolReader protReader;
  protReader.setInput(buf.get());
  Container actual;
  Cpp2Ops<Container>::read(&protReader, &actual);
  EXPECT_EQ(expected, actual);
}


TEST(CustomStructs, RoundTripEmptyContainer) {
  Container expected;

  BinaryProtocolWriter protWriter;
  size_t bufSize = Cpp2Ops<Container>::serializedSize(&protWriter, &expected);
  folly::IOBufQueue queue;
  protWriter.setOutput(&queue, bufSize);
  Cpp2Ops<Container>::write(&protWriter, &expected);

  auto buf = queue.move();
  BinaryProtocolReader protReader;
  protReader.setInput(buf.get());
  Container actual;
  Cpp2Ops<Container>::read(&protReader, &actual);
  EXPECT_EQ(expected, actual);
}


TEST(CustomStructs, SerializeOverHandler) {
  ScopedServerThread sst(getServer());
  auto port = sst.getAddress()->getPort();

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  CustomStructAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  {
    MyCustomStruct expected("Goodnight Moon");
    MyCustomStruct actual("UNKNOWN");
    client.sync_echoStruct(actual, expected);
    EXPECT_EQ(expected, actual);
  }

  {
    MyCustomUnion expected("Goodnight Moon");
    MyCustomUnion actual;
    client.sync_echoUnion(actual, expected);
    EXPECT_EQ(expected, actual);
  }

  {
    MyCustomUnion expected("-4601");
    MyCustomUnion actual;
    client.sync_echoUnion(actual, expected);
    EXPECT_EQ(expected, actual);
  }
}
