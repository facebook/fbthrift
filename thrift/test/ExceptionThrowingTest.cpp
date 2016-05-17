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

#include <folly/ExceptionWrapper.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/async/THeaderAsyncChannel.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <thrift/test/gen-cpp/ExceptionThrowingService.h>
#include <thrift/test/gen-cpp2/ExceptionThrowingService.h>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::util;
using folly::EventBase;
using thrift::test::ExceptionThrowingServiceCobClient;
using thrift::test::SimpleException;
using thrift::test::cpp2::ExceptionThrowingServiceAsyncClient;
using thrift::test::cpp2::ExceptionThrowingServiceSvIf;

class ExceptionThrowingHandler : public ExceptionThrowingServiceSvIf {
 public:
  void echo(std::string& ret, std::unique_ptr<std::string> req) override {
    ret = *req;
  }

  void throwException(std::unique_ptr<std::string> msg) override {
    SimpleException e;
    e.message = *msg;
    throw e;
  }
};

std::unique_ptr<ScopedServerThread> createThriftServer() {
  auto server = std::make_shared<ThriftServer>();
  server->setPort(0);
  server->setInterface(std::unique_ptr<ExceptionThrowingHandler>(
    new ExceptionThrowingHandler));
  return folly::make_unique<ScopedServerThread>(server);
}

TEST(ExceptionThrowingTest, Thrift1Client) {
  EventBase eb;
  auto serverThread = createThriftServer();
  auto* serverAddr = serverThread->getAddress();
  std::shared_ptr<TAsyncSocket> socket(TAsyncSocket::newSocket(
      &eb, *serverAddr));
  std::shared_ptr<THeaderAsyncChannel> headerChannel(
      THeaderAsyncChannel::newChannel(socket));
  auto clientDuplexProtocolFactory =
      std::make_shared<THeaderProtocolFactory>();
  ExceptionThrowingServiceCobClient client(
      headerChannel, clientDuplexProtocolFactory.get());

  // Verify that recv_echo works
  bool echoDone = false;
  client.echo(
    [&echoDone] (ExceptionThrowingServiceCobClient* client) {
      try {
        std::string result;
        client->recv_echo(result);
        EXPECT_EQ(result, "Hello World");
        echoDone = true;
      } catch (const std::exception& e) {
      }
    },
    "Hello World"
  );
  eb.loop();
  EXPECT_TRUE(echoDone);

  // Verify that recv_wrapped_echo works
  echoDone = false;
  client.echo(
    [&echoDone] (ExceptionThrowingServiceCobClient* client) {
      std::string result;
      auto ew = client->recv_wrapped_echo(result);
      if (!ew) {
        EXPECT_EQ(result, "Hello World");
        echoDone = true;
      }
    },
    "Hello World"
  );
  eb.loop();
  EXPECT_TRUE(echoDone);

  // recv_throwException
  bool exceptionThrown = false;
  client.throwException(
    [&exceptionThrown] (ExceptionThrowingServiceCobClient* client) {
      try {
        client->recv_throwException();
      } catch (const SimpleException& e) {
        EXPECT_EQ(e.message, "Hello World");
        exceptionThrown = true;
      }
    },
    "Hello World"
  );
  eb.loop();
  EXPECT_TRUE(exceptionThrown);

  // recv_wrapped_throwException
  exceptionThrown = false;
  client.throwException(
    [&exceptionThrown] (ExceptionThrowingServiceCobClient* client) {
      auto ew = client->recv_wrapped_throwException();
      if (ew && ew.with_exception(
        [] (const SimpleException& e) {
          EXPECT_EQ(e.message, "Hello World");
      })) {
        exceptionThrown = true;
      }
    },
    "Hello World"
  );
  eb.loop();
  EXPECT_TRUE(exceptionThrown);
}

TEST(ExceptionThrowingTest, Thrift2Client) {
  EventBase eb;
  auto serverThread = createThriftServer();
  auto* serverAddr = serverThread->getAddress();
  std::shared_ptr<TAsyncSocket> socket(TAsyncSocket::newSocket(
      &eb, *serverAddr));
  auto channel = HeaderClientChannel::newChannel(socket);
  ExceptionThrowingServiceAsyncClient client(std::move(channel));

  // Verify that recv_echo works
  bool echoDone = false;
  client.echo(
    [&echoDone] (ClientReceiveState&& state) {
      EXPECT_FALSE(state.exceptionWrapper());
      try {
        std::string result;
        ExceptionThrowingServiceAsyncClient::recv_echo(result, state);
        EXPECT_EQ(result, "Hello World");
        echoDone = true;
      } catch (const std::exception& e) {
      }
    },
    "Hello World"
  );
  eb.loop();
  EXPECT_TRUE(echoDone);

  // Verify that recv_wrapped_echo works
  echoDone = false;
  client.echo(
    [&echoDone] (ClientReceiveState&& state) {
      EXPECT_FALSE(state.exceptionWrapper());
      std::string result;
      auto ew = ExceptionThrowingServiceAsyncClient::recv_wrapped_echo(
        result, state);
      if (!ew) {
        EXPECT_EQ(result, "Hello World");
        echoDone = true;
      }
    },
    "Hello World"
  );
  eb.loop();
  EXPECT_TRUE(echoDone);

  // recv_throwException
  bool exceptionThrown = false;
  client.throwException(
    [&exceptionThrown] (ClientReceiveState&& state) {
      EXPECT_FALSE(state.exceptionWrapper());
      try {
        ExceptionThrowingServiceAsyncClient::recv_throwException(state);
      } catch (const SimpleException& e) {
        EXPECT_EQ(e.message, "Hello World");
        exceptionThrown = true;
      }
    },
    "Hello World"
  );
  eb.loop();
  EXPECT_TRUE(exceptionThrown);

  // recv_wrapped_throwException
  exceptionThrown = false;
  client.throwException(
    [&exceptionThrown] (ClientReceiveState&& state) {
      EXPECT_FALSE(state.exceptionWrapper());
      auto ew =
        ExceptionThrowingServiceAsyncClient::recv_wrapped_throwException(state);
      if (ew && ew.with_exception(
        [] (const SimpleException& e) {
          EXPECT_EQ(e.message, "Hello World");
      })) {
        exceptionThrown = true;
      }
    },
    "Hello World"
  );
  eb.loop();
  EXPECT_TRUE(exceptionThrown);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
