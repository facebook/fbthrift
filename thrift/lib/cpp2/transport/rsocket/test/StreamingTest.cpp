/*
 * Copyright 2017-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/transport/rsocket/test/util/TestServiceMock.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/TestUtil.h>

namespace apache {
namespace thrift {

namespace {
void waitNoLeak(StreamServiceAsyncClient* client) {
  auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds{100};
  do {
    std::this_thread::yield();
    if (client->sync_instanceCount() == 0) {
      // There is a race between decrementing the instance count vs
      // sending the error/complete message, so sleep a bit before returning
      /* sleep override */
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      return;
    }
  } while (std::chrono::steady_clock::now() < deadline);
  CHECK(false);
}
} // namespace

// Testing transport layers for their support to Streaming
class StreamingTest
    : public TestSetup,
      public testing::WithParamInterface<bool /* useRocketClient */> {
 protected:
  void SetUp() override {
    handler_ = std::make_shared<StrictMock<TestServiceMock>>();
    server_ = createServer(
        std::make_shared<ThriftServerAsyncProcessorFactory<TestServiceMock>>(
            handler_),
        port_);
  }

  void TearDown() override {
    if (server_) {
      server_->cleanUp();
      server_.reset();
      handler_.reset();
    }
  }

  void connectToServer(
      folly::Function<void(std::unique_ptr<StreamServiceAsyncClient>)> callMe,
      folly::Function<void()> onDetachable = nullptr) {
    auto channel = connectToServer(
        port_, std::move(onDetachable), GetParam() /* useRocketClient */);
    callMe(std::make_unique<StreamServiceAsyncClient>(std::move(channel)));
  }

  void callSleep(
      StreamServiceAsyncClient* client,
      int32_t timeoutMs,
      int32_t sleepMs,
      bool withResponse) {
    auto cb = std::make_unique<MockCallback>(false, timeoutMs < sleepMs);
    RpcOptions opts;
    opts.setTimeout(std::chrono::milliseconds(timeoutMs));
    opts.setQueueTimeout(std::chrono::milliseconds(5000));
    if (withResponse) {
      client->sleepWithResponse(opts, std::move(cb), sleepMs);
    } else {
      client->sleepWithoutResponse(opts, std::move(cb), sleepMs);
    }
  }

 private:
  using TestSetup::connectToServer;

 protected:
  std::unique_ptr<ThriftServer> server_;
  std::shared_ptr<testing::StrictMock<TestServiceMock>> handler_;

  uint16_t port_;
};

class BlockStreamingTest : public StreamingTest {
 protected:
  void SetUp() override {
    handler_ = std::make_shared<StrictMock<TestServiceMock>>();
    server_ = createServer(
        std::make_shared<ThriftServerAsyncProcessorFactory<TestServiceMock>>(
            handler_),
        port_,
        100);
  }
};

TEST_P(StreamingTest, SimpleStream) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    auto result = client->sync_range(0, 10).via(&executor_);
    int j = 0;
    auto subscription = std::move(result).subscribe(
        [&j](auto i) mutable { EXPECT_EQ(j++, i); },
        [](auto ex) { FAIL() << "Should not call onError: " << ex.what(); });
    std::move(subscription).join();
    EXPECT_EQ(10, j);
  });
}

TEST_P(StreamingTest, FutureSimpleStream) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    auto futureRange = client->future_range(0, 10);
    auto stream = std::move(futureRange).get();
    auto result = std::move(stream).via(&executor_);
    int j = 0;
    auto subscription = std::move(result).subscribe(
        [&j](auto i) mutable { EXPECT_EQ(j++, i); },
        [](auto ex) { FAIL() << "Should not call onError: " << ex.what(); });
    std::move(subscription).join();
    EXPECT_EQ(10, j);
  });
}

TEST_P(StreamingTest, CallbackSimpleStream) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    folly::Baton<> done;
    int j = 0;
    auto callback =
        [&done, &j, this](::apache::thrift::ClientReceiveState&& receiveState) {
          ASSERT_FALSE(receiveState.isException());
          auto stream = receiveState.extractStream();
          auto result = std::move(stream).via(&executor_);
          std::move(result)
              .subscribe(
                  [&j](const std::unique_ptr<folly::IOBuf>) mutable { ++j; },
                  [](auto ex) {
                    FAIL() << "Should not call onError: " << ex.what();
                  },
                  [&done]() { done.post(); })
              .detach();
        };

    client->range(std::move(callback), 0, 10);

    EXPECT_TRUE(done.try_wait_for(std::chrono::milliseconds(100)));
    EXPECT_EQ(10, j);
  });
}

