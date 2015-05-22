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

#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/Util.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/server/TConnectionContext.h>
#include <thrift/lib/cpp/server/test/gen-cpp/ConnCtxService.h>
#include <thrift/lib/cpp/test/NetworkUtil.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/transport/TSocket.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/util/TNonblockingServerCreator.h>
#include <thrift/lib/cpp/util/example/TSimpleServerCreator.h>
#include <thrift/lib/cpp/util/TThreadedServerCreator.h>
#include <thrift/lib/cpp/util/example/TThreadPoolServerCreator.h>

#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace boost;
using std::vector;
using std::string;
using std::pair;
using std::cerr;
using std::endl;
using std::make_pair;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::Runnable;
using apache::thrift::concurrency::Thread;
using apache::thrift::concurrency::Util;
using apache::thrift::protocol::TBinaryProtocolT;
using apache::thrift::server::TConnectionContext;
using apache::thrift::server::TServer;
using apache::thrift::transport::TBufferBase;
using apache::thrift::transport::TFramedTransport;
using apache::thrift::transport::TSocket;
using folly::SocketAddress;
using apache::thrift::util::ScopedServerThread;
using apache::thrift::util::ServerCreator;
using apache::thrift::util::TNonblockingServerCreator;
using apache::thrift::util::TSimpleServerCreator;
using apache::thrift::util::TThreadedServerCreator;
using apache::thrift::util::TThreadPoolServerCreator;
using apache::thrift::test::getLocalAddresses;

class ConnCtxHandler : public ConnCtxServiceIf {
 public:
  void setServer(TServer* server) {
    server_ = server;
  }

  void getClientAddress(string& result) override {
    TConnectionContext* ctx = server_->getConnectionContext();
    if (ctx == nullptr) {
      CtxError ex;
      ex.message = "server returned nullptr context";
      throw ex;
    }

    const folly::SocketAddress* peerAddr =
      server_->getConnectionContext()->getPeerAddress();

    sockaddr_storage addrStorage;
    peerAddr->getAddress(&addrStorage);
    result.assign(reinterpret_cast<const char*>(&addrStorage),
                  peerAddr->getActualSize());
  }

 private:
  TServer* server_;
};

class CtxClient : public Runnable {
 public:
  typedef TBinaryProtocolT<TBufferBase> Protocol;
  typedef vector< pair<folly::SocketAddress, folly::SocketAddress> > ErrorVector;

  CtxClient(const vector<folly::SocketAddress>* addresses,
            uint32_t numIterations)
    : numIterations_(numIterations),
      addresses_(addresses) {}

  void run() override {
    for (uint32_t n = 0; n < numIterations_; ++n) {
      for (vector<folly::SocketAddress>::const_iterator it = addresses_->begin();
           it != addresses_->end();
           ++it) {
        // Connect to the server
        std::shared_ptr<TSocket> socket(new TSocket(&*it));
        socket->open();

        // Get the local address that our client socket is using
        folly::SocketAddress clientAddress;
        clientAddress.setFromLocalAddress(socket->getSocketFD());

        // Create a client, and call getAddress() to have the server tell
        // us what address it thinks we are connecting from.
        std::shared_ptr<TFramedTransport> transport(new TFramedTransport(socket));
        std::shared_ptr<Protocol> protocol(new Protocol(transport));
        ConnCtxServiceClient client(protocol);

        string addressData;
        try {
          client.getClientAddress(addressData);
        } catch (const std::exception& ex) {
          folly::SocketAddress uninitAddress;
          errors_.push_back(make_pair(uninitAddress, clientAddress));
          continue;
        }

        folly::SocketAddress returnedAddress;
        returnedAddress.setFromSockaddr(
            reinterpret_cast<const struct sockaddr*>(addressData.c_str()),
            addressData.size());

        if (returnedAddress != clientAddress) {
          returnedAddress.tryConvertToIPv4();
          clientAddress.tryConvertToIPv4();
          if (returnedAddress != clientAddress) {
            errors_.push_back(make_pair(returnedAddress, clientAddress));
          }
        }
      }
    }
  }

  const ErrorVector* getErrors() const {
    return &errors_;
  }

