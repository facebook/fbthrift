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

#include <thrift/lib/cpp/util/TEventServerCreator.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/util/AsyncClientUtil.h>
#include <thrift/lib/cpp/test/gen-cpp/TEventServerTestService.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TFramedAsyncChannel.h>
#include <thrift/lib/cpp/async/THeaderAsyncChannel.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/transport/THeaderTransport.h>

#include <gtest/gtest.h>

using namespace std;
using namespace folly;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;

namespace {
class TEventServerServiceHandler :
      public TEventServerTestServiceCobSvIf {
 public:
  explicit TEventServerServiceHandler(TEventBase *eventBase) :
      eventBase_(eventBase) {
  }

  ~TEventServerServiceHandler() override {}

  void sendResponse(function<void(string const& _return)> cob,
                    int64_t size) override {
    string s(size, 'a');

    // Stop the client event loop so he won't read the whole thing
    eventBase_->terminateLoopSoon();

    // Send a reply
    cob(s);
  }

  void noop(function<void(void)> cob) override { cob(); }

 private:

  TEventBase *eventBase_;
};

static void responseReceived(TEventServerTestServiceCobClient* client,
                             bool expectSuccess) {
  if (client->getChannel()->good()) {
    cerr << "Received full response" << endl;
  } else {
    cerr << "Did not receive full response" << endl;
  }

  if (client->getChannel()->good() != expectSuccess) {
    FAIL() << "oops";
  }
}
}

TEST(TEventServerTest, ServerShutdownWithOutstandingMessage) {
  int workerThreads = 1;

  // Initialize thrift service
  TEventBase eventBase;
  auto handler = make_shared<TEventServerServiceHandler>(&eventBase);
  auto processor = make_shared<TEventServerTestServiceAsyncProcessor>(handler);

  TEventServerCreator serverCreator(processor, 0, workerThreads);
  ScopedServerThread serverThread(&serverCreator);

  auto address = *serverThread.getAddress();
  unique_ptr<TEventServerTestServiceCobClient> cl(
    createClient<TEventServerTestServiceCobClient>(
      &eventBase, address));
  cl->sendResponse(bind(responseReceived, placeholders::_1, false),
                   1024 * 1024 * 8);
  eventBase.loop();
  serverThread.stop();
  eventBase.loop();
}

TEST(TEventServerTest, ExplicitHeaderProtocolAndTransport) {
  // Initialize thrift service
  TEventBase eventBase;
  auto handler = make_shared<TEventServerServiceHandler>(&eventBase);
  auto processor = make_shared<TEventServerTestServiceAsyncProcessor>(handler);
  auto headerProtocolFactory = make_shared<THeaderProtocolFactory>();

  int serverPort = 0;

  auto server =
    make_shared<TEventServer>(processor, headerProtocolFactory, serverPort);
  server->setTransportType(TEventServer::HEADER);

  ScopedServerThread serverThread(server);

  auto address = *serverThread.getAddress();
  auto socket = TAsyncSocket::newSocket(&eventBase, address);
  auto channel = THeaderAsyncChannel::newChannel(socket);
  auto protocolFactory = make_shared<THeaderProtocolFactory>();
  auto cl = make_shared<TEventServerTestServiceCobClient>(
      channel, protocolFactory.get());

  cl->noop(bind(responseReceived, placeholders::_1, true));
  eventBase.loop();
  serverThread.stop();
  eventBase.loop();
}
