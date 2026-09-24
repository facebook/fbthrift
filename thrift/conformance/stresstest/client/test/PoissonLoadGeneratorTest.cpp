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

#include <chrono>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <thrift/conformance/stresstest/client/PoissonLoadGenerator.h>

using namespace apache::thrift;
using namespace apache::thrift::stress;
using ::testing::Gt;
using ::testing::IsTrue;
using ::testing::Lt;

using Signals = folly::coro::AsyncGenerator<PoissonLoadGenerator::Count>;

// A client thread only gets its share of the target QPS, so a single generator
// runs at a low rate even when the aggregate rate is high.
TEST(PoissonLoadGeneratorTest, KeepsGeneratingAtLowRate) {
  PoissonLoadGenerator generator(125);
  generator.start();

  Signals signals = generator.getRequestCount();
  int64_t total = 0;
  for (int i = 0; i < 20; ++i) {
    Signals::NextResult count = folly::coro::blockingWait(signals.next());
    ASSERT_THAT(count.has_value(), IsTrue());
    total += *count;
  }

  EXPECT_THAT(total, Gt(0));
}

// 2000 qps means a 500 us mean gap, so under an exponential distribution only
// about 2% of gaps land under 10 us. A generator that releases a whole bucket
// at once puts nearly all of them there.
//
// The whole loop runs inside one blockingWait. Waiting per arrival instead
// costs more than 10 us on its own and hides the gaps being measured.
TEST(PoissonLoadGeneratorTest, SpreadsArrivalsOverTime) {
  PoissonLoadGenerator generator(2000);
  generator.start();

  const int backToBack =
      folly::coro::blockingWait([&]() -> folly::coro::Task<int> {
        Signals signals = generator.getRequestCount();
        std::chrono::steady_clock::time_point previous =
            std::chrono::steady_clock::now();
        int count = 0;
        for (int i = 0; i < 200; ++i) {
          Signals::NextResult arrival = co_await signals.next();
          EXPECT_THAT(arrival.has_value(), IsTrue());
          const std::chrono::steady_clock::time_point now =
              std::chrono::steady_clock::now();
          if (now - previous < std::chrono::microseconds(10)) {
            ++count;
          }
          previous = now;
        }
        co_return count;
      }());

  // Measured: 7 to 23 back-to-back arrivals with gaps drawn per request, 149
  // when a bucket is released at once.
  EXPECT_THAT(backToBack, Lt(75));
}
