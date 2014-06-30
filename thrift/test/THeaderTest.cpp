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

#include <boost/test/unit_test.hpp>
#include <memory>
#include <stdio.h>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>

#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/async/TSyncToAsyncProcessor.h>
#include <thrift/lib/cpp/concurrency/FunctionRunner.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/TCompactProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/server/TNonblockingServer.h>
#include <thrift/lib/cpp/server/TThreadedServer.h>
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
#include <thrift/lib/cpp/util/TNonblockingServerCreator.h>
#include <thrift/lib/cpp/util/example/TSimpleServerCreator.h>
#include <thrift/lib/cpp/util/TThreadedServerCreator.h>
#include <thrift/lib/cpp/util/example/TThreadPoolServerCreator.h>

#include "thrift/test/gen-cpp/Service.h"

using std::string;
using std::make_pair;
using std::map;
using std::vector;
using std::set;
using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;
using namespace test::stress;
using std::cout;
using std::endl;

static const string testIdentity = "me";

class TProcessorWrapper : public TProcessor {
 public:
  explicit TProcessorWrapper(
      std::shared_ptr<apache::thrift::TDispatchProcessor> wrappedProcessor
  ) : wrappedProcessor_(wrappedProcessor) {}

  virtual bool process(std::shared_ptr<protocol::TProtocol> in,
                       std::shared_ptr<protocol::TProtocol> out,
                       TConnectionContext* connectionContext) {
    bool result;
    try {
      result = wrappedProcessor_->process(in, out, connectionContext);
    } catch (std::exception& ex) {
      BOOST_FAIL("Processor threw an exception");
      throw;
    }
    return result;
  }

 private:
  std::shared_ptr<apache::thrift::TDispatchProcessor> wrappedProcessor_;
};

class TestHandler : public ServiceIf {
public:
  TestHandler() {}

  void echoVoid() {
  }

  int8_t echoByte(int8_t byte) {
    return byte;
  }

  int32_t echoI32(int32_t e) {
    return e;
  }

  int64_t echoI64(int64_t e) {
    return e;
  }

  void echoString(string& out, const string& e) {
    out = e;
  }

  void echoList(vector<int8_t>& out, const vector<int8_t>& e) {
    out = e;
  }

  void echoSet(set<int8_t>& out, const set<int8_t>& e) {
    out = e;
  }

  void echoMap(map<int8_t, int8_t>& out, const map<int8_t, int8_t>& e) {
    out = e;
  }
};

class HeaderEventHandler : public TProcessorEventHandler {
public:
  HeaderEventHandler()
    : iprot_(),
      oprot_() {}

  void* getContext(const char* fn_name,
                   TConnectionContext* connectionContext) {
    iprot_ = std::dynamic_pointer_cast<THeaderProtocol>
      (connectionContext->getInputProtocol());
    oprot_ = std::dynamic_pointer_cast<THeaderProtocol>
      (connectionContext->getOutputProtocol());
    // we don't need to return any context
    return nullptr;
  }

  /**
   * Before writing, get the input headers and replay them to the output
   * headers
   */
  void preWrite(void* ctx, const char* fn_name) {
    if (iprot_.get() != nullptr && oprot_.get() != nullptr) {
      auto headers = iprot_->getHeaders();
      oprot_->getWriteHeaders() = headers;
      oprot_->setIdentity(testIdentity);
    }
  }

private:
  std::shared_ptr<THeaderProtocol> iprot_;
  std::shared_ptr<THeaderProtocol> oprot_;
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
  SERVER_TYPE_NONBLOCKING = 4,
  SERVER_TYPE_EVENT = 5,
};

