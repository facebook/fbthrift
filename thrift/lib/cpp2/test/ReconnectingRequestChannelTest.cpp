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

#include <thrift/lib/cpp2/async/ReconnectingRequestChannel.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/test/ScopedBoundPort.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

using namespace testing;
using namespace apache::thrift;
using namespace apache::thrift::test;
using apache::thrift::transport::TTransportException;
using folly::AsyncSocket;

class TestServiceServerMock : public TestServiceSvIf {
 public:
  MOCK_METHOD1(echoInt, int32_t(int32_t));
  MOCK_METHOD1(noResponse, void(int64_t));
  MOCK_METHOD2(range, apache::thrift::ServerStream<int32_t>(int32_t, int32_t));
};

class ReconnectingRequestChannelTest : public Test {
 public:
  folly::EventBase* eb{folly::EventBaseManager::get()->getEventBase()};
  folly::ScopedBoundPort bound;
  std::shared_ptr<TestServiceServerMock> handler{
      std::make_shared<TestServiceServerMock>()};
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> runner{
      std::make_unique<apache::thrift::ScopedServerInterfaceThread>(handler)};

  folly::SocketAddress up_addr{runner->getAddress()};
  folly::SocketAddress dn_addr{bound.getAddress()};
  uint32_t connection_count_ = 0;

  void runReconnect(TestServiceAsyncClient& client, bool testStreaming);
};

TEST_F(ReconnectingRequestChannelTest, ReconnectHeader) {
  auto channel = ReconnectingRequestChannel::newChannel(
      *eb, [this](folly::EventBase& eb) mutable {
        connection_count_++;
        return HeaderClientChannel::newChannel(
            AsyncSocket::newSocket(&eb, up_addr));
      });
  TestServiceAsyncClient client(std::move(channel));
  runReconnect(client, false);
}

TEST_F(ReconnectingRequestChannelTest, ReconnectRocket) {
  auto channel = ReconnectingRequestChannel::newChannel(
      *eb, [this](folly::EventBase& eb) mutable {
        connection_count_++;
        return RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
            new folly::AsyncSocket(&eb, up_addr)));
      });
  TestServiceAsyncClient client(std::move(channel));
  runReconnect(client, true);
}

void ReconnectingRequestChannelTest::runReconnect(
    TestServiceAsyncClient& client,
    bool testStreaming) {
  EXPECT_CALL(*handler, echoInt(_))
      .WillOnce(Return(1))
      .WillOnce(Return(3))
      .WillOnce(Return(4));
  EXPECT_EQ(client.sync_echoInt(1), 1);
  EXPECT_EQ(connection_count_, 1);

  EXPECT_CALL(*handler, noResponse(_));
  client.sync_noResponse(0);
  EXPECT_EQ(connection_count_, 1);

  auto checkStream = [](auto&& stream, int from, int to) {
    std::move(stream).subscribeInline([idx = from, to](auto nextTry) mutable {
      DCHECK(!nextTry.hasException());
      if (!nextTry.hasValue()) {
        EXPECT_EQ(to, idx);
      } else {
        EXPECT_EQ(idx++, *nextTry);
      }
    });
  };

  if (testStreaming) {
    EXPECT_CALL(*handler, range(_, _))
        .WillRepeatedly(Invoke(
            [](int32_t from,
               int32_t to) -> apache::thrift::ServerStream<int32_t> {
              auto [serverStream, publisher] =
                  apache::thrift::ServerStream<int32_t>::createPublisher();
              for (auto idx = from; idx < to; ++idx) {
                publisher.next(idx);
              }
              std::move(publisher).complete();
              return std::move(serverStream);
            }));
    checkStream(client.sync_range(0, 1), 0, 1);
    EXPECT_EQ(connection_count_, 1);
  }

  // bounce the server
  runner =
      std::make_unique<apache::thrift::ScopedServerInterfaceThread>(handler);
  up_addr = runner->getAddress();

  EXPECT_THROW(client.sync_echoInt(2), TTransportException);
  EXPECT_EQ(client.sync_echoInt(3), 3);
  EXPECT_EQ(connection_count_, 2);
  EXPECT_EQ(client.sync_echoInt(4), 4);
  EXPECT_EQ(connection_count_, 2);

  if (testStreaming) {
    checkStream(client.sync_range(0, 2), 0, 2);
    EXPECT_EQ(connection_count_, 2);
    checkStream(client.sync_range(4, 42), 4, 42);
    EXPECT_EQ(connection_count_, 2);
  }
}
