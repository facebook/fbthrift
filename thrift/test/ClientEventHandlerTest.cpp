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

#include <thrift/test/gen-cpp/Service.h>
#include <memory>

#include <thrift/lib/cpp/async/TAsyncChannel.h>
#include <thrift/lib/cpp/async/TFramedAsyncChannel.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/util/example/TSimpleServerCreator.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/transport/TSocket.h>

#include <gtest/gtest.h>

using namespace std;
using namespace apache::thrift::async;
using namespace apache::thrift;
using namespace apache::thrift::util;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace test::stress;

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

class ClientEventHandler : public TProcessorEventHandler {
  uint32_t contextCalls;

 public:
  ClientEventHandler() : contextCalls(0) {}

  void preRead(void* ctx, const char* fn_name) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
    EXPECT_EQ(contextCalls, 3);
  }

  void postRead(void* ctx,
                const char* fn_name,
                apache::thrift::transport::THeader* header,
                uint32_t bytes) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
    EXPECT_GT(bytes, 20); // check we read something
    EXPECT_EQ(contextCalls, 4);
   }

   void preWrite(void* ctx, const char* fn_name) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
    EXPECT_EQ(contextCalls, 1);
  }

  void postWrite(void* ctx, const char* fn_name, uint32_t bytes) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
    EXPECT_GT(bytes, 20); // check we wrote something
    EXPECT_EQ(contextCalls, 2);
  }

  void handlerError(void* ctx, const char* fn_name) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
  }

  void checkContextCalls() {
    EXPECT_EQ(contextCalls, 4);
  }
};

class ServerEventHandler : public TProcessorEventHandler {
  uint32_t contextCalls;

 public:
  ServerEventHandler() : contextCalls(0) {}

  void preRead(void* ctx, const char* fn_name) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
    EXPECT_EQ(contextCalls, 1);
  }

  void postRead(void* ctx,
                const char* fn_name,
                apache::thrift::transport::THeader* header,
                uint32_t bytes) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
    EXPECT_GT(bytes, 20); // check we read something
    EXPECT_EQ(contextCalls, 2);
   }

   void preWrite(void* ctx, const char* fn_name) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
    EXPECT_EQ(contextCalls, 3);
  }

  void postWrite(void* ctx, const char* fn_name, uint32_t bytes) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
    EXPECT_GT(bytes, 20); // check we wrote something
    EXPECT_EQ(contextCalls, 4);
  }

  void handlerError(void* ctx, const char* fn_name) override {
    contextCalls++;
    EXPECT_STREQ(fn_name, "Service.echoVoid");
  }

  void checkContextCalls() {
    EXPECT_EQ(contextCalls, 4);
  }
};

void echoVoidCallback(ServiceCobClient* client) {
  client->recv_echoVoid();
  /* Currently no way to pass client ctx here, these checks would fail */
  // handler->checkContextCalls();
  // handler2->checkContextCalls();
}

TEST(ClientEventHandlerTest, clientHandlerTest) {
  auto handler = make_shared<ClientEventHandler>();
  auto handler2 = make_shared<ClientEventHandler>();
  auto server_add_handler = make_shared<ServerEventHandler>();
  auto server_add_handler2 = make_shared<ServerEventHandler>();
  auto server_set_handler = make_shared<ServerEventHandler>();

  auto testHandler = make_shared<TestHandler>();
  auto testProcessor = make_shared<ServiceProcessor>(testHandler);
  testProcessor->addEventHandler(server_add_handler);
  testProcessor->addEventHandler(server_add_handler2);
  testProcessor->setEventHandler(server_set_handler);

  // "Handler test, so pick the simplest server"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  auto serverCreator = make_shared<TSimpleServerCreator>(testProcessor, 0);
  #pragma GCC diagnostic pop

  auto server = serverCreator->createServer();
  ScopedServerThread serverThread(server);
  int port = serverThread.getAddress()->getPort();

  auto socket = make_shared<TSocket>("localhost", port);
  socket->open();
  auto transport = make_shared<TFramedTransport>(socket);
  auto protocol = make_shared<TBinaryProtocol>(transport);

  ServiceClient testClient(protocol);
  handler = make_shared<ClientEventHandler>();
  testClient.addEventHandler(handler);
  testClient.echoVoid();
  handler->checkContextCalls();
  socket->close();

  // Check multiple server handlers
  server_add_handler->checkContextCalls();
  server_add_handler2->checkContextCalls();
  server_set_handler->checkContextCalls();

  testProcessor->setEventHandler(nullptr);
  testProcessor->clearEventHandlers();

  // Test async

  folly::EventBase evb;
  auto aSocket = TAsyncSocket::newSocket(&evb, "127.0.0.1", port);
  auto channel = TFramedAsyncChannel::newChannel(aSocket);
  auto protocolFactory = make_shared<TBinaryProtocolFactory>();

  ServiceCobClient testAsyncClient(channel, protocolFactory.get());
  handler = make_shared<ClientEventHandler>();
  testAsyncClient.addEventHandler(handler);
  testAsyncClient.addEventHandler(handler2);
  function<void(ServiceCobClient* client)> recvCob;
  recvCob = bind(echoVoidCallback, &testAsyncClient);

  testAsyncClient.echoVoid(recvCob);
  evb.loop();
}