void runClient(ClientType clientType, ServerType sType, int port) {
  std::shared_ptr<TSocket> socket(new TSocket("localhost", port));
  std::shared_ptr<TTransport> transport;
  std::shared_ptr<TProtocol> protocol;

  switch(clientType) {
    case CLIENT_TYPE_UNFRAMED:
      transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
      protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
      break;
    case CLIENT_TYPE_FRAMED:
      transport = std::shared_ptr<TTransport>(new TFramedTransport(socket));
      protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
      break;
    case CLIENT_TYPE_HEADER:
      transport = std::shared_ptr<TTransport>(new THeaderTransport(socket));
      protocol = std::shared_ptr<TProtocol>(new THeaderProtocol(transport));
      break;
    case CLIENT_TYPE_HTTP:
      transport = std::shared_ptr<TTransport>(new THttpClient("localhost",
                                                         port, "/"));
      protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
      break;
    case CLIENT_TYPE_FRAMED_COMPACT:
      transport = std::shared_ptr<TTransport>(new TFramedTransport(socket));
      protocol = std::shared_ptr<TProtocol>(new TCompactProtocol(transport));
      break;
  }

  ServiceClient testClient(protocol);

  try {
    transport->open();
  } catch (TTransportException& ttx) {
    BOOST_CHECK_EQUAL(true, false);
    return;
  }

  // set some headers and expect them back
  if (clientType == CLIENT_TYPE_HEADER) {
    std::shared_ptr<THeaderProtocol> hprotocol =
      std::static_pointer_cast<THeaderProtocol>(protocol);
    hprotocol->setHeader("my-test-header", "myvalue1");
    hprotocol->setHeader("my-other-header", "myvalue2");

    hprotocol->setPersistentHeader("my-persis-test-header", "myvalue1");

    // Check that an empty identity call works
    BOOST_CHECK_EQUAL(hprotocol->getPeerIdentity(), "");

    try {
      string testString = "test me now";
      string testOut;
      testClient.echoString(testOut, testString);
      // ensure that the function call worked
      BOOST_CHECK_EQUAL(testOut, testString);

      // Verify that the identity works
      BOOST_CHECK_EQUAL(hprotocol->getPeerIdentity(), testIdentity);

      // ensure that the write headers were cleared after issuing the command
      BOOST_CHECK(hprotocol->getWriteHeaders().empty());
      BOOST_CHECK(hprotocol->getPersistentWriteHeaders().empty());

      auto headers = hprotocol->getHeaders();
      bool ok;
      BOOST_CHECK(ok = (headers.find("my-test-header") != headers.end()));
      if (ok) {
        BOOST_CHECK(headers.find("my-test-header")->second.compare("myvalue1")
                    == 0);
      }
      BOOST_CHECK(ok = (headers.find("my-other-header") != headers.end()));
      if (ok) {
        BOOST_CHECK(headers.find("my-other-header")->second.compare("myvalue2")
                    == 0);
      }

      // verify that the persistent header works
      testClient.echoString(testOut, testString);
      headers = hprotocol->getHeaders();
      // verify if the non-persistent header falls apart
      BOOST_CHECK(ok = (headers.find("my-test-header") == headers.end()));
      BOOST_CHECK(
          ok = (headers.find("my-persis-test-header") != headers.end())
      );
      if (ok) {
        BOOST_CHECK(headers.find("my-persis-test-header")
            ->second.compare("myvalue1") == 0);
      }

    } catch (const TApplicationException& tax) {
      BOOST_CHECK_EQUAL(true, false);
    }
  }

  try {
    testClient.echoVoid();
  } catch (const TApplicationException& tax) {
    BOOST_CHECK_EQUAL(true, false);
  }

  // test that headers were cleared after sending the last message
  if (clientType == CLIENT_TYPE_HEADER) {
    std::shared_ptr<THeaderProtocol> hprotocol =
      std::static_pointer_cast<THeaderProtocol>(protocol);
    BOOST_ASSERT(hprotocol->getWriteHeaders().empty());
  }

  try {
    BOOST_CHECK_EQUAL(testClient.echoByte(5), 5);
  } catch (const TApplicationException& tax) {
    BOOST_CHECK_EQUAL(true, false);
  }

  try {
    BOOST_CHECK_EQUAL(testClient.echoI32(5), 5);
  } catch (const TApplicationException& tax) {
    BOOST_CHECK_EQUAL(true, false);
  }

  try {
    BOOST_CHECK_EQUAL(testClient.echoI64(5), 5);
  } catch (const TApplicationException& tax) {
    BOOST_CHECK_EQUAL(true, false);
  }

  try {
    string testString = "test";
    string testOut;
    testClient.echoString(testOut, testString);
    BOOST_CHECK_EQUAL(testString, testOut);
  } catch (const TApplicationException& tax) {
    BOOST_CHECK_EQUAL(true, false);
  }

  try {
    vector<int8_t> listtest, outList;
    listtest.push_back(5);
    testClient.echoList(outList, listtest);
    BOOST_CHECK_EQUAL(outList[0],5);
  } catch (const TApplicationException& tax) {
    BOOST_CHECK_EQUAL(true, false);
  }

  try {
    set<int8_t> settest, outSet;
    settest.insert(5);
    testClient.echoSet(outSet, settest);
    BOOST_CHECK_EQUAL(outSet.count(5), 1);
  } catch (const TApplicationException& tax) {
    BOOST_CHECK_EQUAL(true, false);
  }

  try {
    map<int8_t, int8_t> maptest, outMap;
    maptest[5] = 5;
    testClient.echoMap(outMap, maptest);
    BOOST_CHECK_EQUAL(outMap[5], 5);
  } catch (const TApplicationException& tax) {
    BOOST_CHECK_EQUAL(true, false);
  }

  try {
    transport->close();
  } catch (const TApplicationException& tax) {
    BOOST_CHECK_EQUAL(true, false);
  }
}