 private:
  int64_t duration_;
  uint64_t numIterations_;
  const vector<folly::SocketAddress>* addresses_;
  ErrorVector errors_;
};

struct ClientInfo {
  std::shared_ptr<CtxClient> client;
  std::shared_ptr<Thread> thread;
};

void runTest(std::shared_ptr<ConnCtxHandler> handler,
             ServerCreator* serverCreator) {
  // Get the list of local IPs
  vector<folly::SocketAddress> localAddresses;
  getLocalAddresses(&localAddresses);

  // Start the server
  std::shared_ptr<TServer> server = serverCreator->createServer();
  handler->setServer(server.get());
  ScopedServerThread serverThread(server);

  // Update the localAddresses list to contain the server's port
  uint16_t serverPort = serverThread.getAddress()->getPort();
  for (vector<folly::SocketAddress>::iterator it = localAddresses.begin();
       it != localAddresses.end();
       ++it) {
    it->setPort(serverPort);
  }

  // Start client threads to connect to the server
  uint32_t numIterations = 100;
  unsigned int numThreads = 10;

  vector<ClientInfo> clients;
  PosixThreadFactory threadFactory;
  threadFactory.setDetached(false);
  for (unsigned int n = 0; n < numThreads; ++n) {
    ClientInfo info;
    info.client.reset(new CtxClient(&localAddresses, numIterations));
    info.thread = threadFactory.newThread(info.client);
    clients.push_back(info);
    info.thread->start();
  }

  int totalLogged = 0;
  int maxErrors = 10; // only report the first 10 errors
  for (vector<ClientInfo>::iterator it = clients.begin();
       it != clients.end();
       ++it) {
    it->thread->join();
    // Check that there were no errors.
    // (If we don't do this, and there were no errors, we won't have made any
    // boost test checks, so boost complains.)  We also check size() rather
    // than empty(), so that the number of errors will be included in the
    // output, if there were any.
    BOOST_CHECK_EQUAL(it->client->getErrors()->size(), 0);

    // Log any errors.  (We can't do it as they occur, since
    // BOOST_CHECK/BOOST_ERROR isn't safe to call from other threads.)
    const CtxClient::ErrorVector* errors = it->client->getErrors();
    for (CtxClient::ErrorVector::const_iterator it = errors->begin();
         totalLogged < maxErrors && it != errors->end();
         ++totalLogged, ++it) {
      BOOST_ERROR("address mismatch: " << it->first.describe() << " != " <<
                  it->second.describe());
    }
  }
}

template <typename ServerCreatorT>
void runTest() {
  std::shared_ptr<ConnCtxHandler> handler(new ConnCtxHandler);
  std::shared_ptr<ConnCtxServiceProcessor> processor(
      new ConnCtxServiceProcessor(handler));
  uint16_t port = 0;
  ServerCreatorT serverCreator(processor, port);

  runTest(handler, &serverCreator);
}

BOOST_AUTO_TEST_CASE(TSimpleServerTest) {
  // "For testing TSimpleServerCreator"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  runTest<TSimpleServerCreator>();
  #pragma GCC diagnostic pop
}

BOOST_AUTO_TEST_CASE(TThreadedServerTest) {
  runTest<TThreadedServerCreator>();
}

BOOST_AUTO_TEST_CASE(TThreadPoolServerTest) {
  // "For testing TThreadPoolServerCreator"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  runTest<TThreadPoolServerCreator>();
  #pragma GCC diagnostic pop
}

/**
 * Test TNonblockingServer when used with a thread pool
 */
BOOST_AUTO_TEST_CASE(TNonblockingServerTest) {
  runTest<TNonblockingServerCreator>();
}

/**
 * Test TNonblockingServer when used without a thread pool
 */
BOOST_AUTO_TEST_CASE(TNonblockingServerNoThreadsTest) {
  std::shared_ptr<ConnCtxHandler> handler(new ConnCtxHandler);
  std::shared_ptr<ConnCtxServiceProcessor> processor(
      new ConnCtxServiceProcessor(handler));
  TNonblockingServerCreator serverCreator(processor, 0, 0);

  runTest(handler, &serverCreator);
}

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value = "ConnCtxTest";

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
