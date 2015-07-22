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
#include <boost/random.hpp>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TFramedAsyncChannel.h>
#include <thrift/lib/cpp/async/THttpAsyncChannel.h>
#include <thrift/lib/cpp/async/THeaderAsyncChannel.h>
#include <thrift/lib/cpp/async/TZlibAsyncChannel.h>
#include <thrift/lib/cpp/async/TBinaryAsyncChannel.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include "thrift/perf/cpp/LoadHandler.h"
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/transport/TZlibTransport.h>
#include <thrift/lib/cpp/transport/THttpServer.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/util/TThreadedServerCreator.h>

using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;
using namespace boost;
using std::string;
using std::cerr;
using std::endl;

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
    } catch (const std::exception& ex) {
      BOOST_ERROR("recv_add error: " << ex.what());
    }
  }

  void echoDone(LoadTestCobClient* client) {
    done = true;
    try {
      client->recv_echo(echoResult);
    } catch (const std::exception& ex) {
      BOOST_ERROR("recv_echo error: " << ex.what());
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

void runTest(TEventBase& evb, LoadTestCobClient& client) {
  // Test sending a few requests to the server
  TestCallback callback;
  client.add(std::bind(&TestCallback::addDone, &callback,
                       std::placeholders::_1),
                       9, 12);
  evb.loop();
  BOOST_CHECK(callback.done);
  BOOST_CHECK_EQUAL(callback.addResult, 21);

  callback.reset();
  string testData;
  randomizeString(&testData, 3*1024);
  client.echo(std::bind(&TestCallback::echoDone, &callback,
                        std::placeholders::_1),
                        testData);
  evb.loop();
  BOOST_CHECK(callback.done);
  BOOST_CHECK(callback.echoResult == testData);

  callback.reset();
  randomizeString(&testData, 1024*1024);
  client.echo(std::bind(&TestCallback::echoDone, &callback,
                        std::placeholders::_1),
                        testData);
  evb.loop();
  BOOST_CHECK(callback.done);
  BOOST_CHECK(callback.echoResult == testData);

  callback.reset();
  client.add(std::bind(&TestCallback::addDone, &callback,
                       std::placeholders::_1),
                       321, 987);
  evb.loop();
  BOOST_CHECK(callback.done);
  BOOST_CHECK_EQUAL(callback.addResult, 1308);
}

BOOST_AUTO_TEST_CASE(TestZlibClient) {
  // Start a TThreadedServer using zlib transport
  std::shared_ptr<LoadHandler> handler(new LoadHandler);
  std::shared_ptr<LoadTestProcessor> processor(new LoadTestProcessor(handler));
  std::shared_ptr<TTransportFactory> transportFactory(
      new TFramedZlibTransportFactory);
  std::shared_ptr<TProtocolFactory> protocolFactory(
      new TBinaryProtocolFactoryT<TZlibTransport>(0, 0, true, true));
  TThreadedServerCreator serverCreator(processor, 0, transportFactory,
                                       protocolFactory);
  ScopedServerThread serverThread(&serverCreator);

  // Create an async client using TZlibAsyncChannel
  TEventBase evb;
  const folly::SocketAddress* serverAddr = serverThread.getAddress();
  std::shared_ptr<TAsyncSocket> socket(TAsyncSocket::newSocket(
        &evb, serverAddr->getAddressStr(), serverAddr->getPort()));
  std::shared_ptr<TFramedAsyncChannel> framedChannel(
      TFramedAsyncChannel::newChannel(socket));
  std::shared_ptr<TZlibAsyncChannel> zlibChannel(
      TZlibAsyncChannel::newChannel(framedChannel));

  TBinaryProtocolFactoryT<TBufferBase> clientProtocolFactory;
  clientProtocolFactory.setStrict(true, true);
  LoadTestCobClient client(zlibChannel, &clientProtocolFactory);

  runTest(evb, client);
}

BOOST_AUTO_TEST_CASE(TestHttpClient) {
  // Start a TThreadedServer using HTTP transport
  std::shared_ptr<LoadHandler> handler(new LoadHandler);
  std::shared_ptr<LoadTestProcessor> processor(new LoadTestProcessor(handler));
  std::shared_ptr<TTransportFactory> transportFactory(
      new THttpServerTransportFactory());
  std::shared_ptr<TProtocolFactory> protocolFactory(
      new TBinaryProtocolFactoryT<THttpServer>());
  TThreadedServerCreator serverCreator(processor, 0, transportFactory,
                                       protocolFactory);
  ScopedServerThread serverThread(&serverCreator);

  // Create an async client using THttpAsyncChannel
  TEventBase evb;
  const folly::SocketAddress* serverAddr = serverThread.getAddress();
  std::shared_ptr<TAsyncSocket> socket(TAsyncSocket::newSocket(
        &evb, serverAddr->getAddressStr(), serverAddr->getPort()));
  std::shared_ptr<THttpAsyncChannel> httpChannel(
      THttpAsyncChannel::newChannel(socket));
  std::shared_ptr<THttpParser> parser(
    new THttpClientParser(serverAddr->getAddressStr(),
    "test"));
  httpChannel->setParser(parser);
  TBinaryProtocolFactoryT<TBufferBase> clientProtocolFactory;
  LoadTestCobClient client(httpChannel, &clientProtocolFactory);

  runTest(evb, client);
}

BOOST_AUTO_TEST_CASE(TestDuplexHeaderClient) {
  // Start a TThreadedServer using Header transport
  std::shared_ptr<LoadHandler> handler(new LoadHandler);
  std::shared_ptr<LoadTestProcessor> processor(new LoadTestProcessor(handler));

  std::bitset<CLIENT_TYPES_LEN> clientTypes;
  clientTypes[THRIFT_FRAMED_DEPRECATED] = 1;
  clientTypes[THRIFT_HEADER_CLIENT_TYPE] = 1;
  std::shared_ptr<THeaderProtocolFactory> clientDuplexProtocolFactory(
    new THeaderProtocolFactory);
  std::shared_ptr<THeaderProtocolFactory> serverDuplexProtocolFactory(
    new THeaderProtocolFactory);
  clientDuplexProtocolFactory->setClientTypes(clientTypes);
  serverDuplexProtocolFactory->setClientTypes(clientTypes);

  std::shared_ptr<TTransportFactory> transportFactory(
    new TBufferedTransportFactory());

  TThreadedServerCreator serverCreator(processor, 0);
  serverCreator.setTransportFactory(transportFactory);
  serverCreator.setDuplexProtocolFactory(serverDuplexProtocolFactory);
  ScopedServerThread serverThread(&serverCreator);

  // Create an async client using THeaderAsyncChannel
  TEventBase evb;
  const folly::SocketAddress* serverAddr = serverThread.getAddress();
  std::shared_ptr<TAsyncSocket> socket(TAsyncSocket::newSocket(
        &evb, serverAddr->getAddressStr(), serverAddr->getPort()));
  std::shared_ptr<THeaderAsyncChannel> headerChannel(
      THeaderAsyncChannel::newChannel(socket));
  LoadTestCobClient client(headerChannel, clientDuplexProtocolFactory.get());

  runTest(evb, client);
}

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value =
  "TZlibAsyncChannelTest";

  if (argc != 1) {
    cerr << "error: unhandled arguments:";
    for (int n = 1; n < argc; ++n) {
      cerr << " " << argv[n];
    }
    cerr << endl;
    exit(1);
  }

  return nullptr;
}