void* runServer(void*data) {

  std::shared_ptr<TServer> server(*(std::shared_ptr<TServer>*)data);
  server->serve();

  return nullptr;
}

void runTestCase(ServerType sType, ClientType clientType) {
  std::bitset<CLIENT_TYPES_LEN> clientTypes;
  clientTypes[THRIFT_UNFRAMED_DEPRECATED] = 1;
  clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
  clientTypes[THRIFT_HTTP_SERVER_TYPE] = 1;
  clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;
  clientTypes[THRIFT_FRAMED_COMPACT] = 1;
  THeaderProtocolFactory* factory = new THeaderProtocolFactory();
  factory->setClientTypes(clientTypes);

  std::shared_ptr<TDuplexProtocolFactory> protocolFactory =
    std::shared_ptr<TDuplexProtocolFactory>(factory);

  std::shared_ptr<TestHandler> testHandler(new TestHandler());

  std::shared_ptr<TDispatchProcessor> wrappedProcessor(
    new ServiceProcessor(testHandler));
  // Wrap the processor to observe exceptions in processing
  std::shared_ptr<TProcessor> testProcessor(
    new TProcessorWrapper(wrappedProcessor));
  std::shared_ptr<TAsyncProcessor> testAsyncProcessor(
    new TSyncToAsyncProcessor(testProcessor));

  std::shared_ptr<TDuplexTransportFactory> transportFactory(
    new TSingleTransportFactory<TBufferedTransportFactory>());

  auto event_handler = std::shared_ptr<TProcessorEventHandler>(
      new HeaderEventHandler());
  // set the header-replying event handler
  wrappedProcessor->addEventHandler(event_handler);
  testAsyncProcessor->addEventHandler(event_handler);

  int port = 0;

  std::shared_ptr<TServer> server;
  std::shared_ptr<ServerCreatorBase> serverCreator;
  switch(sType) {
    case SERVER_TYPE_SIMPLE:
      // "Testing TSimpleServerCreator"
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      serverCreator = std::shared_ptr<ServerCreatorBase>(new TSimpleServerCreator(
                                                      testProcessor, port,
                                                      false));
      #pragma GCC diagnostic pop
      break;
    case SERVER_TYPE_THREADED:
      serverCreator = std::shared_ptr<ServerCreatorBase>(new TThreadedServerCreator(
                                                      testProcessor, port,
                                                      false));
      break;
    case SERVER_TYPE_THREADPOOL:
      // "Testing TThreadPoolServerCreator"
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      serverCreator = std::shared_ptr<ServerCreatorBase>(
        new TThreadPoolServerCreator(testProcessor, port, false));
      #pragma GCC diagnostic pop
      break;
    case SERVER_TYPE_NONBLOCKING:
      serverCreator.reset(new TNonblockingServerCreator(testProcessor, port));
      break;
    case SERVER_TYPE_EVENT:
      serverCreator.reset(new TEventServerCreator(testAsyncProcessor, port));
      break;
  }
  serverCreator->setDuplexProtocolFactory(protocolFactory);

  server = serverCreator->createServer();
  ScopedServerThread serverThread(server);
  const TSocketAddress* address = serverThread.getAddress();
  runClient(clientType, sType, address->getPort());
}

