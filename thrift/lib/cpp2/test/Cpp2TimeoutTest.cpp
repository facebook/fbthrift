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
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>

#include <thrift/lib/cpp2/async/StubSaslClient.h>
#include <thrift/lib/cpp2/async/StubSaslServer.h>

#include <boost/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::util;
using namespace apache::thrift::async;

class TestInterface : public TestServiceSvIf {
  void sendResponse(std::string& _return, int64_t size) {
    if (size >= 0) {
      usleep(size * 1000);
    }

    _return = "test";
  }

  void noResponse(int64_t size) {
    usleep(size * 1000);
  }
};

std::shared_ptr<ThriftServer> getServer() {
  std::shared_ptr<ThriftServer> server = std::make_shared<ThriftServer>();
  server->setPort(0);
  server->setInterface(std::unique_ptr<TestInterface>(new TestInterface));
  server->setIdleTimeout(std::chrono::milliseconds(10));
  return server;
}

class CloseChecker : public CloseCallback {
 public:
  CloseChecker() : closed_(false) {}
  void channelClosed() {
    closed_ = true;
  }
  bool getClosed() {
    return closed_;
  }
 private:
  bool closed_;
};

TEST(ThriftServer, IdleTimeoutTest) {
  ScopedServerThread sst(getServer());

  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  auto client_channel = HeaderClientChannel::newChannel(socket);
  CloseChecker checker;
  client_channel->setCloseCallback(&checker);
  base.tryRunAfterDelay([&base](){
      base.terminateLoopSoon();
    }, 100);
  base.loopForever();
  EXPECT_TRUE(checker.getClosed());
  client_channel->setCloseCallback(nullptr);
}

TEST(ThriftServer, NoIdleTimeoutWhileWorkingTest) {
  ScopedServerThread sst(getServer());

  TEventBase base;
  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  auto client_channel = HeaderClientChannel::newChannel(socket);
  auto client_channelp = client_channel.get();
  CloseChecker checker;

  client_channel->setCloseCallback(&checker);
  TestServiceAsyncClient client(std::move(client_channel));
  std::string ret;
  client.sync_sendResponse(ret, 20);

  client.sync_sendResponse(ret, 20);

  EXPECT_FALSE(checker.getClosed());
  client_channelp->setCloseCallback(nullptr);
}

TEST(ThriftServer, IdleTimeoutAfterTest) {
  ScopedServerThread sst(getServer());

  TEventBase base;

  std::shared_ptr<TAsyncSocket> socket(
    TAsyncSocket::newSocket(&base, *sst.getAddress()));

  auto client_channel = HeaderClientChannel::newChannel(socket);
  auto client_channelp = client_channel.get();
  CloseChecker checker;

  client_channel->setCloseCallback(&checker);
  TestServiceAsyncClient client(std::move(client_channel));
  std::string ret;
  client.sync_sendResponse(ret, 20);

  EXPECT_FALSE(checker.getClosed());

  base.tryRunAfterDelay([&base](){
      base.terminateLoopSoon();
    }, 200);
  base.loopForever();
  EXPECT_TRUE(checker.getClosed());
  client_channelp->setCloseCallback(nullptr);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}
