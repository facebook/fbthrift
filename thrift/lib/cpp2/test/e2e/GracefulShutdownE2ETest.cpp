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

#include <atomic>

#include <fmt/core.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/coro/Sleep.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/test/e2e/gen-cpp2/TestGracefulShutdownService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace ::testing;

namespace apache::thrift {

namespace {

/**
 * Test fixture that exposes the underlying server for shutdown control.
 *
 * Unlike E2ETestFixture, this gives tests direct access to the
 * ScopedServerInterfaceThread so they can trigger graceful shutdown
 * while RPCs are in flight.
 */
class GracefulShutdownE2ETest : public ::testing::Test {
 protected:
  void startServer(std::shared_ptr<AsyncProcessorFactory> handler) {
    server_ = std::make_unique<ScopedServerInterfaceThread>(std::move(handler));
  }

  template <typename ServiceTag>
  std::unique_ptr<Client<ServiceTag>> makeClient() {
    return server_->newClient<Client<ServiceTag>>(
        nullptr,
        [](folly::AsyncSocket::UniquePtr socket) -> RequestChannel::Ptr {
          return RocketClientChannel::newChannel(std::move(socket));
        });
  }

  void stopServer() { server_.reset(); }

  std::unique_ptr<ScopedServerInterfaceThread> server_;
};

using ServiceHandler_ =
    ServiceHandler<detail::test::TestGracefulShutdownService>;

} // namespace

// ==================== Stream + Shutdown ====================

CO_TEST_F(GracefulShutdownE2ETest, ActiveStreamTerminatesOnServerShutdown) {
  // Server streams items slowly. We trigger shutdown mid-stream and verify
  // the client sees the stream end (via error or clean completion).
  folly::Baton<> handlerStarted;

  struct Handler : public ServiceHandler_ {
    explicit Handler(folly::Baton<>& started) : started_(started) {}

    ServerStream<int32_t> range(int32_t from, int32_t to) override {
      started_.post();
      for (int32_t i = from; i <= to; ++i) {
        co_yield int32_t(i);
        co_await folly::coro::sleep(std::chrono::milliseconds(50));
        co_await folly::coro::co_safe_point;
      }
    }

    folly::Baton<>& started_;
  };

  startServer(std::make_shared<Handler>(handlerStarted));
  auto client = makeClient<detail::test::TestGracefulShutdownService>();
  auto gen = (co_await client->co_range(0, 100)).toAsyncGenerator();

  auto first = co_await gen.next();
  EXPECT_TRUE(first.has_value());
  EXPECT_EQ(*first, 0);
  EXPECT_TRUE(handlerStarted.try_wait_for(std::chrono::seconds(5)));

  stopServer();

  // Client should see the stream end -- not hang forever.
  bool streamEnded = false;
  try {
    while (co_await gen.next()) {
    }
    streamEnded = true;
  } catch (const transport::TTransportException&) {
    streamEnded = true;
  } catch (const TApplicationException&) {
    streamEnded = true;
  }
  EXPECT_TRUE(streamEnded);
}

CO_TEST_F(GracefulShutdownE2ETest, NewStreamRejectedAfterShutdown) {
  struct Handler : public ServiceHandler_ {
    ServerStream<int32_t> range(int32_t from, int32_t to) override {
      for (int32_t i = from; i <= to; ++i) {
        co_yield int32_t(i);
      }
    }
  };

  startServer(std::make_shared<Handler>());
  auto client = makeClient<detail::test::TestGracefulShutdownService>();

  // Verify server works before shutdown
  auto gen = (co_await client->co_range(0, 2)).toAsyncGenerator();
  while (co_await gen.next()) {
  }

  stopServer();

  EXPECT_THROW(co_await client->co_range(0, 5), transport::TTransportException);
}

// ==================== Sink + Shutdown ====================

CO_TEST_F(GracefulShutdownE2ETest, ActiveSinkTerminatesOnServerShutdown) {
  // Client sends sink items slowly. Server shuts down mid-sink.
  // The sink() call should terminate, not hang.
  folly::Baton<> consumerStarted;

  struct Handler : public ServiceHandler_ {
    explicit Handler(folly::Baton<>& started) : started_(started) {}

    SinkConsumer<int32_t, std::string> consume(int32_t /*count*/) override {
      return SinkConsumer<int32_t, std::string>{
          [this](folly::coro::AsyncGenerator<int32_t&&> gen)
              -> folly::coro::Task<std::string> {
            started_.post();
            int32_t received = 0;
            while (auto item = co_await gen.next()) {
              ++received;
            }
            co_return fmt::format("received {}", received);
          },
          10};
    }

    folly::Baton<>& started_;
  };

  startServer(std::make_shared<Handler>(consumerStarted));
  auto client = makeClient<detail::test::TestGracefulShutdownService>();
  auto sink = co_await client->co_consume(1000);

  // Run sink sending and shutdown concurrently via collectAll.
  // The sink task must be running *before* stopServer() to test
  // mid-operation shutdown (not post-shutdown failure).
  auto sinkTask = folly::coro::co_invoke(
      [sink = std::move(sink)]() mutable -> folly::coro::Task<bool> {
        try {
          co_await sink.sink([]() -> folly::coro::AsyncGenerator<int32_t&&> {
            for (int32_t i = 0; i < 1000; ++i) {
              co_yield int32_t(i);
              co_await folly::coro::sleep(std::chrono::milliseconds(10));
            }
          }());
          co_return true; // Completed with final response
        } catch (const transport::TTransportException&) {
          co_return true; // Terminated by shutdown
        } catch (const TApplicationException&) {
          co_return true; // Terminated by shutdown
        }
      });

  auto shutdownTask = folly::coro::co_invoke(
      [this, &consumerStarted]() -> folly::coro::Task<void> {
        EXPECT_TRUE(consumerStarted.try_wait_for(std::chrono::seconds(5)));
        co_await folly::coro::sleep(std::chrono::milliseconds(100));
        stopServer();
      });

  auto [sinkResult, shutdownResult] = co_await folly::coro::collectAll(
      std::move(sinkTask), std::move(shutdownTask));
  EXPECT_TRUE(sinkResult) << "Sink should terminate (not hang) on shutdown";
}

// ==================== BiDi + Shutdown ====================

CO_TEST_F(GracefulShutdownE2ETest, ActiveBiDiTerminatesOnServerShutdown) {
  // BiDi echo is active. Server shuts down. Both sides should terminate.
  folly::Baton<> handlerStarted;

  struct Handler : public ServiceHandler_ {
    explicit Handler(folly::Baton<>& started) : started_(started) {}

    folly::coro::Task<StreamTransformation<std::string, std::string>>
    co_bidiEcho() override {
      started_.post();
      co_return StreamTransformation<std::string, std::string>{
          [](folly::coro::AsyncGenerator<std::string&&> input)
              -> folly::coro::AsyncGenerator<std::string&&> {
            while (auto item = co_await input.next()) {
              co_yield std::move(*item);
            }
          }};
    }

    folly::Baton<>& started_;
  };

  startServer(std::make_shared<Handler>(handlerStarted));
  auto client = makeClient<detail::test::TestGracefulShutdownService>();
  auto bidi = co_await client->co_bidiEcho();
  EXPECT_TRUE(handlerStarted.try_wait_for(std::chrono::seconds(5)));

  // Run sink, stream, and shutdown concurrently via collectAll.
  // All three tasks start at the same time so sink/stream are active
  // before shutdown triggers.
  auto sinkTask = folly::coro::co_invoke(
      [clientSink = std::move(bidi.sink)]() mutable -> folly::coro::Task<void> {
        try {
          co_await std::move(clientSink)
              .sink([]() -> folly::coro::AsyncGenerator<std::string&&> {
                for (int i = 0; i < 1000; ++i) {
                  co_yield std::to_string(i);
                  co_await folly::coro::sleep(std::chrono::milliseconds(20));
                }
              }());
        } catch (...) {
          // Expected: server shutdown terminates the sink
        }
      });

  auto streamTask = folly::coro::co_invoke(
      [streamGen = std::move(bidi.stream).toAsyncGenerator()]() mutable
          -> folly::coro::Task<bool> {
        try {
          while (co_await streamGen.next()) {
          }
          co_return true;
        } catch (const transport::TTransportException&) {
          co_return true;
        } catch (const TApplicationException&) {
          co_return true;
        }
      });

  auto shutdownTask =
      folly::coro::co_invoke([this]() -> folly::coro::Task<void> {
        // Let some items flow before shutdown
        co_await folly::coro::sleep(std::chrono::milliseconds(200));
        stopServer();
      });

  auto [sinkResult, streamResult, shutdownResult] =
      co_await folly::coro::collectAll(
          std::move(sinkTask), std::move(streamTask), std::move(shutdownTask));
  EXPECT_TRUE(streamResult) << "Stream should terminate on shutdown";
}

// ==================== Request-Response + Shutdown ====================

CO_TEST_F(GracefulShutdownE2ETest, InFlightRequestCompletesOrFailsOnShutdown) {
  // Handler takes a while to complete. We trigger shutdown while the
  // request is in-flight. The request should complete (handler finishes
  // within the drain deadline) or fail, not hang.
  folly::Baton<> handlerBlocked;
  folly::Baton<> proceed;

  struct Handler : public ServiceHandler_ {
    Handler(folly::Baton<>& blocked, folly::Baton<>& p)
        : blocked_(blocked), proceed_(p) {}

    folly::coro::Task<std::unique_ptr<std::string>> co_echo(
        std::unique_ptr<std::string> message) override {
      blocked_.post();
      // Cooperatively wait until signaled; suspends without blocking
      // the executor thread.
      while (!proceed_.try_wait_for(std::chrono::milliseconds(10))) {
        co_await folly::coro::co_reschedule_on_current_executor;
      }
      co_return std::move(message);
    }

    folly::Baton<>& blocked_;
    folly::Baton<>& proceed_;
  };

  startServer(std::make_shared<Handler>(handlerBlocked, proceed));
  auto client = makeClient<detail::test::TestGracefulShutdownService>();

  // Use semifuture to send the request without blocking the current thread
  auto requestFuture = client->semifuture_echo("hello");

  EXPECT_TRUE(handlerBlocked.try_wait_for(std::chrono::seconds(5)));

  // Unblock the handler, then shut down.
  // The handler should complete within the drain deadline.
  proceed.post();
  stopServer();

  bool completed = false;
  try {
    co_await std::move(requestFuture);
    completed = true;
  } catch (const transport::TTransportException&) {
    completed = true;
  } catch (const TApplicationException&) {
    completed = true;
  }
  EXPECT_TRUE(completed)
      << "In-flight request should complete or fail, not hang";
}

// ==================== Multiple clients + Shutdown ====================

CO_TEST_F(
    GracefulShutdownE2ETest, MultipleConcurrentStreamsTerminateOnShutdown) {
  // Multiple clients each have an active infinite stream.
  // Server shuts down. All streams should terminate.
  std::atomic<int> handlersStarted{0};

  struct Handler : public ServiceHandler_ {
    explicit Handler(std::atomic<int>& started) : started_(started) {}

    // Intentionally ignores 'to' -- runs forever until cancelled.
    ServerStream<int32_t> range(int32_t from, int32_t /*to*/) override {
      started_.fetch_add(1);
      int32_t i = from;
      while (true) {
        co_yield int32_t(i++);
        co_await folly::coro::sleep(std::chrono::milliseconds(30));
        co_await folly::coro::co_safe_point;
      }
    }

    std::atomic<int>& started_;
  };

  startServer(std::make_shared<Handler>(handlersStarted));

  constexpr int kNumClients = 3;
  std::vector<
      std::unique_ptr<Client<detail::test::TestGracefulShutdownService>>>
      clients;
  std::vector<folly::coro::AsyncGenerator<int32_t&&>> generators;

  for (int i = 0; i < kNumClients; ++i) {
    auto client = makeClient<detail::test::TestGracefulShutdownService>();
    auto gen = (co_await client->co_range(i * 100, 0)).toAsyncGenerator();
    auto first = co_await gen.next();
    EXPECT_TRUE(first.has_value());
    clients.push_back(std::move(client));
    generators.push_back(std::move(gen));
  }

  EXPECT_EQ(handlersStarted.load(), kNumClients);
  stopServer();

  for (int i = 0; i < kNumClients; ++i) {
    bool terminated = false;
    try {
      while (co_await generators[i].next()) {
      }
      terminated = true;
    } catch (const transport::TTransportException&) {
      terminated = true;
    } catch (const TApplicationException&) {
      terminated = true;
    }
    EXPECT_TRUE(terminated)
        << "Stream " << i << " should terminate on shutdown";
  }
}

// ==================== onStopRequested + active streams ====================

CO_TEST_F(GracefulShutdownE2ETest, OnStopRequestedCalledWithActiveStreams) {
  // Verify co_onStopRequested fires even when streams are active.
  folly::Baton<> onStopCalled;
  folly::Baton<> streamStarted;

  struct Handler : public ServiceHandler_ {
    Handler(folly::Baton<>& stopCalled, folly::Baton<>& started)
        : stopCalled_(stopCalled), started_(started) {}

    folly::coro::Task<void> co_onStopRequested() override {
      stopCalled_.post();
      co_return;
    }

    // Intentionally ignores 'to' -- runs forever until cancelled.
    ServerStream<int32_t> range(int32_t from, int32_t /*to*/) override {
      started_.post();
      int32_t i = from;
      while (true) {
        co_yield int32_t(i++);
        co_await folly::coro::sleep(std::chrono::milliseconds(30));
        co_await folly::coro::co_safe_point;
      }
    }

    folly::Baton<>& stopCalled_;
    folly::Baton<>& started_;
  };

  startServer(std::make_shared<Handler>(onStopCalled, streamStarted));
  auto client = makeClient<detail::test::TestGracefulShutdownService>();

  auto gen = (co_await client->co_range(0, 0)).toAsyncGenerator();
  auto first = co_await gen.next();
  EXPECT_TRUE(first.has_value());
  EXPECT_TRUE(streamStarted.try_wait_for(std::chrono::seconds(5)));

  stopServer();

  EXPECT_TRUE(onStopCalled.try_wait_for(std::chrono::seconds(5)));
}

// ==================== Mixed RPC patterns + Shutdown ====================

CO_TEST_F(GracefulShutdownE2ETest, MixedStreamAndSinkTerminateOnShutdown) {
  // One client has an active stream, another has an active sink.
  // Server shuts down. Both should terminate.
  folly::Baton<> streamStarted;
  folly::Baton<> sinkStarted;

  struct Handler : public ServiceHandler_ {
    Handler(folly::Baton<>& sBaton, folly::Baton<>& kBaton)
        : streamStarted_(sBaton), sinkStarted_(kBaton) {}

    // Intentionally ignores 'to' -- runs forever until cancelled.
    ServerStream<int32_t> range(int32_t from, int32_t /*to*/) override {
      streamStarted_.post();
      int32_t i = from;
      while (true) {
        co_yield int32_t(i++);
        co_await folly::coro::sleep(std::chrono::milliseconds(30));
        co_await folly::coro::co_safe_point;
      }
    }

    SinkConsumer<int32_t, std::string> consume(int32_t /*count*/) override {
      return SinkConsumer<int32_t, std::string>{
          [this](folly::coro::AsyncGenerator<int32_t&&> gen)
              -> folly::coro::Task<std::string> {
            sinkStarted_.post();
            int32_t received = 0;
            while (auto item = co_await gen.next()) {
              ++received;
            }
            co_return fmt::format("received {}", received);
          },
          10};
    }

    folly::Baton<>& streamStarted_;
    folly::Baton<>& sinkStarted_;
  };

  startServer(std::make_shared<Handler>(streamStarted, sinkStarted));

  // Start a stream on one client
  auto streamClient = makeClient<detail::test::TestGracefulShutdownService>();
  auto gen = (co_await streamClient->co_range(0, 0)).toAsyncGenerator();
  auto first = co_await gen.next();
  EXPECT_TRUE(first.has_value());
  EXPECT_TRUE(streamStarted.try_wait_for(std::chrono::seconds(5)));

  // Start a sink on another client
  auto sinkClient = makeClient<detail::test::TestGracefulShutdownService>();
  auto sink = co_await sinkClient->co_consume(1000);

  // Run sink and shutdown concurrently via collectAll so the sink
  // is actively sending before shutdown triggers.
  auto sinkTask = folly::coro::co_invoke(
      [sink = std::move(sink)]() mutable -> folly::coro::Task<bool> {
        try {
          co_await sink.sink([]() -> folly::coro::AsyncGenerator<int32_t&&> {
            for (int32_t i = 0; i < 1000; ++i) {
              co_yield int32_t(i);
              co_await folly::coro::sleep(std::chrono::milliseconds(10));
            }
          }());
          co_return true;
        } catch (...) {
          co_return true;
        }
      });

  auto streamDrainTask =
      folly::coro::co_invoke([&gen]() -> folly::coro::Task<bool> {
        try {
          while (co_await gen.next()) {
          }
          co_return true;
        } catch (...) {
          co_return true;
        }
      });

  auto shutdownTask =
      folly::coro::co_invoke([this, &sinkStarted]() -> folly::coro::Task<void> {
        EXPECT_TRUE(sinkStarted.try_wait_for(std::chrono::seconds(5)));
        co_await folly::coro::sleep(std::chrono::milliseconds(100));
        stopServer();
      });

  auto [sinkResult, streamResult, shutdownResult] =
      co_await folly::coro::collectAll(
          std::move(sinkTask),
          std::move(streamDrainTask),
          std::move(shutdownTask));
  EXPECT_TRUE(sinkResult) << "Sink should terminate on shutdown";
  EXPECT_TRUE(streamResult) << "Stream should terminate on shutdown";
}

} // namespace apache::thrift
