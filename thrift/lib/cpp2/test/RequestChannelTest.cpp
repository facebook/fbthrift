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

#include <thrift/lib/cpp2/async/RequestChannel.h>

#include <memory>
#include <folly/Memory.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncServerSocket.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::transport;
using namespace testing;

using CSR = ClientReceiveState;

//  This binds to an ephemeral port but does not listen.
//  We therefore know at least one port which is guaranteed not to be listening.
//  Useful for testing server-down cases.
class PortHolder {
 public:
  PortHolder() {
    th_ = thread([&]{ eb_.loopForever(); });
    eb_.waitUntilRunning();
    sock_ = TAsyncServerSocket::newSocket(&eb_);
    sock_->bind(0);
  }
  ~PortHolder() {
    eb_.terminateLoopSoon();
    th_.join();
  }
  folly::SocketAddress getAddress() {
    folly::SocketAddress ret;
    sock_->getAddress(&ret);
    return ret;
  }
 private:
  EventBase eb_;
  thread th_;
  shared_ptr<TAsyncServerSocket> sock_;
};

class TestServiceServerMock : public TestServiceSvIf {
 public:
  MOCK_METHOD1(noResponse, void(int64_t));
};

class FunctionSendCallbackTest : public Test {
 public:
  unique_ptr<TestServiceAsyncClient> getClient(
      const folly::SocketAddress& addr) {
    return make_unique<TestServiceAsyncClient>(
      HeaderClientChannel::newChannel(TAsyncSocket::newSocket(&eb, addr)));
  }
  void sendOnewayMessage(
      const folly::SocketAddress& addr,
      function<void(ClientReceiveState&&)> cb) {
    auto client = getClient(addr);
    client->noResponse(make_unique<FunctionSendCallback>(move(cb)),
                       68 /* without loss of generality */);
    eb.loop();
  }
  EventBase eb;
};

TEST_F(FunctionSendCallbackTest, with_missing_server_fails) {
  PortHolder ph;
  exception_wrapper exn;
  sendOnewayMessage(ph.getAddress(), [&](CSR&& state) {
      exn = state.exceptionWrapper();
  });
  EXPECT_TRUE(bool(exn));
  auto err = "transport is closed in write()";
  EXPECT_NE(string::npos, exn.what().find(err));
}

TEST_F(FunctionSendCallbackTest, with_throwing_server_passes) {
  auto si = make_shared<TestServiceServerMock>();
  ScopedServerInterfaceThread ssit(si);
  EXPECT_CALL(*si, noResponse(_)).WillOnce(Throw(runtime_error("hi")));
  exception_wrapper exn = make_exception_wrapper<runtime_error>("lo");
  sendOnewayMessage(ssit.getAddress(), [&](CSR&& state) {
      exn = state.exceptionWrapper();
  });
  EXPECT_FALSE(exn);
}
