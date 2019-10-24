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

namespace apache {
namespace thrift {

using namespace testutil::testservice;

class SinkServiceTest : public testing::Test, public TestSetup {};

TEST_F(SinkServiceTest, SimpleSink) {
  connectToServer(
      [](TestSinkServiceAsyncClient& client) -> folly::coro::Task<void> {
        auto sink = co_await client.co_range(0, 100);
        bool finalResponse =
            co_await sink.sink([]() -> folly::coro::AsyncGenerator<int&&> {
              for (int i = 0; i <= 100; i++) {
                co_yield std::move(i);
              }
            }());
        EXPECT_TRUE(finalResponse);
      });
}

TEST_F(SinkServiceTest, SinkThrow) {
  connectToServer(
      [](TestSinkServiceAsyncClient& client) -> folly::coro::Task<void> {
        auto sink = co_await client.co_rangeThrow(0, 100);
        bool throwed = false;
        try {
          co_await sink.sink([]() -> folly::coro::AsyncGenerator<int&&> {
            co_yield 0;
            co_yield 1;
            throw std::runtime_error("test");
          }());
        } catch (const std::exception& ex) {
          throwed = true;
        }
        EXPECT_TRUE(throwed);
      });
}

TEST_F(SinkServiceTest, SinkFinalThrow) {
  connectToServer(
      [](TestSinkServiceAsyncClient& client) -> folly::coro::Task<void> {
        auto sink = co_await client.co_rangeFinalResponseThrow(0, 100);
        bool throwed = false;
        try {
          co_await sink.sink([]() -> folly::coro::AsyncGenerator<int&&> {
            for (int i = 0; i <= 100; i++) {
              co_yield std::move(i);
            }
          }());
        } catch (const std::exception& ex) {
          throwed = true;
          EXPECT_EQ("std::runtime_error: test", std::string(ex.what()));
        }
        EXPECT_TRUE(throwed);
      });
}

TEST_F(SinkServiceTest, SinkEarlyFinalResponse) {
  connectToServer(
      [](TestSinkServiceAsyncClient& client) -> folly::coro::Task<void> {
        auto sink = co_await client.co_rangeEarlyResponse(0, 100, 20);

        int finalResponse =
            co_await sink.sink([]() -> folly::coro::AsyncGenerator<int&&> {
              for (int i = 0; i <= 100; i++) {
                co_yield std::move(i);
              }
            }());
        EXPECT_EQ(20, finalResponse);
      });
}

TEST_F(SinkServiceTest, SinkUnimplemented) {
  connectToServer(
      [](TestSinkServiceAsyncClient& client) -> folly::coro::Task<void> {
        bool throwed = true;
        try {
          auto sink = co_await client.co_unimplemented();
        } catch (const std::exception&) {
          throwed = true;
        }
        EXPECT_TRUE(throwed);
      });
}

TEST_F(SinkServiceTest, SinkNotCalled) {
  connectToServer(
      [](TestSinkServiceAsyncClient& client) -> folly::coro::Task<void> {
        // even though don't really call sink.sink(...),
        // after sink get out of scope, the sink should be cancelled properly
        co_await client.co_unSubscribedSink();
        bool unsubscribed = co_await client.co_isSinkUnSubscribed();
        EXPECT_TRUE(unsubscribed);
      });
}

TEST_F(SinkServiceTest, SinkInitialThrows) {
  connectToServer(
      [](TestSinkServiceAsyncClient& client) -> folly::coro::Task<void> {
        try {
          co_await client.co_initialThrow();
        } catch (const MyException& ex) {
          EXPECT_EQ("reason", ex.reason);
        }
      });
}

} // namespace thrift
} // namespace apache
