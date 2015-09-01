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
#include <folly/Format.h>

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

#include <gtest/gtest.h>

using namespace std;
using namespace folly;
using namespace apache::thrift::async;
using namespace apache::thrift;
using namespace apache::thrift::util;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace test::stress;
using apache::thrift::util::TEventServerCreator;

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
    auto socket = make_shared<TSocket>("localhost", port_);
    socket->open();
    auto transport = make_shared<TFramedTransport>(socket);
    auto protocol = make_shared<TBinaryProtocol>(transport);
    auto testClient = make_shared<ServiceClient>(protocol);

    // Get the ip for this client
    auto sa = make_shared<folly::SocketAddress>();
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
          FAIL() << "echoType not supported";  // Not support this type
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
      ADD_FAILURE() << "The given connection context is null";
      throw logic_error("The given connection context is null");
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
      ADD_FAILURE() << "The given fn_name is not supported: " << + fn_name;
      throw logic_error(
          sformat("The given fn_name is not supported: {}", fn_name));
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
    EXPECT_EQ(clientCountMap.size(), countMap.size());
    for (auto it: countMap) {
      echoTypeMap& serverEntry = it.second;
      echoTypeMap& clientEntry =
        clientCountMap.find(it.first)->second;
      EXPECT_EQ(serverEntry.size(), 1);
      EXPECT_EQ(clientEntry.size(), 1);
      EXPECT_EQ(clientEntry.find(serverEntry.begin()->first)->second,
                        serverEntry.begin()->second);
      LOG(INFO) << "Client IP: " << it.first <<  " echoType: "
                << it.second.begin()->first << " count: "
                << it.second.begin()->second;
    }
  }
};

TEST(AsyncConnectionContextTest, asyncConnectionContextTest) {
  // Start the thrift service
  PosixThreadFactory tf;
  tf.setDetached(false);

  auto serverHandler = make_shared<ServerEventHandler>();
  auto testHandler = make_shared<TestHandler>();

  auto processor = make_shared<ServiceProcessor>(testHandler);
  processor->addEventHandler(serverHandler);

  auto asyncProcessor = make_shared<TSyncToAsyncProcessor>(processor);
  asyncProcessor->addEventHandler(serverHandler);

  // Start the async server
  TEventServerCreator serverCreator(asyncProcessor, 0, 1);
  ScopedServerThread serverThread(&serverCreator);

  const folly::SocketAddress* socketAddress = serverThread.getAddress();
  int port = socketAddress->getPort();

  LOG(INFO) << "Initializing server on port " << port;

  // Going to create some client threads
  LOG(INFO) << "Generate a EchoVoid client";
  auto t1 = tf.newThread(make_shared<Client>(port, Client::ECHO_VOID, 5));

  LOG(INFO) << "Generate a EchoI32 client";
  auto t2 = tf.newThread(make_shared<Client>(port, Client::ECHO_I32, 2));

  LOG(INFO) << "Generate a EchoByte client";
  auto t3 = tf.newThread(make_shared<Client>(port, Client::ECHO_BYTE, 3));

  t1->start();
  t1->join();
  t2->start();
  t2->join();
  t3->start();
  t3->join();

  // Going to check the count info stored in the server
  serverHandler->checkCount(Client::countMap);
  LOG(INFO) << "Server check end";
}
