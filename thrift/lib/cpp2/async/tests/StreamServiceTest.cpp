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

#include <thrift/lib/cpp2/async/tests/util/Util.h>

using namespace testutil::testservice;
using namespace apache::thrift;

auto doTest = [](auto& client) -> folly::coro::Task<void> {
  auto gen = (co_await client.co_range(0, 100)).toAsyncGenerator();
  co_await([&]() mutable -> folly::coro::Task<void> {
    int i = 0;
    while (auto t = co_await gen.next()) {
      if (i <= 100) {
        EXPECT_EQ(i++, *t);
      } else {
        EXPECT_FALSE(t);
      }
    }
  }());

  gen = (co_await client.co_rangeThrow(0, 100)).toAsyncGenerator();
  co_await[&]() mutable->folly::coro::Task<void> {
    for (int i = 0; i <= 101; i++) {
      if (i <= 100) {
        auto t = co_await gen.next();
        EXPECT_EQ(i, *t);
      } else {
        EXPECT_ANY_THROW(co_await gen.next());
      }
    }
  }
  ();

  gen = (co_await client.co_rangeThrowUDE(0, 100)).toAsyncGenerator();
  co_await[&]() mutable->folly::coro::Task<void> {
    for (int i = 0; i <= 101; i++) {
      if (i <= 100) {
        auto t = co_await gen.next();
        EXPECT_EQ(i, *t);
      } else {
        EXPECT_ANY_THROW(co_await gen.next());
      }
    }
  }
  ();
};

struct StreamGeneratorServiceTest : public testing::Test,
                                    public TestSetup<
                                        TestStreamGeneratorService,
                                        TestStreamServiceAsyncClient> {};

struct StreamPublisherServiceTest : public testing::Test,
                                    public TestSetup<
                                        TestStreamPublisherService,
                                        TestStreamServiceAsyncClient> {};

TEST_F(StreamGeneratorServiceTest, Stream) {
  connectToServer(doTest);
}

TEST_F(StreamPublisherServiceTest, Stream) {
  connectToServer(doTest);
}
