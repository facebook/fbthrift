/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <gtest/gtest.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/async/GuardedRequestChannel.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace testing;
using namespace apache::thrift;
using namespace apache::thrift::test;
using folly::AsyncSocket;

class TestServiceServerMock
    : public apache::thrift::ServiceHandler<TestService> {
 public:
  MOCK_METHOD(int32_t, echoInt, (int32_t), (override));
  MOCK_METHOD(
      folly::SemiFuture<std::unique_ptr<std::string>>,
      semifuture_echoRequest,
      (std::unique_ptr<std::string>),
      (override));
  MOCK_METHOD(
      folly::SemiFuture<ServerStream<int8_t>>,
      semifuture_echoIOBufAsByteStream,
      (std::unique_ptr<folly::IOBuf>, int32_t),
      (override));
};

class GuardedRequestChannelTest : public Test {
 protected:
  std::shared_ptr<TestServiceServerMock> handler{
      std::make_shared<TestServiceServerMock>()};
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> runner{
      std::make_unique<apache::thrift::ScopedServerInterfaceThread>(handler)};
  folly::SocketAddress serverAddress{runner->getAddress()};
};

TEST_F(GuardedRequestChannelTest, normalSingleRequestSuccess) {
  auto evbThread = std::make_shared<folly::ScopedEventBaseThread>();
  auto pooledChannel = PooledRequestChannel::newChannel(
      evbThread->getEventBase(), evbThread, [&](folly::EventBase& evb) {
        auto socket =
            AsyncSocket::UniquePtr(new AsyncSocket(&evb, serverAddress));
        auto rocketChannel = RocketClientChannel::newChannel(std::move(socket));
        return rocketChannel;
      });

  auto guardedChannel =
      GuardedRequestChannel<folly::Unit, folly::Unit>::newChannel(
          std::move(pooledChannel));

  apache::thrift::Client<TestService> client(std::move(guardedChannel));
  EXPECT_CALL(*handler, echoInt(_)).WillOnce(Return(1)).WillOnce(Return(2));
  EXPECT_EQ(client.sync_echoInt(1), 1);
  EXPECT_EQ(client.sync_echoInt(2), 2);
}
