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

#include "thrift/test/gen-cpp/Service.h"
#include <boost/test/unit_test.hpp>
#include <memory>

#include <thrift/lib/cpp/async/TAsyncChannel.h>
#include <thrift/lib/cpp/async/TFramedAsyncChannel.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/util/example/TSimpleServerCreator.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/transport/TSocket.h>

using namespace apache::thrift::async;
using namespace apache::thrift;
using namespace apache::thrift::util;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace boost;
using namespace test::stress;
using std::string;
using std::map;
using std::set;
using std::vector;

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

class ClientEventHandler : public TProcessorEventHandler {
  uint32_t contextCalls;

 public:
  ClientEventHandler() : contextCalls(0) {}

  void preRead(void* ctx, const char* fn_name) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
    BOOST_CHECK_EQUAL(contextCalls, 3);
  }

  void postRead(void* ctx, const char* fn_name, uint32_t bytes) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
    BOOST_CHECK(bytes > 20); // check we read something
    BOOST_CHECK_EQUAL(contextCalls, 4);
   }

  void preWrite(void *ctx, const char* fn_name) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
    BOOST_CHECK_EQUAL(contextCalls, 1);
  }

  void postWrite(void *ctx, const char* fn_name, uint32_t bytes) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
    BOOST_CHECK(bytes > 20); // check we wrote something
    BOOST_CHECK_EQUAL(contextCalls, 2);
  }

  virtual void handlerError(void* ctx, const char* fn_name) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
  }

  void checkContextCalls() {
    BOOST_CHECK_EQUAL(contextCalls, 4);
  }
};

class ServerEventHandler : public TProcessorEventHandler {
  uint32_t contextCalls;

 public:
  ServerEventHandler() : contextCalls(0) {}

  void preRead(void* ctx, const char* fn_name) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
    BOOST_CHECK_EQUAL(contextCalls, 1);
  }

  void postRead(void* ctx, const char* fn_name, uint32_t bytes) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
    BOOST_CHECK(bytes > 20); // check we read something
    BOOST_CHECK_EQUAL(contextCalls, 2);
   }

  void preWrite(void *ctx, const char* fn_name) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
    BOOST_CHECK_EQUAL(contextCalls, 3);
  }

  void postWrite(void *ctx, const char* fn_name, uint32_t bytes) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
    BOOST_CHECK(bytes > 20); // check we wrote something
    BOOST_CHECK_EQUAL(contextCalls, 4);
  }

  virtual void handlerError(void* ctx, const char* fn_name) {
    contextCalls++;
    BOOST_CHECK_EQUAL(fn_name, "Service.echoVoid");
  }

  void checkContextCalls() {
    BOOST_CHECK_EQUAL(contextCalls, 4);
  }
};

std::shared_ptr<ClientEventHandler> handler(new ClientEventHandler());
std::shared_ptr<ClientEventHandler> handler2(new ClientEventHandler());
std::shared_ptr<ServerEventHandler> server_add_handler(new ServerEventHandler());
std::shared_ptr<ServerEventHandler> server_add_handler2(new ServerEventHandler());
std::shared_ptr<ServerEventHandler> server_set_handler(new ServerEventHandler());

void echoVoidCallback(ServiceCobClient* client) {
  client->recv_echoVoid();
  /* Currently no way to pass client ctx here, these checks would fail */
  // handler->checkContextCalls();
  // handler2->checkContextCalls();
}

BOOST_AUTO_TEST_CASE(clientHandlerTest) {
  std::shared_ptr<TestHandler> testHandler(new TestHandler());
  std::shared_ptr<TDispatchProcessor> testProcessor(
    new ServiceProcessor(testHandler));
  testProcessor->addEventHandler(server_add_handler);
  testProcessor->addEventHandler(server_add_handler2);
  testProcessor->setEventHandler(server_set_handler);

  // "Handler test, so pick the simplest server"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  std::shared_ptr<TSimpleServerCreator> serverCreator(
    new TSimpleServerCreator(testProcessor, 0));
  #pragma GCC diagnostic pop

  std::shared_ptr<TServer> server(serverCreator->createServer());
  ScopedServerThread serverThread(server);
  const TSocketAddress* address = serverThread.getAddress();
  int port = address->getPort();

  std::shared_ptr<TSocket> socket(new TSocket("localhost", port));
  socket->open();
  std::shared_ptr<TTransport> transport(new TFramedTransport(socket));
  std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  ServiceClient testClient(protocol);
  handler.reset(new ClientEventHandler());
  testClient.addEventHandler(handler);
  testClient.echoVoid();
  handler->checkContextCalls();
  socket->close();

  // Check multiple server handlers
  server_add_handler->checkContextCalls();
  server_add_handler2->checkContextCalls();
  server_set_handler->checkContextCalls();

  testProcessor->setEventHandler(std::shared_ptr<TProcessorEventHandler>());
  testProcessor->clearEventHandlers();

  // Test async

  TEventBase evb;
  std::shared_ptr<TAsyncSocket> aSocket(TAsyncSocket::newSocket(&evb,
                                                           "127.0.0.1", port));
  std::shared_ptr<TFramedAsyncChannel> channel(
    TFramedAsyncChannel::newChannel(aSocket));
  std::shared_ptr<TBinaryProtocolFactory> protocolFactory(
    new TBinaryProtocolFactory());

  ServiceCobClient testAsyncClient(channel, protocolFactory.get());
  handler.reset(new ClientEventHandler());
  testAsyncClient.addEventHandler(handler);
  testAsyncClient.addEventHandler(handler2);
  std::function<void(ServiceCobClient* client)> recvCob;
  recvCob = std::bind(echoVoidCallback, &testAsyncClient);

  testAsyncClient.echoVoid(recvCob);
  evb.loop();
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  boost::unit_test::framework::master_test_suite().p_name.value =
    "ClientHandlerTest";

  if (argc != 1) {
    fprintf(stderr, "unexpected arguments: %s\n", argv[1]);
    exit(1);
  }

  return nullptr;
}
