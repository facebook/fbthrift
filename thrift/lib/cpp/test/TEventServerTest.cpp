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

#include <signal.h>
#include <pthread.h>

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

#include <boost/test/unit_test.hpp>
#include <memory>

using namespace boost;

using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using std::cerr;
using std::endl;
using std::string;
using std::shared_ptr;
using std::make_shared;

namespace apache { namespace thrift { namespace test {

class TEventServerServiceHandler :
      public TEventServerTestServiceCobSvIf {
 public:
  explicit TEventServerServiceHandler(TEventBase *eventBase) :
      eventBase_(eventBase) {
  }

  ~TEventServerServiceHandler() override {}

  void sendResponse(std::function<void(std::string const& _return)> cob,
                    int64_t size) override {
    string s(size, 'a');

    // Stop the client event loop so he won't read the whole thing
    eventBase_->terminateLoopSoon();

    // Send a reply
    cob(s);
  }

  void noop(std::function<void(void)> cob) override { cob(); }

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
    BOOST_FAIL("oops");
  }
}

BOOST_AUTO_TEST_CASE(ServerShutdownWithOutstandingMessage) {
  int workerThreads = 1;

  // Initialize thrift service
  TEventBase eventBase;
  std::shared_ptr<TEventServerServiceHandler> handler(
    new TEventServerServiceHandler(&eventBase));
  std::shared_ptr<TAsyncProcessor> processor(
    new TEventServerTestServiceAsyncProcessor(handler));

  TEventServerCreator serverCreator(processor, 0, workerThreads);
  ScopedServerThread serverThread(&serverCreator);

  const folly::SocketAddress *address = serverThread.getAddress();
  std::unique_ptr<TEventServerTestServiceCobClient> cl(
    createClient<TEventServerTestServiceCobClient>(
      &eventBase, *address));
  cl->sendResponse(std::bind(responseReceived, std::placeholders::_1, false),
                   1024 * 1024 * 8);
  eventBase.loop();
  serverThread.stop();
  eventBase.loop();
}

BOOST_AUTO_TEST_CASE(ExplicitHeaderProtocolAndTransport) {
  // Initialize thrift service
  TEventBase eventBase;
  std::shared_ptr<TEventServerServiceHandler> handler =
    std::make_shared<TEventServerServiceHandler>(&eventBase);
  std::shared_ptr<TAsyncProcessor> processor =
    std::make_shared<TEventServerTestServiceAsyncProcessor>(handler);
  std::shared_ptr<THeaderProtocolFactory> headerProtocolFactory =
    std::make_shared<THeaderProtocolFactory>();

  int serverPort = 0;

  std::shared_ptr<TEventServer> server =
    std::make_shared<TEventServer>(processor, headerProtocolFactory, serverPort);
  server->setTransportType(TEventServer::HEADER);

  ScopedServerThread serverThread(server);

  const folly::SocketAddress *address = serverThread.getAddress();
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&eventBase, *address));
  std::shared_ptr<THeaderAsyncChannel> channel(
    THeaderAsyncChannel::newChannel(socket));
  std::shared_ptr<THeaderProtocolFactory> protocolFactory =
    std::make_shared<THeaderProtocolFactory>();
  std::shared_ptr<TEventServerTestServiceCobClient> cl =
    std::make_shared<TEventServerTestServiceCobClient>(channel,
                                                  protocolFactory.get());

  cl->noop(std::bind(responseReceived, std::placeholders::_1, true));
  eventBase.loop();
  serverThread.stop();
  eventBase.loop();
}
}}}

///////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
///////////////////////////////////////////////////////////////////////////

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value =
    "TEventServerTest";
  signal(SIGPIPE, SIG_IGN);

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