TEST_P(StreamingTest, DefaultStreamImplementation) {
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    EXPECT_THROW(
        toFlowable(client->sync_nonImplementedStream("test").via(&executor_)),
        apache::thrift::TApplicationException);
  });
}

TEST_P(StreamingTest, ReturnsNullptr) {
  // User function should return a Stream, but it returns a nullptr.
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    bool success = false;
    client->sync_returnNullptr()
        .via(&executor_)
        .subscribe(
            [](auto) { FAIL() << "No value was expected"; },
            [](auto ex) { FAIL() << "No error was expected: " << ex.what(); },
            [&success]() { success = true; })
        .join();
    EXPECT_TRUE(success);
  });
}

TEST_P(StreamingTest, ThrowsWithResponse) {
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    EXPECT_THROW(client->sync_throwError(), Error);
  });
}

TEST_P(StreamingTest, LifeTimeTesting) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    CHECK_EQ(0, client->sync_instanceCount());

    { // Never subscribe
      auto result = client->sync_leakCheck(0, 100);
      EXPECT_EQ(1, client->sync_instanceCount());
    }
    waitNoLeak(client.get());

    { // Never subscribe to the flowable
      auto result =
          toFlowable((client->sync_leakCheck(0, 100).stream).via(&executor_));
      EXPECT_EQ(1, client->sync_instanceCount());
    }
    waitNoLeak(client.get());

    { // Drop the result stream
      client->sync_leakCheck(0, 100);
      waitNoLeak(client.get());
    }

    { // Regular usage
      auto subscriber = std::make_shared<TestSubscriber<int32_t>>(0);
      {
        auto result = toFlowable(
            std::move(client->sync_leakCheck(0, 100).stream).via(&executor_));
        result->subscribe(subscriber);
        EXPECT_EQ(1, client->sync_instanceCount());
      }
      subscriber->request(100);
      subscriber->awaitTerminalEvent();
      EXPECT_EQ(0, client->sync_instanceCount()); // no leak!
    }

    { // Early cancel
      auto subscriber = std::make_shared<TestSubscriber<int32_t>>(0);
      {
        auto result = toFlowable(
            std::move(client->sync_leakCheck(0, 100).stream).via(&executor_));
        result->subscribe(subscriber);
        EXPECT_EQ(1, client->sync_instanceCount());
      }
      EXPECT_EQ(1, client->sync_instanceCount());
      subscriber->cancel();
      waitNoLeak(client.get());
    }

    { // Early cancel - no Yarpl
      auto result = client->sync_leakCheck(0, 100);
      EXPECT_EQ(1, client->sync_instanceCount());

      auto subscription =
          std::move(result.stream).via(&executor_).subscribe([](auto) {}, 0);
      subscription.cancel();
      std::move(subscription).join();

      // Check that the cancellation has reached to the server safely
      waitNoLeak(client.get());
    }

    { // Always alive
      {
        auto subscriber = std::make_shared<TestSubscriber<int32_t>>(0);
        {
          auto result = toFlowable(
              std::move(client->sync_leakCheck(0, 100).stream).via(&executor_));
          result->subscribe(subscriber);
          EXPECT_EQ(1, client->sync_instanceCount());
        }
        EXPECT_EQ(1, client->sync_instanceCount());
      }
      // Subscriber is still alive!
      EXPECT_EQ(1, client->sync_instanceCount());
    }
  });
}

TEST_P(StreamingTest, RequestTimeout) {
  bool withResponse = false;
  auto test =
      [this, &withResponse](std::unique_ptr<StreamServiceAsyncClient> client) {
        // This test focuses on timeout for the initial response. We will have
        // another test for timeout of each onNext calls.
        callSleep(client.get(), 1, 100, withResponse);
        callSleep(client.get(), 100, 0, withResponse);
        callSleep(client.get(), 1, 100, withResponse);
        callSleep(client.get(), 100, 0, withResponse);
        callSleep(client.get(), 2000, 500, withResponse);
        /* sleep override */
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        callSleep(client.get(), 100, 1000, withResponse);
        callSleep(client.get(), 200, 0, withResponse);
        /* Sleep to give time for all callbacks to be completed */
        /* sleep override */
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      };

  connectToServer(test);
  EXPECT_EQ(3, observer_->taskTimeout_);
  EXPECT_EQ(0, observer_->queueTimeout_);

  withResponse = true;
  connectToServer(test);
  EXPECT_EQ(6, observer_->taskTimeout_);
  EXPECT_EQ(0, observer_->queueTimeout_);
}

