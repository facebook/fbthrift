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

#include <memory>
#include <stdio.h>
#include <iostream>

#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/async/TSyncToAsyncProcessor.h>
#include <thrift/lib/cpp/concurrency/FunctionRunner.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/TCompactProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/server/example/TThreadedServer.h>
#include <thrift/lib/cpp/server/example/TThreadPoolServer.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/server/TConnectionContext.h>
#include <thrift/lib/cpp/server/example/TSimpleServer.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/transport/THttpClient.h>
#include <thrift/lib/cpp/transport/TTransportUtils.h>
#include <thrift/lib/cpp/transport/TSocket.h>
#include <thrift/lib/cpp/transport/TSSLSocket.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/util/ServerCreatorBase.h>
#include <thrift/lib/cpp/util/TEventServerCreator.h>
#include <thrift/lib/cpp/util/example/TSimpleServerCreator.h>
#include <thrift/lib/cpp/util/TThreadedServerCreator.h>
#include <thrift/lib/cpp/util/example/TThreadPoolServerCreator.h>

#include <thrift/test/gen-cpp/Service.h>

#include <folly/portability/SysTime.h>
#include <folly/portability/Unistd.h>

#include <gtest/gtest.h>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;
using namespace test::stress;

static const string testIdentity = "me";

class TestHandler : public ServiceIf {
public:
  TestHandler() {}

  void echoVoid() override {}

  int8_t echoByte(int8_t byte) override { return byte; }

  int32_t echoI32(int32_t e) override { return e; }

  int64_t echoI64(int64_t e) override { return e; }

  void echoString(string& out, const string& e) override { out = e; }

  void echoList(vector<int8_t>& out, const vector<int8_t>& e) override {
    out = e;
  }

  void echoSet(set<int8_t>& out, const set<int8_t>& e) override { out = e; }

  void echoMap(map<int8_t, int8_t>& out,
               const map<int8_t, int8_t>& e) override {
    out = e;
  }
};

class HeaderEventHandler : public TProcessorEventHandler {
public:
  HeaderEventHandler()
    : iprot_(),
      oprot_() {}

  void* getContext(const char* fn_name,
                   TConnectionContext* connectionContext) override {
    iprot_ = dynamic_pointer_cast<THeaderProtocol>
      (connectionContext->getInputProtocol());
    oprot_ = dynamic_pointer_cast<THeaderProtocol>
      (connectionContext->getOutputProtocol());
    // we don't need to return any context
    return nullptr;
  }

  /**
   * Before writing, get the input headers and replay them to the output
   * headers
   */
  void preWrite(void* ctx, const char* fn_name) override {
    if (iprot_.get() != nullptr && oprot_.get() != nullptr) {
      auto headers = iprot_->getHeaders();
      oprot_->setHeaders(headers);
      oprot_->setIdentity(testIdentity);
    }
  }

private:
  shared_ptr<THeaderProtocol> iprot_;
  shared_ptr<THeaderProtocol> oprot_;
};

enum ClientType {
  CLIENT_TYPE_UNFRAMED = 0,
  CLIENT_TYPE_FRAMED = 1,
  CLIENT_TYPE_HEADER = 2,
  CLIENT_TYPE_HTTP = 3,
  CLIENT_TYPE_FRAMED_COMPACT = 4,
};

enum ServerType {
  SERVER_TYPE_SIMPLE = 0,
  SERVER_TYPE_THREADED = 1,
  SERVER_TYPE_THREADPOOL = 2,
  SERVER_TYPE_EVENT = 5,
};

