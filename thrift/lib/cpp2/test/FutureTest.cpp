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
#include <gtest/gtest.h>
#include "thrift/lib/cpp2/async/RequestChannel.h"
#include "thrift/lib/cpp2/async/FutureRequest.h"
#include "thrift/lib/cpp2/test/gen-cpp2/FutureService.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp2/async/HeaderClientChannel.h"

#include "thrift/lib/cpp/util/ScopedServerThread.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"

#include "thrift/lib/cpp2/async/StubSaslClient.h"
#include "thrift/lib/cpp2/async/StubSaslServer.h"

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <atomic>

#include "thrift/lib/cpp2/test/TestUtils.h"

#include "common/concurrency/Executor.h"
#include "common/wangle/GenericThreadGate.h"

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::async;
using namespace facebook::concurrency;
using namespace facebook::wangle;
using namespace folly;

class TestInterface : public FutureServiceSvIf {
  Future<std::unique_ptr<std::string>> future_sendResponse(int64_t size) {
    EXPECT_NE("", getConnectionContext()->getPeerAddress()->describe());

    MoveWrapper<Promise<std::unique_ptr<std::string> > > p;
    auto f = p->getFuture();

    auto func = std::function<void()>([=]() mutable {
      std::unique_ptr<std::string> _return(
        new std::string("test" + boost::lexical_cast<std::string>(size)));
      p->setValue(std::move(_return));
    });

    getEventBase()->runAfterDelay(func, size);

    return std::move(f);
  }

  Future<void> future_noResponse(int64_t size) {
    MoveWrapper<Promise<void>> p;
    auto f = p->getFuture();

    auto func = std::function<void()>([=]() mutable {
      p->setValue();
    });
    getEventBase()->runAfterDelay(func, size);
    return std::move(f);
  }

  Future<std::unique_ptr<std::string>> future_echoRequest(
    std::unique_ptr<std::string> req) {
    *req += "ccccccccccccccccccccccccccccccccccccccccccccc";

    return makeFuture<std::unique_ptr<std::string>>(std::move(req));
  }

  Future<int> future_throwing() {
    Promise<int> p;
    auto f = p.getFuture();

    Xception x;
    x.errorCode = 32;
    x.message = "test";

    p.setException(x);

    return std::move(f);
  }
};

std::shared_ptr<ThriftServer> getServer() {
  auto server = std::make_shared<ThriftServer>();
  server->setPort(0);
  server->setInterface(std::unique_ptr<TestInterface>(new TestInterface));
  server->setSaslEnabled(true);
  server->setSaslServerFactory(
    [] (apache::thrift::async::TEventBase* evb) {
      return std::unique_ptr<SaslServer>(new StubSaslServer(evb));
    }
  );
  return server;
}

void AsyncCpp2Test(bool enable_security) {
  TEventBase base;

  auto port = Server::get(getServer)->getAddress().getPort();
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  auto client_channel = HeaderClientChannel::newChannel(socket);
  if (enable_security) {
    client_channel->getHeader()->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
    client_channel->setSaslClient(std::unique_ptr<SaslClient>(
      new StubSaslClient(socket->getEventBase())
    ));
  }
  FutureServiceAsyncClient client(std::move(client_channel));

  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->setTimeout(10000);
  client.sendResponse([](ClientReceiveState&& state) {
                        std::string response;
                        try {
                          FutureServiceAsyncClient::recv_sendResponse(
                              response, state);
                        } catch(const std::exception& ex) {
                        }
                        EXPECT_EQ(response, "test64");
                      },
                      64);
  base.loop();
}

TEST(ThriftServer, FutureExceptions) {
  TEventBase base;

  auto port = Server::get(getServer)->getAddress().getPort();
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  FutureServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));
  auto f = client.future_throwing();

  while (!f.isReady()) {
    base.loop();
  }

  EXPECT_THROW(f.value(), Xception);
}

TEST(ThriftServer, FutureClientTest) {
  using std::chrono::steady_clock;

  TEventBase base;
  TEventBaseExecutor e(&base);

  auto gate = GenericThreadGate<Executor*, Executor*, TEventBaseExecutor*>(
    nullptr, nullptr, &e);

  auto port = Server::get(getServer)->getAddress().getPort();
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  FutureServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->setTimeout(10000);

  // only once you call wait() should you start looping,
  // so the wait time in value() (which calls wait()) should
  // be much higher than the time for future_sendResponse(0)
  steady_clock::time_point start = steady_clock::now();

  auto future = client.future_sendResponse(1000);
  steady_clock::time_point sent = steady_clock::now();

  auto value = gate.value(future);
  steady_clock::time_point got = steady_clock::now();

  EXPECT_EQ(value, "test1000");

  steady_clock::duration sentTime = sent - start;
  steady_clock::duration waitTime = got - sent;

  int factor = 2;
  EXPECT_GE(waitTime, factor * sentTime);

  auto len = client.future_sendResponse(64).then(
    [](facebook::wangle::Try<std::string>&& response) {
      EXPECT_TRUE(response.hasValue());
      EXPECT_EQ(response.value(), "test64");
      return response.value().size();
    }
  );

  EXPECT_EQ(gate.value(len), 6);

  RpcOptions options;
  options.setTimeout(std::chrono::milliseconds(1));
  try {
    // should timeout
    auto f = client.future_sendResponse(options, 10000);

    // Wait for future to finish
    gate.value(f);
    EXPECT_EQ(true, false);
  } catch (...) {
    return;
  }
}

