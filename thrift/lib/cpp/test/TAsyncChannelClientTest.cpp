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

#include <random>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TFramedAsyncChannel.h>
#include <thrift/lib/cpp/async/THttpAsyncChannel.h>
#include <thrift/lib/cpp/async/THeaderAsyncChannel.h>
#include <thrift/lib/cpp/async/TZlibAsyncChannel.h>
#include <thrift/lib/cpp/async/TBinaryAsyncChannel.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/transport/TZlibTransport.h>
#include <thrift/lib/cpp/transport/THttpServer.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/util/TThreadedServerCreator.h>
#include <thrift/perf/cpp/LoadHandler.h>

#include <gtest/gtest.h>

using namespace std;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;

class TestCallback {
 public:
  TestCallback() {
    reset();
  }

  void reset() {
    done = false;
    addResult = 0;
    echoResult.assign("");
  }

  void addDone(LoadTestCobClient* client) {
    done = true;
    try {
      addResult = client->recv_add();
    } catch (const exception& ex) {
      ADD_FAILURE() << "recv_add error: " << ex.what();
    }
  }

  void echoDone(LoadTestCobClient* client) {
    done = true;
    try {
      client->recv_echo(echoResult);
    } catch (const exception& ex) {
      ADD_FAILURE() << "recv_echo error: " << ex.what();
    }
  }

  bool done;
  int64_t addResult;
  string echoResult;
};

void randomizeString(string* str, size_t length) {
  char *buf = new char[length + sizeof(long int)];
  for (size_t n = 0; n < length; n += sizeof(long int)) {
    long int val = ::random();
    memcpy(buf + n, &val, sizeof(long int));
  }
  str->assign(buf, length);
}

void runTest(folly::EventBase& evb, LoadTestCobClient& client) {
  // Test sending a few requests to the server
  TestCallback callback;
  client.add(bind(&TestCallback::addDone, &callback,
                  placeholders::_1),
                  9, 12);
  evb.loop();
  EXPECT_TRUE(callback.done);
  EXPECT_EQ(21, callback.addResult);

  callback.reset();
  string testData;
  randomizeString(&testData, 3*1024);
  client.echo(bind(&TestCallback::echoDone, &callback,
                   placeholders::_1),
                   testData);
  evb.loop();
  EXPECT_TRUE(callback.done);
  EXPECT_EQ(testData, callback.echoResult);

  callback.reset();
  randomizeString(&testData, 1024*1024);
  client.echo(bind(&TestCallback::echoDone, &callback,
                   placeholders::_1),
                   testData);
  evb.loop();
  EXPECT_TRUE(callback.done);
  EXPECT_EQ(testData, callback.echoResult);

  callback.reset();
  client.add(bind(&TestCallback::addDone, &callback,
                  placeholders::_1),
                  321, 987);
  evb.loop();
  EXPECT_TRUE(callback.done);
  EXPECT_EQ(callback.addResult, 1308);
}

TEST(TAsyncChannelClientTest, TestZlibClient) {
  // Start a TThreadedServer using zlib transport
  auto handler = make_shared<LoadHandler>();
  auto processor = make_shared<LoadTestProcessor>(handler);
  auto transportFactory = dynamic_pointer_cast<TTransportFactory>(
      make_shared<TFramedZlibTransportFactory>());
  auto protocolFactory = dynamic_pointer_cast<TProtocolFactory>(
    make_shared<TBinaryProtocolFactoryT<TZlibTransport>>(0, 0, true, true));
  TThreadedServerCreator serverCreator(processor, 0, transportFactory,
                                       protocolFactory);
  ScopedServerThread serverThread(&serverCreator);

  // Create an async client using TZlibAsyncChannel
  folly::EventBase evb;
  auto serverAddr = *serverThread.getAddress();
  auto socket = TAsyncSocket::newSocket(&evb, serverAddr);
  auto framedChannel = TFramedAsyncChannel::newChannel(socket);
  auto zlibChannel = TZlibAsyncChannel::newChannel(framedChannel);

  TBinaryProtocolFactoryT<TBufferBase> clientProtocolFactory;
  clientProtocolFactory.setStrict(true, true);
  LoadTestCobClient client(zlibChannel, &clientProtocolFactory);

  runTest(evb, client);
}

TEST(TAsyncChannelClientTest, TestHttpClient) {
  // Start a TThreadedServer using HTTP transport
  auto handler = make_shared<LoadHandler>();
  auto processor = make_shared<LoadTestProcessor>(handler);
  auto transportFactory = dynamic_pointer_cast<TTransportFactory>(
      make_shared<THttpServerTransportFactory>());
  auto protocolFactory = dynamic_pointer_cast<TProtocolFactory>(
      make_shared<TBinaryProtocolFactoryT<THttpServer>>());
  TThreadedServerCreator serverCreator(processor, 0, transportFactory,
                                       protocolFactory);
  ScopedServerThread serverThread(&serverCreator);

  // Create an async client using THttpAsyncChannel
  folly::EventBase evb;
  auto serverAddr = *serverThread.getAddress();
  auto socket = TAsyncSocket::newSocket(&evb, serverAddr);
  auto httpChannel = THttpAsyncChannel::newChannel(socket);
  auto parser = make_shared<THttpClientParser>(
      serverAddr.getAddressStr(), "test");
  httpChannel->setParser(parser);
  TBinaryProtocolFactoryT<TBufferBase> clientProtocolFactory;
  LoadTestCobClient client(httpChannel, &clientProtocolFactory);

  runTest(evb, client);
}

TEST(TAsyncChannelClientTest, TestDuplexHeaderClient) {
  // Start a TThreadedServer using Header transport
  auto handler = make_shared<LoadHandler>();
  auto processor = make_shared<LoadTestProcessor>(handler);

  bitset<CLIENT_TYPES_LEN> clientTypes;
  clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
  clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;
  auto clientDuplexProtocolFactory = make_shared<THeaderProtocolFactory>();
  auto serverDuplexProtocolFactory = make_shared<THeaderProtocolFactory>();
  clientDuplexProtocolFactory->setClientTypes(clientTypes);
  serverDuplexProtocolFactory->setClientTypes(clientTypes);

  auto transportFactory = make_shared<TBufferedTransportFactory>();

  TThreadedServerCreator serverCreator(processor, 0);
  serverCreator.setTransportFactory(transportFactory);
  serverCreator.setDuplexProtocolFactory(serverDuplexProtocolFactory);
  ScopedServerThread serverThread(&serverCreator);

  // Create an async client using THeaderAsyncChannel
  folly::EventBase evb;
  auto serverAddr = *serverThread.getAddress();
  auto socket = TAsyncSocket::newSocket(&evb, serverAddr);
  auto headerChannel = THeaderAsyncChannel::newChannel(socket);
  LoadTestCobClient client(headerChannel, clientDuplexProtocolFactory.get());

  runTest(evb, client);
}