void runClient(ClientType clientType, ServerType sType, int port) {
  auto socket = make_shared<TSocket>("localhost", port);
  shared_ptr<TTransport> transport;
  shared_ptr<TProtocol> protocol;

  switch(clientType) {
    case CLIENT_TYPE_UNFRAMED:
      transport = make_shared<TBufferedTransport>(socket);
      protocol = make_shared<TBinaryProtocol>(transport);
      break;
    case CLIENT_TYPE_FRAMED:
      transport = make_shared<TFramedTransport>(socket);
      protocol = make_shared<TBinaryProtocol>(transport);
      break;
    case CLIENT_TYPE_HEADER:
      transport = make_shared<THeaderTransport>(socket);
      protocol = make_shared<THeaderProtocol>(transport);
      break;
    case CLIENT_TYPE_HTTP:
      transport = make_shared<THttpClient>("localhost", port, "/");
      protocol = make_shared<TBinaryProtocol>(transport);
      break;
    case CLIENT_TYPE_FRAMED_COMPACT:
      transport = make_shared<TFramedTransport>(socket);
      protocol = make_shared<TCompactProtocol>(transport);
      break;
  }

  ServiceClient testClient(protocol);

  transport->open();

  // set some headers and expect them back
  if (clientType == CLIENT_TYPE_HEADER) {
    auto hprotocol = static_pointer_cast<THeaderProtocol>(protocol);
    hprotocol->setHeader("my-test-header", "myvalue1");
    hprotocol->setHeader("my-other-header", "myvalue2");

    hprotocol->setPersistentHeader("my-persis-test-header", "myvalue1");

    // Check that an empty identity call works
    EXPECT_EQ(hprotocol->getPeerIdentity(), "");

    string testString = "test me now";
    string testOut;
    testClient.echoString(testOut, testString);
    // ensure that the function call worked
    EXPECT_EQ(testOut, testString);

    // Verify that the identity works
    EXPECT_EQ(hprotocol->getPeerIdentity(), testIdentity);

    // ensure that the write headers were cleared after issuing the command
    EXPECT_TRUE(hprotocol->isWriteHeadersEmpty());
    EXPECT_TRUE(hprotocol->getPersistentWriteHeaders().empty());

    auto headers = hprotocol->getHeaders();
    bool ok;
    EXPECT_TRUE(ok = (headers.find("my-test-header") != headers.end()));
    if (ok) {
      EXPECT_EQ(0, headers.find("my-test-header")->second.compare("myvalue1"));
    }
    EXPECT_TRUE(ok = (headers.find("my-other-header") != headers.end()));
    if (ok) {
      EXPECT_EQ(0, headers.find("my-other-header")->second.compare("myvalue2"));
    }

    // verify that the persistent header works
    testClient.echoString(testOut, testString);
    headers = hprotocol->getHeaders();
    // verify if the non-persistent header falls apart
    EXPECT_TRUE(ok = (headers.find("my-test-header") == headers.end()));
    EXPECT_TRUE(
        ok = (headers.find("my-persis-test-header") != headers.end())
    );
    if (ok) {
      EXPECT_EQ(
          0, headers.find("my-persis-test-header")->second.compare("myvalue1"));
    }

  }

  testClient.echoVoid();

  // test that headers were cleared after sending the last message
  if (clientType == CLIENT_TYPE_HEADER) {
    auto hprotocol = static_pointer_cast<THeaderProtocol>(protocol);
    ASSERT_TRUE(hprotocol->isWriteHeadersEmpty());
  }

  EXPECT_EQ(testClient.echoByte(5), 5);

  EXPECT_EQ(testClient.echoI32(5), 5);

  EXPECT_EQ(testClient.echoI64(5), 5);

  string testString = "test";
  string testOut;
  testClient.echoString(testOut, testString);
  EXPECT_EQ(testString, testOut);

  vector<int8_t> listtest, outList;
  listtest.push_back(5);
  testClient.echoList(outList, listtest);
  EXPECT_EQ(outList[0],5);

  set<int8_t> settest, outSet;
  settest.insert(5);
  testClient.echoSet(outSet, settest);
  EXPECT_EQ(outSet.count(5), 1);

  map<int8_t, int8_t> maptest, outMap;
  maptest[5] = 5;
  testClient.echoMap(outMap, maptest);
  EXPECT_EQ(outMap[5], 5);

  transport->close();
}

void runTestCase(ServerType sType, ClientType clientType) {
  bitset<CLIENT_TYPES_LEN> clientTypes;
  clientTypes[THRIFT_UNFRAMED_DEPRECATED] = 1;
  clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
  clientTypes[THRIFT_HTTP_SERVER_TYPE] = 1;
  clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;
  clientTypes[THRIFT_FRAMED_COMPACT] = 1;
  auto factory = make_shared<THeaderProtocolFactory>();
  factory->setClientTypes(clientTypes);

  auto protocolFactory = factory;
  auto testHandler = make_shared<TestHandler>();

  auto testProcessor = make_shared<ServiceProcessor>(testHandler);
  auto testAsyncProcessor = make_shared<TSyncToAsyncProcessor>(testProcessor);
  auto transportFactory =
    make_shared<TSingleTransportFactory<TBufferedTransportFactory>>();

  auto event_handler = make_shared<HeaderEventHandler>();
  // set the header-replying event handler
  testProcessor->addEventHandler(event_handler);
  testAsyncProcessor->addEventHandler(event_handler);

  int port = 0;

  shared_ptr<ServerCreatorBase> serverCreator;
  switch(sType) {
    case SERVER_TYPE_SIMPLE:
      // "Testing TSimpleServerCreator"
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      serverCreator = make_shared<TSimpleServerCreator>(
          testProcessor, port, false);
      #pragma GCC diagnostic pop
      break;
    case SERVER_TYPE_THREADED:
      serverCreator = make_shared<TThreadedServerCreator>(
          testProcessor, port, false);
      break;
    case SERVER_TYPE_THREADPOOL:
      // "Testing TThreadPoolServerCreator"
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      serverCreator = make_shared<TThreadPoolServerCreator>(
          testProcessor, port, false);
      #pragma GCC diagnostic pop
      break;
    case SERVER_TYPE_EVENT:
      serverCreator = make_shared<TEventServerCreator>(
          testAsyncProcessor, port);
      break;
  }
  serverCreator->setDuplexProtocolFactory(protocolFactory);

  auto server = serverCreator->createServer();
  ScopedServerThread serverThread(server);
  runClient(clientType, sType, serverThread.getAddress()->getPort());
}

