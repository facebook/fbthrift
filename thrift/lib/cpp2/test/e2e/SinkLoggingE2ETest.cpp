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

#include <folly/coro/GtestHelpers.h>
#include <folly/coro/Sleep.h>
#include <thrift/lib/cpp2/logging/IThriftRequestLogging.h>
#include <thrift/lib/cpp2/logging/IThriftServerCounters.h>
#include <thrift/lib/cpp2/server/ThriftServerInternals.h>
#include <thrift/lib/cpp2/test/e2e/E2ETestFixture.h>
#include <thrift/lib/cpp2/test/e2e/gen-cpp2/TestSinkE2EService.h>

using namespace ::testing;

namespace apache::thrift {

namespace {

class RecordingSinkCounters : public IThriftServerCounters {
 public:
  std::atomic<int> subscribes{0};
  std::atomic<int> nexts{0};
  std::atomic<int> consumeds{0};
  std::atomic<int> completes{0};
  std::atomic<int> sinkCredits{0};
  std::atomic<detail::SinkEndReason> lastEndReason{};

  void onSinkSubscribe(std::string_view /*methodName*/) override {
    ++subscribes;
  }
  void onSinkNext(std::string_view /*methodName*/) override { ++nexts; }
  void onSinkConsumed(std::string_view /*methodName*/) override { ++consumeds; }
  void onSinkCredit(
      std::string_view /*methodName*/, uint32_t /*credits*/) override {
    ++sinkCredits;
  }
  void onSinkComplete(
      std::string_view /*methodName*/, detail::SinkEndReason reason) override {
    ++completes;
    lastEndReason.store(reason);
  }
  void onSinkReceiveInterval(
      std::string_view /*methodName*/,
      std::chrono::milliseconds /*interval*/) override {}
  void onSinkConsumeLatency(
      std::string_view /*methodName*/,
      std::chrono::milliseconds /*latency*/) override {}
  void onSinkApproxPausedDuration(
      std::string_view /*methodName*/,
      std::chrono::milliseconds /*duration*/) override {}
};

class RecordingSinkLogging : public IThriftRequestLogging {
 public:
  std::atomic<int> sinkCompletes{0};
  std::mutex mutex;
  SinkSummary lastSummary{};

  void onSinkComplete(const SinkSummary& summary) override {
    ++sinkCompletes;
    std::lock_guard lock(mutex);
    lastSummary = summary;
  }
};

class SinkLoggingE2ETest : public test::E2ETestFixture {
 protected:
  std::shared_ptr<RecordingSinkCounters> counters_ =
      std::make_shared<RecordingSinkCounters>();
  std::shared_ptr<RecordingSinkLogging> logging_ =
      std::make_shared<RecordingSinkLogging>();

