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

#include <gtest/gtest.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/FutureRequest.h>
#include <thrift/lib/cpp2/test/gen-cpp2/FutureService.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>

#include <thrift/lib/cpp2/async/StubSaslClient.h>
#include <thrift/lib/cpp2/async/StubSaslServer.h>

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <atomic>

#include <folly/Executor.h>
#include <folly/wangle/futures/ManualExecutor.h>
#include "common/concurrency/Executor.h"

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace folly::wangle;
using namespace folly;
using facebook::concurrency::TEventBaseExecutor;

template <class T, class W>
T& waitFor(Future<T>& f, W& w) {
  while (!f.isReady()) {
    w.makeProgress();
  }
  return f.value();
}

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

    RequestEventBase::get()->runInEventBaseThread([this, func, size](){
      RequestEventBase::get()->runAfterDelay(func, size);
    });

    return std::move(f);
  }

  Future<void> future_noResponse(int64_t size) {
    MoveWrapper<Promise<void>> p;
    auto f = p->getFuture();

    auto func = std::function<void()>([=]() mutable {
      p->setValue();
    });
    RequestEventBase::get()->runInEventBaseThread([this, func, size](){
      RequestEventBase::get()->runAfterDelay(func, size);
    });
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
  ScopedServerThread sst(getServer());
  TEventBase base;

  auto port = sst.getAddress()->getPort();
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
  ScopedServerThread sst(getServer());
  TEventBase base;

  auto port = sst.getAddress()->getPort();
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

  ScopedServerThread sst(getServer());
  TEventBase base;
  TEventBaseExecutor e(&base);

  auto port = sst.getAddress()->getPort();
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

  auto value = waitFor(future, e);
  steady_clock::time_point got = steady_clock::now();

  EXPECT_EQ(value, "test1000");

  steady_clock::duration sentTime = sent - start;
  steady_clock::duration waitTime = got - sent;

  int factor = 2;
  EXPECT_GE(waitTime, factor * sentTime);

  auto len = client.future_sendResponse(64).then(
    [](folly::wangle::Try<std::string>&& response) {
      EXPECT_TRUE(response.hasValue());
      EXPECT_EQ(response.value(), "test64");
      return response.value().size();
    }
  );

  EXPECT_EQ(waitFor(len, e), 6);

  RpcOptions options;
  options.setTimeout(std::chrono::milliseconds(1));
  try {
    // should timeout
    auto f = client.future_sendResponse(options, 10000);

    // Wait for future to finish
    waitFor(f, e);
    EXPECT_EQ(true, false);
  } catch (...) {
    return;
  }
}

// Needs wait()
TEST(ThriftServer, FutureGetOrderTest) {
  using std::chrono::steady_clock;

  ScopedServerThread sst(getServer());
  TEventBase base;
  TEventBaseExecutor e(&base);

  auto port = sst.getAddress()->getPort();
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

  EXPECT_EQ(waitFor(future3, e), "test30");
  steady_clock::time_point sent = steady_clock::now();
  EXPECT_EQ(waitFor(future4, e), "test40");
  EXPECT_EQ(waitFor(future0, e), "test0");
  EXPECT_EQ(waitFor(future2, e), "test20");
  EXPECT_EQ(waitFor(future1, e), "test10");
  steady_clock::time_point gets = steady_clock::now();

  steady_clock::duration sentTime = sent - start;
  steady_clock::duration getsTime = gets - sent;

  int factor = 2;
  EXPECT_GE(sentTime, factor * getsTime);
}

TEST(ThriftServer, OnewayFutureClientTest) {
  using std::chrono::steady_clock;

  ScopedServerThread sst(getServer());
  TEventBase base;

  auto port = sst.getAddress()->getPort();
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

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}