// Listed individually to make it easy to see in the unit runner

TEST(THeaderTest, simpleServerUnframed) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_UNFRAMED);
}

TEST(THeaderTest, threadedServerUnframed) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_UNFRAMED);
}

TEST(THeaderTest, threadPoolServerUnframed) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_UNFRAMED);
}

TEST(THeaderTest, simpleServerFramed) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_FRAMED);
}

TEST(THeaderTest, threadedServerFramed) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_FRAMED);
}

TEST(THeaderTest, threadPoolServerFramed) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_FRAMED);
}

TEST(THeaderTest, eventServerFramed) {
  runTestCase(SERVER_TYPE_EVENT, CLIENT_TYPE_FRAMED);
}

TEST(THeaderTest, simpleServerHeader) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_HEADER);
}

TEST(THeaderTest, threadedServerHeader) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_HEADER);
}

TEST(THeaderTest, threadPoolServerHeader) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_HEADER);
}

TEST(THeaderTest, eventServerHeader) {
  runTestCase(SERVER_TYPE_EVENT, CLIENT_TYPE_HEADER);
}

TEST(THeaderTest, simpleServerHttp) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_HTTP);
}

TEST(THeaderTest, threadedServerHttp) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_HTTP);
}

TEST(THeaderTest, threadPoolServerHttp) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_HTTP);
}

TEST(THeaderTest, simpleServerCompactFramed) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_FRAMED_COMPACT);
}

TEST(THeaderTest, threadedServerCompactFramed) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_FRAMED_COMPACT);
}

TEST(THeaderTest, threadPoolServerCompactFramed) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_FRAMED_COMPACT);
}

TEST(THeaderTest, eventServerCompactFramed) {
  runTestCase(SERVER_TYPE_EVENT, CLIENT_TYPE_FRAMED_COMPACT);
}

TEST(THeaderTest, unframedBadRead) {
  auto buffer = make_shared<TMemoryBuffer>();
  auto transport = make_shared<THeaderTransport>(buffer);
  auto protocol = make_shared<THeaderProtocol>(transport);
  string name = "test";
  TMessageType messageType = T_CALL;
  int32_t seqId = 0;
  uint8_t buf1 = 0x80;
  uint8_t buf2 = 0x01;
  uint8_t buf3 = 0x00;
  uint8_t buf4 = 0x00;
  buffer->write(&buf1, 1);
  buffer->write(&buf2, 1);
  buffer->write(&buf3, 1);
  buffer->write(&buf4, 1);

  EXPECT_THROW(
      protocol->readMessageBegin(name, messageType, seqId),
      TTransportException);
}

TEST(THeaderTest, removeBadHeaderStringSize) {
  uint8_t badHeader[] = {
    0x00, 0x00, 0x00, 0x13, // Frame size is corrupted here
    0x0F, 0xFF, 0x00, 0x00, // THRIFT_HEADER_CLIENT_TYPE
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, // Header size
    0x00, // Proto ID
    0x00, // Num transforms
    0x01, // Info ID Key value
    0x01, // Num headers
    0xFF, 0xFF, 0xFF, 0xFF, // Malformed varint32 string size
    0x00 // String should go here
  };
  folly::IOBufQueue queue;
  queue.append(folly::IOBuf::wrapBuffer(badHeader, sizeof(badHeader)));
  // Try to remove the bad header
  THeader header;
  size_t needed;
  std::map<std::string, std::string> persistentHeaders;
  EXPECT_THROW(
    auto buf = header.removeHeader(&queue, needed, persistentHeaders),
    TTransportException
  );
}