  void setupWithHandler(std::shared_ptr<AsyncProcessorFactory> handler) {
    testConfig(
        {.handler = std::move(handler),
         .serverConfigCb = [counters = counters_,
                            logging = logging_](ThriftServer& server) {
           detail::ThriftServerInternals internals(server);
           internals.setThriftServerCounters(counters);
           internals.setThriftRequestLogging(logging);
         }});
  }
};

} // namespace

CO_TEST_F(SinkLoggingE2ETest, BasicSinkLogsSubscribeNextAndComplete) {
  struct Handler : public ServiceHandler<detail::test::TestSinkE2EService> {
    SinkConsumer<int32_t, std::string> echo(int32_t count) override {
      return SinkConsumer<int32_t, std::string>{
          [count](folly::coro::AsyncGenerator<int32_t&&> gen)
              -> folly::coro::Task<std::string> {
            int32_t received = 0;
            while (auto item = co_await gen.next()) {
              EXPECT_EQ(*item, received);
              ++received;
            }
            EXPECT_EQ(received, count);
            co_return fmt::format("received {}", received);
          },
          10};
    }
  };

  setupWithHandler(std::make_shared<Handler>());
  auto client = makeClient<detail::test::TestSinkE2EService>();
  auto sink = co_await client->co_echo(5);
  auto finalResponse =
      co_await sink.sink([]() -> folly::coro::AsyncGenerator<int32_t&&> {
        for (int32_t i = 0; i < 5; ++i) {
          co_yield int32_t(i);
        }
      }());
  EXPECT_EQ(finalResponse, "received 5");

  /* sleep override */ co_await folly::coro::sleep(
      std::chrono::milliseconds(100));

  EXPECT_EQ(counters_->subscribes.load(), 1);
  EXPECT_GE(counters_->nexts.load(), 5);
  EXPECT_GE(counters_->consumeds.load(), 5);
  EXPECT_EQ(counters_->completes.load(), 1);
  EXPECT_EQ(counters_->lastEndReason.load(), detail::SinkEndReason::COMPLETE);
  EXPECT_EQ(logging_->sinkCompletes.load(), 1);

  {
    std::lock_guard lock(logging_->mutex);
    EXPECT_GE(logging_->lastSummary.chunksReceived, 5);
    EXPECT_GE(logging_->lastSummary.chunksConsumed, 5);
    EXPECT_EQ(logging_->lastSummary.endReason, detail::SinkEndReason::COMPLETE);
  }
}

CO_TEST_F(SinkLoggingE2ETest, EmptySinkLogsSubscribeAndComplete) {
  struct Handler : public ServiceHandler<detail::test::TestSinkE2EService> {
    SinkConsumer<int32_t, std::string> echo(int32_t /*count*/) override {
      return SinkConsumer<int32_t, std::string>{
          [](folly::coro::AsyncGenerator<int32_t&&> gen)
              -> folly::coro::Task<std::string> {
            while (auto item = co_await gen.next()) {
            }
            co_return "empty";
          },
          10};
    }
  };

  setupWithHandler(std::make_shared<Handler>());
  auto client = makeClient<detail::test::TestSinkE2EService>();
  auto sink = co_await client->co_echo(0);
  auto finalResponse = co_await sink.sink(
      []() -> folly::coro::AsyncGenerator<int32_t&&> { co_return; }());
  EXPECT_EQ(finalResponse, "empty");

  /* sleep override */ co_await folly::coro::sleep(
      std::chrono::milliseconds(100));

  EXPECT_EQ(counters_->subscribes.load(), 1);
  EXPECT_EQ(counters_->completes.load(), 1);
  EXPECT_EQ(counters_->lastEndReason.load(), detail::SinkEndReason::COMPLETE);
  EXPECT_EQ(logging_->sinkCompletes.load(), 1);

  {
    std::lock_guard lock(logging_->mutex);
    EXPECT_EQ(logging_->lastSummary.chunksReceived, 0);
    EXPECT_EQ(logging_->lastSummary.chunksConsumed, 0);
  }
}

CO_TEST_F(SinkLoggingE2ETest, SinkErrorLogsErrorReason) {
  struct Handler : public ServiceHandler<detail::test::TestSinkE2EService> {
    SinkConsumer<int32_t, std::string> canThrow() override {
      return SinkConsumer<int32_t, std::string>{
          [](folly::coro::AsyncGenerator<int32_t&&> gen)
              -> folly::coro::Task<std::string> {
            while (auto item = co_await gen.next()) {
            }
            throw detail::test::SinkFinalException{"final error"};
          },
          10};
    }
  };

  setupWithHandler(std::make_shared<Handler>());
  auto client = makeClient<detail::test::TestSinkE2EService>();
  auto sink = co_await client->co_canThrow();
  EXPECT_THROW(
      co_await sink.sink([]() -> folly::coro::AsyncGenerator<int32_t&&> {
        co_yield int32_t(1);
      }()),
      detail::test::SinkFinalException);

  /* sleep override */ co_await folly::coro::sleep(
      std::chrono::milliseconds(100));

  EXPECT_EQ(counters_->subscribes.load(), 1);
  EXPECT_EQ(counters_->completes.load(), 1);
  EXPECT_EQ(
      counters_->lastEndReason.load(),
      detail::SinkEndReason::COMPLETE_WITH_ERROR);
}

CO_TEST_F(SinkLoggingE2ETest, SinkClientErrorLogsErrorReason) {
  struct Handler : public ServiceHandler<detail::test::TestSinkE2EService> {
    SinkConsumer<int32_t, std::string> canThrow() override {
      return SinkConsumer<int32_t, std::string>{
          [](folly::coro::AsyncGenerator<int32_t&&> gen)
              -> folly::coro::Task<std::string> {
            try {
              while (auto item = co_await gen.next()) {
              }
            } catch (const detail::test::SinkItemException&) {
              co_return "caught";
            }
            co_return "no exception";
          },
          10};
    }
  };

  setupWithHandler(std::make_shared<Handler>());
  auto client = makeClient<detail::test::TestSinkE2EService>();
  auto sink = co_await client->co_canThrow();
  EXPECT_THROW(
      co_await sink.sink([]() -> folly::coro::AsyncGenerator<int32_t&&> {
        co_yield int32_t(1);
        throw detail::test::SinkItemException{"item error"};
      }()),
      apache::thrift::SinkThrew);

  /* sleep override */ co_await folly::coro::sleep(
      std::chrono::milliseconds(100));

  EXPECT_EQ(counters_->subscribes.load(), 1);
  EXPECT_EQ(counters_->completes.load(), 1);
  EXPECT_EQ(counters_->lastEndReason.load(), detail::SinkEndReason::ERROR);
}

CO_TEST_F(SinkLoggingE2ETest, MultipleSinksLogIndependently) {
  struct Handler : public ServiceHandler<detail::test::TestSinkE2EService> {
    SinkConsumer<int32_t, std::string> echo(int32_t /*count*/) override {
      return SinkConsumer<int32_t, std::string>{
          [](folly::coro::AsyncGenerator<int32_t&&> gen)
              -> folly::coro::Task<std::string> {
            int32_t received = 0;
            while (auto item = co_await gen.next()) {
              ++received;
            }
            co_return fmt::format("received {}", received);
          },
          10};
    }
  };

  setupWithHandler(std::make_shared<Handler>());
  auto client = makeClient<detail::test::TestSinkE2EService>();

  // First sink
  auto sink1 = co_await client->co_echo(3);
  co_await sink1.sink([]() -> folly::coro::AsyncGenerator<int32_t&&> {
    for (int32_t i = 0; i < 3; ++i) {
      co_yield int32_t(i);
    }
  }());

  // Second sink
  auto sink2 = co_await client->co_echo(2);
  co_await sink2.sink([]() -> folly::coro::AsyncGenerator<int32_t&&> {
    for (int32_t i = 0; i < 2; ++i) {
      co_yield int32_t(i);
    }
  }());

  /* sleep override */ co_await folly::coro::sleep(
      std::chrono::milliseconds(100));

  EXPECT_EQ(counters_->subscribes.load(), 2);
  EXPECT_EQ(counters_->completes.load(), 2);
  EXPECT_EQ(logging_->sinkCompletes.load(), 2);
}

CO_TEST_F(SinkLoggingE2ETest, SinkWithInitialResponseLogsCorrectly) {
  struct Handler : public ServiceHandler<detail::test::TestSinkE2EService> {
    ResponseAndSinkConsumer<std::string, int32_t, std::string> echoWithResponse(
        int32_t count) override {
      return {
          fmt::format("expecting {}", count),
          SinkConsumer<int32_t, std::string>{
              [count](folly::coro::AsyncGenerator<int32_t&&> gen)
                  -> folly::coro::Task<std::string> {
                int32_t received = 0;
                while (auto item = co_await gen.next()) {
                  ++received;
                }
                EXPECT_EQ(received, count);
                co_return fmt::format("received {}", received);
              },
              10}};
    }
  };

  setupWithHandler(std::make_shared<Handler>());
  auto client = makeClient<detail::test::TestSinkE2EService>();
  auto result = co_await client->co_echoWithResponse(3);
  EXPECT_EQ(result.response, "expecting 3");

  auto finalResponse =
      co_await result.sink.sink([]() -> folly::coro::AsyncGenerator<int32_t&&> {
        for (int32_t i = 0; i < 3; ++i) {
          co_yield int32_t(i);
        }
      }());
  EXPECT_EQ(finalResponse, "received 3");

  /* sleep override */ co_await folly::coro::sleep(
      std::chrono::milliseconds(100));

  EXPECT_EQ(counters_->subscribes.load(), 1);
  EXPECT_EQ(counters_->completes.load(), 1);
  EXPECT_EQ(counters_->lastEndReason.load(), detail::SinkEndReason::COMPLETE);
  EXPECT_EQ(logging_->sinkCompletes.load(), 1);
}

} // namespace apache::thrift