// Listed individually to make it easy to see in the unit runner

BOOST_AUTO_TEST_CASE(simpleServerUnframed) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_UNFRAMED);
}

BOOST_AUTO_TEST_CASE(threadedServerUnframed) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_UNFRAMED);
}

BOOST_AUTO_TEST_CASE(threadPoolServerUnframed) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_UNFRAMED);
}

BOOST_AUTO_TEST_CASE(simpleServerFramed) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_FRAMED);
}

BOOST_AUTO_TEST_CASE(threadedServerFramed) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_FRAMED);
}

BOOST_AUTO_TEST_CASE(threadPoolServerFramed) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_FRAMED);
}

BOOST_AUTO_TEST_CASE(nonblockingServerFramed) {
  runTestCase(SERVER_TYPE_NONBLOCKING, CLIENT_TYPE_FRAMED);
}

BOOST_AUTO_TEST_CASE(eventServerFramed) {
  runTestCase(SERVER_TYPE_EVENT, CLIENT_TYPE_FRAMED);
}

BOOST_AUTO_TEST_CASE(simpleServerHeader) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_HEADER);
}

BOOST_AUTO_TEST_CASE(threadedServerHeader) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_HEADER);
}

BOOST_AUTO_TEST_CASE(threadPoolServerHeader) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_HEADER);
}

BOOST_AUTO_TEST_CASE(nonblockingServerHeader) {
  runTestCase(SERVER_TYPE_NONBLOCKING, CLIENT_TYPE_HEADER);
}

BOOST_AUTO_TEST_CASE(eventServerHeader) {
  runTestCase(SERVER_TYPE_EVENT, CLIENT_TYPE_HEADER);
}

BOOST_AUTO_TEST_CASE(simpleServerHttp) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_HTTP);
}

BOOST_AUTO_TEST_CASE(threadedServerHttp) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_HTTP);
}

BOOST_AUTO_TEST_CASE(threadPoolServerHttp) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_HTTP);
}

BOOST_AUTO_TEST_CASE(simpleServerCompactFramed) {
  runTestCase(SERVER_TYPE_SIMPLE, CLIENT_TYPE_FRAMED_COMPACT);
}

BOOST_AUTO_TEST_CASE(threadedServerCompactFramed) {
  runTestCase(SERVER_TYPE_THREADED, CLIENT_TYPE_FRAMED_COMPACT);
}

BOOST_AUTO_TEST_CASE(threadPoolServerCompactFramed) {
  runTestCase(SERVER_TYPE_THREADPOOL, CLIENT_TYPE_FRAMED_COMPACT);
}

BOOST_AUTO_TEST_CASE(nonblockingServerCompactFramed) {
  runTestCase(SERVER_TYPE_NONBLOCKING, CLIENT_TYPE_FRAMED_COMPACT);
}

BOOST_AUTO_TEST_CASE(eventServerCompactFramed) {
  runTestCase(SERVER_TYPE_EVENT, CLIENT_TYPE_FRAMED_COMPACT);
}

BOOST_AUTO_TEST_CASE(unframedBadRead) {
  auto buffer = std::shared_ptr<TTransport>(new TMemoryBuffer());
  auto transport = std::shared_ptr<TTransport>(new THeaderTransport(buffer));
  auto protocol = std::shared_ptr<TProtocol>(new THeaderProtocol(transport));
  std::string name = "test";
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

  try {
    protocol->readMessageBegin(name, messageType, seqId);
  } catch (...) {
    return;
  }
  BOOST_CHECK_EQUAL(true, false);
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  boost::unit_test::framework::master_test_suite().p_name.value =
    "THeaderTest";

  if (argc != 1) {
    fprintf(stderr, "unexpected arguments: %s\n", argv[1]);
    exit(1);
  }

  return nullptr;
}
