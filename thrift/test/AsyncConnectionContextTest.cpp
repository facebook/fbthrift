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

#include "thrift/test/gen-cpp/Service.h"
#include <boost/test/unit_test.hpp>
#include <memory>

#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/util/TEventServerCreator.h>
#include <thrift/lib/cpp/util/example/TSimpleServerCreator.h>
#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/transport/TSocket.h>
#include <thrift/lib/cpp/async/TSyncToAsyncProcessor.h>

using namespace apache::thrift::async;
using namespace apache::thrift;
using namespace apache::thrift::util;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace boost;
using namespace test::stress;
using apache::thrift::util::TEventServerCreator;
using std::function;
using std::string;
using std::map;
using std::set;
using std::vector;
using std::pair;

/**
 * The class for creating a client thread
 */
class Client: public Runnable {

 public:
  // The enum for each echo type
  enum EchoType {
    ECHO_VOID,
    ECHO_I32,
    ECHO_BYTE,
    ERROR_TYPE
  };

  Client(int port, EchoType echoType, int count) {
    port_ = port;
    echoType_ = echoType;
    count_ = count;
  }

  void run() override {
    std::shared_ptr<TSocket> socket(new TSocket("localhost", port_));
    socket->open();
    std::shared_ptr<TTransport> transport(new TFramedTransport(socket));
    std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    std::shared_ptr<ServiceClient> testClient(new ServiceClient(protocol));

    // Get the ip for this client
    std::shared_ptr<folly::SocketAddress> sa(new folly::SocketAddress());
    sa->setFromLocalAddress(socket->getSocketFD());
    // Update the countMap using the ipAddr port
    int port = sa->getPort();
    auto it = countMap.find(port);
    if (it == countMap.end()) {
      map<Client::EchoType, int> entry;
      entry[echoType_] = count_;
      countMap.insert( pair<int, map<Client::EchoType, int>>(port, entry) );
    } else {
      it->second[echoType_] += count_;
    }

    for (int i = 0; i < count_; i++) {
      switch (echoType_) {
        case ECHO_VOID:
          testClient->echoVoid();
          break;
        case ECHO_I32:
          testClient->echoI32(1);
          break;
        case ECHO_BYTE:
          testClient->echoByte(2);
          break;
        default:
          BOOST_FAIL("echoType not supported");  // Not support this type
      }
    }
    socket->close();
  }

  static map<int, map<Client::EchoType, int>> countMap;

 private:
  int port_;
  EchoType echoType_;
  int count_;
};

typedef map<Client::EchoType, int> echoTypeMap;
map<int, echoTypeMap> Client::countMap;

class TestHandler :public ServiceIf {
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

class ServerEventHandler : public TProcessorEventHandler {
  // port -> map<echoType, count>
  map<int, echoTypeMap> countMap;

 public:
  /**
   * The function used to get the connection context (IP_addr) for the
   * current connection.
   *
   * @returns current IP_address for this connection,
   *          folly::SocketAddress* casted into a void*.
   */
  void* getContext(const char* fn_name,
                   TConnectionContext* serverContext = nullptr) override {
    if (serverContext != nullptr) {
      return (void *)serverContext->getPeerAddress();
    }
    else {
      BOOST_FAIL("The given connection context is null\n");
      return nullptr;
    }
  }

  /**
   * Get the client ip port from the connection context
   */
  int getPort(void* ctx) {
    return ((folly::SocketAddress*)ctx)->getPort();
  }

  /**
   * Get the echo type from the fn_name
   */
  Client::EchoType getEchoType(const char* fn_name) {
    string type(fn_name);
    if (type == "Service.echoVoid") {
      return Client::ECHO_VOID;
    } else if (type == "Service.echoI32") {
      return Client::ECHO_I32;
    } else if (type == "Service.echoByte") {
      return Client::ECHO_BYTE;
    } else {
      BOOST_FAIL(
        string("The given fn_name is not supported:") + fn_name);
      return Client::ERROR_TYPE;  // Should never go here
    }
  }

  /**
   * Update the localCountMap according to the echoType
   */
  void addToCountMap(int port, Client::EchoType echoType) {
    auto it = countMap.find(port);
    if (it == countMap.end()) {
      echoTypeMap entry;
      entry[echoType] = 1;
      countMap.insert( pair<int, echoTypeMap>(port, entry) );
    } else {
      it->second[echoType] += 1;
    }
  }

  void preRead(void* ctx, const char* fn_name) override {
    addToCountMap(getPort(ctx), getEchoType(fn_name));
  }

  /**
   * Check if the count data stored in the server is same as the client
   * side countMap.
   */
  virtual void checkCount(
               map<int, echoTypeMap>& clientCountMap) {
    BOOST_CHECK_EQUAL(clientCountMap.size(), countMap.size());
    for (auto it: countMap) {
      echoTypeMap& serverEntry = it.second;
      echoTypeMap& clientEntry =
        clientCountMap.find(it.first)->second;
      BOOST_CHECK_EQUAL(serverEntry.size(), 1);
      BOOST_CHECK_EQUAL(clientEntry.size(), 1);
      BOOST_CHECK_EQUAL(clientEntry.find(serverEntry.begin()->first)->second,
                        serverEntry.begin()->second);
      std::cout <<"Client IP: " << it.first <<  " echoType: " <<
        it.second.begin()->first << " count: "
        << it.second.begin()->second << "\n";
    }
  }
};

BOOST_AUTO_TEST_CASE(asyncConnectionContextTest) {
  // Start the thrift service
  PosixThreadFactory tf;
  tf.setDetached(false);

  std::shared_ptr<ServerEventHandler> serverHandler(new ServerEventHandler());
  std::shared_ptr<TestHandler> testHandler(new TestHandler());

  std::shared_ptr<TDispatchProcessor> processor(
    new ServiceProcessor(testHandler));
  processor->addEventHandler(serverHandler);

  std::shared_ptr<TAsyncProcessor> asyncProcessor(
    new TSyncToAsyncProcessor(processor));
  asyncProcessor->addEventHandler(serverHandler);

  // Start the async server
  TEventServerCreator serverCreator(asyncProcessor, 0, 1);
  ScopedServerThread serverThread(&serverCreator);

  const folly::SocketAddress* socketAddress = serverThread.getAddress();
  int port = socketAddress->getPort();

  std::cout << "Initializing server on port " << port << "\n";

  // Going to create some client threads
  std::cout << "Generate a EchoVoid client\n";
  std::shared_ptr<Thread> t1 =
    tf.newThread(std::shared_ptr<Runnable>(new Client(
      port, Client::ECHO_VOID, 5)));

  std::cout << "Generate a EchoI32 client\n";
  std::shared_ptr<Thread> t2 =
    tf.newThread(std::shared_ptr<Runnable>(new Client(
      port, Client::ECHO_I32, 2)));

  std::cout << "Generate a EchoByte client\n";
  std::shared_ptr<Thread> t3 =
    tf.newThread(std::shared_ptr<Runnable>(new Client(
      port, Client::ECHO_BYTE, 3)));

  t1->start();
  t1->join();
  t2->start();
  t2->join();
  t3->start();
  t3->join();

  // Going to check the count info stored in the server
  serverHandler->checkCount(Client::countMap);
  std::cout << "Server check end\n";
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  boost::unit_test::framework::master_test_suite().p_name.value =
    "AsyncConnectionContextTest";

  if (argc != 1) {
    fprintf(stderr, "unexpected arguments: %s\n", argv[1]);
    exit(1);
  }

  return nullptr;
}
