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

#include <thrift/lib/cpp2/logging/ThriftSinkLog.h>

#include <gtest/gtest.h>

namespace apache::thrift {

namespace {

class RecordingCounters : public IThriftServerCounters {
 public:
  int subscribes = 0;
  int nexts = 0;
  int consumeds = 0;
  int cancels = 0;
  int credits = 0;
  int completes = 0;
  int receiveIntervals = 0;
  int consumeLatencies = 0;
  int approxPauses = 0;
  uint32_t lastCredits = 0;
  detail::SinkEndReason lastEndReason{};

  void onSinkSubscribe(std::string_view /*methodName*/) override {
    ++subscribes;
  }
  void onSinkNext(std::string_view /*methodName*/) override { ++nexts; }
  void onSinkConsumed(std::string_view /*methodName*/) override { ++consumeds; }
  void onSinkCancel(std::string_view /*methodName*/) override { ++cancels; }
  void onSinkCredit(std::string_view /*methodName*/, uint32_t c) override {
    ++credits;
    lastCredits = c;
  }
  void onSinkComplete(
      std::string_view /*methodName*/, detail::SinkEndReason reason) override {
    ++completes;
    lastEndReason = reason;
  }
  void onSinkReceiveInterval(
      std::string_view /*methodName*/,
      std::chrono::milliseconds /*interval*/) override {
    ++receiveIntervals;
  }
  void onSinkConsumeLatency(
      std::string_view /*methodName*/,
      std::chrono::milliseconds /*latency*/) override {
    ++consumeLatencies;
  }
  void onSinkApproxPausedDuration(
      std::string_view /*methodName*/,
      std::chrono::milliseconds /*duration*/) override {
    ++approxPauses;
  }
};

class RecordingLogging : public IThriftRequestLogging {
 public:
  int sinkCompletes = 0;
  SinkSummary lastSummary{};

  void onSinkComplete(const SinkSummary& summary) override {
    ++sinkCompletes;
    lastSummary = summary;
  }
};

} // namespace

class ThriftSinkLogTest : public ::testing::Test {
 protected:
  RecordingCounters counters_;
  RecordingLogging logging_;
};

TEST_F(ThriftSinkLogTest, SubscribeDispatchesToCounters) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  EXPECT_EQ(counters_.subscribes, 1);
}

TEST_F(ThriftSinkLogTest, NextDispatchesToCounters) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkNextEvent{});
  EXPECT_EQ(counters_.nexts, 1);
}

TEST_F(ThriftSinkLogTest, ConsumedDispatchesToCounters) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkConsumedEvent{});
  EXPECT_EQ(counters_.consumeds, 1);
}

TEST_F(ThriftSinkLogTest, CreditDispatchesToCounters) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkCreditEvent{10});
  EXPECT_EQ(counters_.credits, 1);
  EXPECT_EQ(counters_.lastCredits, 10);
}

TEST_F(ThriftSinkLogTest, ReceiveIntervalSkippedForFirstChunk) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkNextEvent{});
  EXPECT_EQ(counters_.receiveIntervals, 0);

  log.log(detail::SinkNextEvent{});
  EXPECT_EQ(counters_.receiveIntervals, 1);
}

TEST_F(ThriftSinkLogTest, CompleteEvent) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkNextEvent{});
  log.log(detail::SinkConsumedEvent{});
  log.log(detail::SinkCompleteEvent{detail::SinkEndReason::COMPLETE});

  EXPECT_EQ(counters_.completes, 1);
  EXPECT_EQ(counters_.lastEndReason, detail::SinkEndReason::COMPLETE);
  EXPECT_EQ(logging_.sinkCompletes, 1);
  EXPECT_EQ(logging_.lastSummary.chunksReceived, 1);
  EXPECT_EQ(logging_.lastSummary.chunksConsumed, 1);
}