TEST_P(StreamingTest, OnDetachable) {
  folly::Promise<folly::Unit> detachablePromise;
  auto detachableFuture = detachablePromise.getSemiFuture();
  connectToServer(
      [&](std::unique_ptr<StreamServiceAsyncClient> client) {
        const auto timeout = std::chrono::milliseconds{100};
        auto stream = client->sync_range(0, 10);
        std::this_thread::sleep_for(timeout);
        EXPECT_FALSE(detachableFuture.isReady());

        folly::Baton<> done;

        toFlowable(std::move(stream).via(&executor_))
            ->subscribe(
                [](int) {},
                [](auto ex) { FAIL() << "Should not call onError: " << ex; },
                [&done]() { done.post(); });

        EXPECT_TRUE(done.try_wait_for(timeout));
        EXPECT_TRUE(std::move(detachableFuture).wait(timeout));
      },
      [promise = std::move(detachablePromise)]() mutable {
        promise.setValue(folly::unit);
      });
}

TEST_P(StreamingTest, ChunkTimeout) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    RpcOptions options;
    options.setChunkTimeout(std::chrono::milliseconds{10});
    auto result = client->sync_streamNever(options);
    bool failed{false};
    auto subscription =
        std::move(result.stream)
            .via(&executor_)
            .subscribe(
                [](auto) { FAIL() << "Should have failed."; },
                [&failed](auto ew) {
                  ew.with_exception([&](TTransportException& tae) {
                    CHECK_EQ(
                        TTransportException::TTransportExceptionType::TIMED_OUT,
                        tae.getType());
                    failed = true;
                  });
                },
                []() { FAIL() << "onError should be called"; });
    std::move(subscription).join();
    EXPECT_TRUE(failed);
  });
}

TEST_P(StreamingTest, UserCantBlockIOThread) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    RpcOptions options;
    options.setChunkTimeout(std::chrono::milliseconds{10});
    auto stream = client->sync_range(options, 0, 10);

    bool failed{true};
    int count = 0;
    auto subscription =
        std::move(stream)
            .via(&executor_)
            .subscribe(
                [&count](auto) {
                  // sleep, so that client will be late!
                  /* sleep override */
                  std::this_thread::sleep_for(std::chrono::milliseconds(20));
                  ++count;
                },
                [](auto) { FAIL() << "no error was expected"; },
                [&failed]() { failed = false; });
    std::move(subscription).join();
    EXPECT_FALSE(failed);
    // As there is no flow control, all of the messages will be sent from server
    // to client without waiting the user thread.
    EXPECT_EQ(10, count);
  });
}

TEST_P(StreamingTest, TwoRequestsOneTimesOut) {
  folly::Promise<folly::Unit> detachablePromise;
  auto detachableFuture = detachablePromise.getSemiFuture();

  connectToServer(
      [&, this](std::unique_ptr<StreamServiceAsyncClient> client) {
        const auto waitForMs = std::chrono::milliseconds{100};

        auto stream = client->sync_registerToMessages();
        int32_t last = 0;
        folly::Baton<> baton;
        auto subscription = std::move(stream)
                                .via(&executor_)
                                .subscribe([&last, &baton](int32_t next) {
                                  last = next;
                                  baton.post();
                                });

        client->sync_sendMessage(1, false, false);
        ASSERT_TRUE(baton.try_wait_for(std::chrono::milliseconds(100)));
        baton.reset();
        CHECK_EQ(last, 1);

        // timeout a single request
        callSleep(client.get(), 1, 100, true);

        // Still there is one stream in the client side
        std::this_thread::sleep_for(waitForMs);
        EXPECT_FALSE(detachableFuture.isReady());

        client->sync_sendMessage(2, true, false);
        ASSERT_TRUE(baton.try_wait_for(waitForMs));
        baton.reset();
        CHECK_EQ(last, 2);

        std::move(subscription).join();

        // All streams are cleaned up in the client side
        EXPECT_TRUE(std::move(detachableFuture).wait(waitForMs));
      },
      [promise = std::move(detachablePromise)]() mutable {
        promise.setValue(folly::unit);
      });
}

TEST_P(StreamingTest, StreamStarvationNoRequest) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    server_->setStreamExpireTime(std::chrono::milliseconds(10));

    auto result = client->sync_leakCheck(0, 10);
    EXPECT_EQ(1, client->sync_instanceCount());

    bool failed{false};
    int count = 0;
    auto subscription = std::move(result.stream)
                            .via(&executor_)
                            .subscribe(
                                [&count](auto) { ++count; },
                                [&failed](auto) mutable { failed = true; },
                                // request no item - starvation
                                0);
    std::move(subscription).detach();
    waitNoLeak(client.get());

    EXPECT_TRUE(failed);
    EXPECT_EQ(0, count);
  });
}