// Needs wait()
TEST(ThriftServer, FutureGetOrderTest) {
  using std::chrono::steady_clock;

  TEventBase base;
  TEventBaseExecutor e(&base);

  auto gate = GenericThreadGate<Executor*, Executor*, TEventBaseExecutor*>(
    nullptr, nullptr, &e);

  auto port = Server::get(getServer)->getAddress().getPort();
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  FutureServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  boost::polymorphic_downcast<HeaderClientChannel*>(
    client.getChannel())->setTimeout(10000);

  auto future0 = client.future_sendResponse(0);
  auto future1 = client.future_sendResponse(10);
  auto future2 = client.future_sendResponse(20);
  auto future3 = client.future_sendResponse(30);
  auto future4 = client.future_sendResponse(40);

  steady_clock::time_point start = steady_clock::now();

  EXPECT_EQ(gate.value(future3), "test30");
  steady_clock::time_point sent = steady_clock::now();
  EXPECT_EQ(gate.value(future4), "test40");
  EXPECT_EQ(gate.value(future0), "test0");
  EXPECT_EQ(gate.value(future2), "test20");
  EXPECT_EQ(gate.value(future1), "test10");
  steady_clock::time_point gets = steady_clock::now();

  steady_clock::duration sentTime = sent - start;
  steady_clock::duration getsTime = gets - sent;

  int factor = 2;
  EXPECT_GE(sentTime, factor * getsTime);
}

TEST(ThriftServer, OnewayFutureClientTest) {
  using std::chrono::steady_clock;

  TEventBase base;

  auto port = Server::get(getServer)->getAddress().getPort();
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, "127.0.0.1", port));

  FutureServiceAsyncClient client(
    std::unique_ptr<HeaderClientChannel,
                    apache::thrift::async::TDelayedDestruction::Destructor>(
                      new HeaderClientChannel(socket)));

  auto future = client.future_noResponse(1000);
  steady_clock::time_point sent = steady_clock::now();

  // wait for future to finish.
  base.loop();
  steady_clock::time_point waited = steady_clock::now();

  future.value();
  steady_clock::time_point got = steady_clock::now();

  steady_clock::duration waitTime = waited - sent;
  steady_clock::duration gotTime = got - waited;

  int factor = 1;
  EXPECT_GE(waitTime, factor * gotTime);
}

TEST(ThriftServer, ThreadGateTest) {
  using std::chrono::steady_clock;

  std::atomic<TEventBase*> atomicEventBasePtr(nullptr);
  std::atomic<FutureServiceAsyncClient*> atomicClientPtr(nullptr);
  std::atomic<bool> ready(false);

  std::thread([&](){
    TEventBase eventBase;
    atomicEventBasePtr = &eventBase;

    auto port = Server::get(getServer)->getAddress().getPort();
    std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&eventBase, "127.0.0.1", port));

    FutureServiceAsyncClient client(
      std::unique_ptr<HeaderClientChannel,
                      apache::thrift::async::TDelayedDestruction::Destructor>(
                        new HeaderClientChannel(socket)));
    atomicClientPtr = &client;

    boost::polymorphic_downcast<HeaderClientChannel*>(
      client.getChannel())->setTimeout(10000);

    ready = true;

    EXPECT_NO_THROW(eventBase.loopForever());
  }).detach();

  // Wait for base to start
  while (!ready) {
  }

  TEventBase* eventBase = atomicEventBasePtr.load();
  FutureServiceAsyncClient* client = atomicClientPtr.load();

  TEventBaseExecutor eastExecutor(eventBase);
  ManualExecutor westExecutor;

  auto gate = GenericThreadGate<Executor*, Executor*, ManualExecutor*>(
    &westExecutor, &eastExecutor, &westExecutor);

  auto future0 = client->future_sendResponse(&gate, 0);
  auto future1 = client->future_sendResponse(&gate, 10);
  auto future2 = client->future_sendResponse(&gate, 20);
  auto future3 = client->future_sendResponse(&gate, 30);
  auto future4 = client->future_sendResponse(&gate, 40);

  steady_clock::time_point start = steady_clock::now();

  EXPECT_EQ(gate.value(future3), "test30");

  steady_clock::time_point sent = steady_clock::now();

  EXPECT_EQ(gate.value(future4), "test40");
  EXPECT_EQ(gate.value(future0), "test0");
  EXPECT_EQ(gate.value(future2), "test20");
  EXPECT_EQ(gate.value(future1), "test10");

  steady_clock::time_point gets = steady_clock::now();

  steady_clock::duration sentTime = sent - start;
  steady_clock::duration getsTime = gets - sent;

  int factor = 2;
  EXPECT_GE(sentTime, factor * getsTime);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}