TEST_F(ThriftSinkLogTest, ErrorEvent) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkCompleteEvent{detail::SinkEndReason::ERROR});
  EXPECT_EQ(counters_.lastEndReason, detail::SinkEndReason::ERROR);
}

TEST_F(ThriftSinkLogTest, CompleteWithErrorEvent) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(
      detail::SinkCompleteEvent{detail::SinkEndReason::COMPLETE_WITH_ERROR});
  EXPECT_EQ(
      counters_.lastEndReason, detail::SinkEndReason::COMPLETE_WITH_ERROR);
}

TEST_F(ThriftSinkLogTest, CancelDispatchesToCounters) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkCancelEvent{});
  EXPECT_EQ(counters_.cancels, 1);
}

TEST_F(ThriftSinkLogTest, DestructorCleansUp) {
  {
    ThriftSinkLog log("myMethod", &counters_, &logging_);
    log.log(detail::SinkSubscribeEvent{});
  }
  EXPECT_EQ(counters_.completes, 1);
  EXPECT_EQ(counters_.lastEndReason, detail::SinkEndReason::ERROR);
  EXPECT_EQ(logging_.sinkCompletes, 1);
}

TEST_F(ThriftSinkLogTest, IdempotentFinish) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkCompleteEvent{detail::SinkEndReason::COMPLETE});
  log.log(detail::SinkCompleteEvent{detail::SinkEndReason::ERROR});
  EXPECT_EQ(counters_.completes, 1);
  EXPECT_EQ(logging_.sinkCompletes, 1);
}

TEST_F(ThriftSinkLogTest, NullBackends) {
  ThriftSinkLog log("myMethod", nullptr, nullptr);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkNextEvent{});
  log.log(detail::SinkConsumedEvent{});
  log.log(detail::SinkCancelEvent{});
  log.log(detail::SinkCreditEvent{5});
  log.log(detail::SinkCompleteEvent{detail::SinkEndReason::COMPLETE});
}

TEST_F(ThriftSinkLogTest, CreditPauseDetection) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});

  // Credits are 0 initially, so receiving a chunk marks as paused
  log.log(detail::SinkNextEvent{});

  // Sending credits should trigger approx pause duration
  log.log(detail::SinkCreditEvent{5});
  EXPECT_EQ(counters_.approxPauses, 1);
}

TEST_F(ThriftSinkLogTest, SummaryAccumulation) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkCreditEvent{10});

  for (int i = 0; i < 5; ++i) {
    log.log(detail::SinkNextEvent{});
    log.log(detail::SinkConsumedEvent{});
  }

  log.log(detail::SinkCompleteEvent{detail::SinkEndReason::COMPLETE});

  EXPECT_EQ(logging_.lastSummary.chunksReceived, 5);
  EXPECT_EQ(logging_.lastSummary.chunksConsumed, 5);
  EXPECT_EQ(logging_.lastSummary.totalCreditsSent, 10);
}

TEST_F(ThriftSinkLogTest, ConsumeLatencySampling) {
  ThriftSinkLog log("myMethod", &counters_, &logging_);
  log.log(detail::SinkSubscribeEvent{});
  log.log(detail::SinkCreditEvent{20});

  // Chunk 1 triggers sampling (1 % 10 == 1)
  log.log(detail::SinkNextEvent{});
  log.log(detail::SinkConsumedEvent{});
  EXPECT_EQ(counters_.consumeLatencies, 1);

  // Chunks 2-10 should not trigger sampling
  for (int i = 2; i <= 10; ++i) {
    log.log(detail::SinkNextEvent{});
    log.log(detail::SinkConsumedEvent{});
  }
  EXPECT_EQ(counters_.consumeLatencies, 1);

  // Chunk 11 triggers sampling again (11 % 10 == 1)
  log.log(detail::SinkNextEvent{});
  log.log(detail::SinkConsumedEvent{});
  EXPECT_EQ(counters_.consumeLatencies, 2);
}

} // namespace apache::thrift
