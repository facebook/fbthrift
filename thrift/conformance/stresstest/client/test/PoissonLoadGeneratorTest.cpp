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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <folly/coro/BlockingWait.h>
#include <folly/executors/GlobalExecutor.h>
#include <thrift/conformance/stresstest/client/PoissonLoadGenerator.h>

using namespace apache::thrift;
using namespace apache::thrift::stress;
using ::testing::Gt;
using ::testing::IsTrue;

using Signals = folly::coro::AsyncGenerator<PoissonLoadGenerator::Count>;

TEST(PoissonLoadGeneratorTest, Basic) {
  PoissonLoadGenerator generator(1000, std::chrono::milliseconds(5));
  generator.start();

  Signals signals = generator.getRequestCount();
  unsigned count = 0;
  while (count < 5) {
    signals.next().viaIfAsync(folly::getGlobalCPUExecutor()).await_ready();
    count++;
  }

  EXPECT_EQ(count, 5);
}

// A client thread gets its share of the target QPS, so the per-generator rate
// is low even when the aggregate rate is high. 125 qps over a 5 ms bucket
// averages 0.625 requests per bucket, and the generator has to keep offering
// load at that rate rather than fall silent.
TEST(PoissonLoadGeneratorTest, KeepsGeneratingBelowOneRequestPerBucket) {
  PoissonLoadGenerator generator(125, std::chrono::milliseconds(5));
  generator.start();

  Signals signals = generator.getRequestCount();
  int64_t total = 0;
  for (int i = 0; i < 100; ++i) {
    Signals::NextResult count = folly::coro::blockingWait(signals.next());
    ASSERT_THAT(count.has_value(), IsTrue());
    total += *count;
  }

  EXPECT_THAT(total, Gt(0));
}
