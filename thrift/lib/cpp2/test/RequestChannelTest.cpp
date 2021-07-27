/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/async/RequestChannel.h>

#include <memory>
#include <thread>

#include <folly/Memory.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/test/ScopedBoundPort.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace std;
using namespace std::chrono;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;
using namespace testing;

using CSR = ClientReceiveState;

class TestServiceServerMock : public TestServiceSvIf {
 public:
  MOCK_METHOD(void, noResponse, (int64_t), (override));
  MOCK_METHOD(void, voidResponse, (), (override));
};

class FunctionSendRecvRequestCallbackTest : public Test {
 public:
  EventBase* eb{EventBaseManager::get()->getEventBase()};
  ScopedBoundPort bound;
  shared_ptr<TestServiceServerMock> handler{
      make_shared<TestServiceServerMock>()};
  ScopedServerInterfaceThread runner{handler};

  unique_ptr<TestServiceAsyncClient> newClient(SocketAddress const& addr) {
    return make_unique<TestServiceAsyncClient>(HeaderClientChannel::newChannel(
        HeaderClientChannel::WithoutRocketUpgrade{},
        AsyncSocket::newSocket(eb, addr)));
  }

  exception_wrapper ew;
  ClientReceiveState state;

  unique_ptr<FunctionSendRecvRequestCallback> newCallback() {
    return make_unique<FunctionSendRecvRequestCallback>(
        [&](auto&& _) { ew = std::move(_); },
        [&](auto&& _) { state = std::move(_); });
  }
};

TEST_F(FunctionSendRecvRequestCallbackTest, 1w_send_failure) {
  auto client = newClient(bound.getAddress());
  client->noResponse(newCallback(), 68 /* a random number */);
  eb->loop();
  EXPECT_TRUE(ew.with_exception([](TTransportException const& ex) {
    EXPECT_EQ(TTransportException::UNKNOWN, ex.getType());
    EXPECT_STREQ("transport is closed in write()", ex.what());
  }));
  EXPECT_FALSE(state.hasResponseBuffer());
}

TEST_F(FunctionSendRecvRequestCallbackTest, 1w_send_success) {
  auto client = newClient(runner.getAddress());
  client->noResponse(newCallback(), 68 /* a random number */);
  eb->loop();
  EXPECT_FALSE(bool(ew));
  EXPECT_FALSE(state.hasResponseBuffer());
}

TEST_F(FunctionSendRecvRequestCallbackTest, 2w_send_failure) {
  auto client = newClient(bound.getAddress());
  client->voidResponse(newCallback());
  eb->loop();
  EXPECT_TRUE(ew.with_exception([](TTransportException const& ex) {
    EXPECT_EQ(TTransportException::NOT_OPEN, ex.getType());
  }));
  EXPECT_FALSE(state.hasResponseBuffer());
}

TEST_F(FunctionSendRecvRequestCallbackTest, 2w_recv_failure) {
  auto client = newClient(runner.getAddress());
  RpcOptions opts;
  opts.setTimeout(milliseconds(20));
  auto done = make_shared<Baton<>>();
  SCOPE_EXIT { done->post(); };
  EXPECT_CALL(*handler, voidResponse()).WillOnce(Invoke([done] {
    EXPECT_TRUE(done->try_wait_for(seconds(1)));
  }));
  client->voidResponse(opts, newCallback());
  eb->loop();
  EXPECT_FALSE(bool(ew));
  ew = std::move(state.exception());
  EXPECT_TRUE(ew.with_exception([](TTransportException const& ex) {
    EXPECT_EQ(TTransportException::TIMED_OUT, ex.getType());
  }));
  EXPECT_FALSE(state.hasResponseBuffer());
}

TEST_F(FunctionSendRecvRequestCallbackTest, 2w_recv_success) {
  auto client = newClient(runner.getAddress());
  RpcOptions opts;
  opts.setTimeout(milliseconds(20));
  EXPECT_CALL(*handler, voidResponse());
  client->voidResponse(opts, newCallback());
  eb->loop();
  EXPECT_FALSE(bool(ew));
  ew = std::move(state.exception());
  EXPECT_FALSE(bool(ew));
  EXPECT_TRUE(state.hasResponseBuffer());
}

class FunctionSendCallbackTest : public Test {
 public:
  unique_ptr<TestServiceAsyncClient> getClient(
      const folly::SocketAddress& addr) {
    return make_unique<TestServiceAsyncClient>(HeaderClientChannel::newChannel(
        HeaderClientChannel::WithoutRocketUpgrade{},
        AsyncSocket::newSocket(&eb, addr)));
  }
  void sendOnewayMessage(
      const folly::SocketAddress& addr,
      function<void(ClientReceiveState&&)> cb) {
    auto client = getClient(addr);
    client->noResponse(
        make_unique<FunctionSendCallback>(move(cb)),
        68 /* without loss of generality */);
    eb.loop();
  }
  EventBase eb;
};

TEST_F(FunctionSendCallbackTest, with_missing_server_fails) {
  ScopedBoundPort bound;
  exception_wrapper exn;
  sendOnewayMessage(bound.getAddress(), [&](CSR&& state) {
    exn = std::move(state.exception());
  });
  EXPECT_TRUE(bool(exn));
  auto err = "transport is closed in write()";
  EXPECT_NE(string::npos, exn.what().find(err));
}

TEST_F(FunctionSendCallbackTest, with_throwing_server_passes) {
  auto si = make_shared<TestServiceServerMock>();
  ScopedServerInterfaceThread ssit(si);
  Baton<> done;
  EXPECT_CALL(*si, noResponse(_))
      .WillOnce(DoAll(
          Invoke([&](int64_t) { done.post(); }), Throw(runtime_error("hi"))));
  exception_wrapper exn = make_exception_wrapper<runtime_error>("lo");
  sendOnewayMessage(ssit.getAddress(), [&](CSR&& state) {
    exn = std::move(state.exception());
  });
  done.try_wait_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(exn);
}