TEST_P(StreamingTest, StreamStarvationNoSubscribe) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    server_->setStreamExpireTime(std::chrono::milliseconds(10));

    auto result = client->sync_leakCheck(0, 10);
    EXPECT_EQ(1, client->sync_instanceCount());

    // Did not subscribe at all
    waitNoLeak(client.get());
  });
}

TEST_P(StreamingTest, StreamThrowsKnownException) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    bool thrown = false;
    auto stream = client->sync_streamThrows(1);
    auto subscription =
        std::move(stream)
            .via(&executor_)
            .subscribe(
                [](auto) {},
                [&thrown](folly::exception_wrapper ew) {
                  thrown = true;
                  EXPECT_TRUE(ew.is_compatible_with<FirstEx>());
                  EXPECT_TRUE(ew.with_exception([](FirstEx& ex) {
                    EXPECT_EQ(1, ex.get_errCode());
                    EXPECT_STREQ("FirstEx", ex.get_errMsg().c_str());
                  }));
                });
    std::move(subscription).join();
    EXPECT_TRUE(thrown);
  });
}

TEST_P(StreamingTest, StreamThrowsNonspecifiedException) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    bool thrown = false;
    auto stream = client->sync_streamThrows(2);
    auto subscription =
        std::move(stream)
            .via(&executor_)
            .subscribe(
                [](auto) {},
                [&thrown](folly::exception_wrapper ew) {
                  thrown = true;
                  EXPECT_TRUE(ew.is_compatible_with<TApplicationException>());
                  EXPECT_TRUE(ew.with_exception([](TApplicationException& ex) {
                    EXPECT_STREQ(
                        "testutil::testservice::SecondEx:  ::testutil::testservice::SecondEx",
                        ex.what());
                  }));
                });
    std::move(subscription).join();
    EXPECT_TRUE(thrown);
  });
}

TEST_P(StreamingTest, StreamThrowsRuntimeError) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    bool thrown = false;
    auto stream = client->sync_streamThrows(3);
    auto subscription =
        std::move(stream)
            .via(&executor_)
            .subscribe(
                [](auto) {},
                [&thrown](folly::exception_wrapper ew) {
                  thrown = true;
                  EXPECT_TRUE(ew.is_compatible_with<TApplicationException>());
                  EXPECT_TRUE(ew.with_exception([](TApplicationException& ex) {
                    EXPECT_STREQ("std::runtime_error: random error", ex.what());
                  }));
                });
    std::move(subscription).join();
    EXPECT_TRUE(thrown);
  });
}

TEST_P(StreamingTest, StreamFunctionThrowsImmediately) {
  connectToServer([](std::unique_ptr<StreamServiceAsyncClient> client) {
    bool thrown = false;
    try {
      client->sync_streamThrows(0);
    } catch (const SecondEx& ex) {
      thrown = true;
    }
    EXPECT_TRUE(thrown);
  });
}

TEST_P(StreamingTest, ResponseAndStreamThrowsKnownException) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    bool thrown = false;
    auto responseAndStream = client->sync_responseAndStreamThrows(1);
    auto stream = std::move(responseAndStream.stream);
    auto subscription =
        std::move(stream)
            .via(&executor_)
            .subscribe(
                [](auto) {},
                [&thrown](folly::exception_wrapper ew) {
                  thrown = true;
                  EXPECT_TRUE(ew.is_compatible_with<FirstEx>());
                  EXPECT_TRUE(ew.with_exception([](FirstEx& ex) {
                    EXPECT_EQ(1, ex.get_errCode());
                    EXPECT_STREQ("FirstEx", ex.get_errMsg().c_str());
                  }));
                });
    std::move(subscription).join();
    EXPECT_TRUE(thrown);
  });
}

TEST_P(StreamingTest, ResponseAndStreamFunctionThrowsImmediately) {
  connectToServer([](std::unique_ptr<StreamServiceAsyncClient> client) {
    bool thrown = false;
    try {
      client->sync_responseAndStreamThrows(0);
    } catch (const SecondEx& ex) {
      thrown = true;
    }
    EXPECT_TRUE(thrown);
  });
}

TEST_P(BlockStreamingTest, StreamBlockTaskQueue) {
  connectToServer([](std::unique_ptr<StreamServiceAsyncClient> client) {
    std::vector<apache::thrift::SemiStream<int32_t>> streams;
    for (int ind = 0; ind < 1000; ind++) {
      streams.push_back(client->sync_slowCancellation());
    }
  });
}

INSTANTIATE_TEST_CASE_P(
    StreamingTests,
    StreamingTest,
    testing::Values(false, true));

INSTANTIATE_TEST_CASE_P(
    BlockingStreamingTests,
    BlockStreamingTest,
    testing::Values(false, true));

} // namespace thrift
} // namespace apache
