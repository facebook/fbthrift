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

#include <gtest/gtest.h>

#include <folly/experimental/coro/Sleep.h>
#include <thrift/lib/cpp2/async/tests/util/Util.h>

using namespace testutil::testservice;
using namespace apache::thrift;

template <typename Service>
class StreamServiceTest
    : public AsyncTestSetup<Service, TestStreamServiceAsyncClient> {};

using TestTypes =
    ::testing::Types<TestStreamGeneratorService, TestStreamPublisherService>;
TYPED_TEST_CASE(StreamServiceTest, TestTypes);

TYPED_TEST(StreamServiceTest, Basic) {
  this->connectToServer(
      [](TestStreamServiceAsyncClient& client) -> folly::coro::Task<void> {
        auto gen = (co_await client.co_range(0, 100)).toAsyncGenerator();
        int i = 0;
        while (auto t = co_await gen.next()) {
          if (i <= 100) {
            EXPECT_EQ(i++, *t);
          } else {
            EXPECT_FALSE(t);
          }
        }
      });
}

TYPED_TEST(StreamServiceTest, Throw) {
  this->connectToServer(
      [](TestStreamServiceAsyncClient& client) -> folly::coro::Task<void> {
        auto gen = (co_await client.co_rangeThrow(0, 100)).toAsyncGenerator();
        for (int i = 0; i <= 100; i++) {
          auto t = co_await gen.next();
          EXPECT_EQ(i, *t);
        }
        EXPECT_ANY_THROW(co_await gen.next());
      });
}

TYPED_TEST(StreamServiceTest, ThrowUDE) {
  this->connectToServer(
      [](TestStreamServiceAsyncClient& client) -> folly::coro::Task<void> {
        auto gen =
            (co_await client.co_rangeThrowUDE(0, 100)).toAsyncGenerator();
        for (int i = 0; i <= 100; i++) {
          auto t = co_await gen.next();
          EXPECT_EQ(i, *t);
        }
        EXPECT_ANY_THROW(co_await gen.next());
      });
}

TYPED_TEST(StreamServiceTest, ServerTimeout) {
  this->server_->setStreamExpireTime(std::chrono::milliseconds(1));
  this->connectToServer(
      [](TestStreamServiceAsyncClient& client) -> folly::coro::Task<void> {
        auto gen = (co_await client.co_range(0, 100)).toAsyncGenerator();
        co_await folly::coro::sleep(std::chrono::milliseconds(100));
        EXPECT_THROW(while (co_await gen.next()){}, TApplicationException);
      });
}
