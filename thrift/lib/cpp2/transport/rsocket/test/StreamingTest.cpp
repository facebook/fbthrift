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

#include <thrift/lib/cpp2/transport/rsocket/test/util/TestServiceMock.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/TestUtil.h>

namespace apache {
namespace thrift {

// Testing transport layers for their support to Streaming
class StreamingTest : public TestSetup {
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
    auto channel = connectToServer(port_, std::move(onDetachable));
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

TEST_F(StreamingTest, SimpleStream) {
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

TEST_F(StreamingTest, FutureSimpleStream) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    auto futureRange = client->future_range(0, 10);
    auto stream = futureRange.get();
    auto result = std::move(stream).via(&executor_);
    int j = 0;
    auto subscription = std::move(result).subscribe(
        [&j](auto i) mutable { EXPECT_EQ(j++, i); },
        [](auto ex) { FAIL() << "Should not call onError: " << ex.what(); });
    std::move(subscription).join();
    EXPECT_EQ(10, j);
  });
}

TEST_F(StreamingTest, CallbackSimpleStream) {
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

TEST_F(StreamingTest, DefaultStreamImplementation) {
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    EXPECT_THROW(
        toFlowable(client->sync_nonImplementedStream("test").via(&executor_)),
        apache::thrift::TApplicationException);
  });
}

TEST_F(StreamingTest, ReturnsNullptr) {
  // User function should return a Stream, but it returns a nullptr.
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    EXPECT_THROW(
        client->sync_returnNullptr(), apache::thrift::TApplicationException);
  });
}

TEST_F(StreamingTest, ThrowsWithResponse) {
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    EXPECT_THROW(client->sync_throwError(), Error);
  });
}

TEST_F(StreamingTest, LifeTimeTesting) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    auto waitNoLeak = [&] {
      auto deadline =
          std::chrono::steady_clock::now() + std::chrono::milliseconds{100};
      do {
        std::this_thread::yield();
        if (client->sync_instanceCount() == 0) {
          return;
        }
      } while (std::chrono::steady_clock::now() < deadline);
      CHECK(false);
    };

    CHECK_EQ(0, client->sync_instanceCount());

    { // Never subscribe
      auto result = client->sync_leakCheck(0, 100);
      EXPECT_EQ(1, client->sync_instanceCount());
    }
    waitNoLeak();

    { // Never subscribe to the flowable
      auto result =
          toFlowable((client->sync_leakCheck(0, 100).stream).via(&executor_));
      EXPECT_EQ(1, client->sync_instanceCount());
    }
    waitNoLeak();

    { // Drop the result stream
      client->sync_leakCheck(0, 100);
      waitNoLeak();
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
      waitNoLeak();
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

TEST_F(StreamingTest, RequestTimeout) {
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

TEST_F(StreamingTest, OnDetachable) {
  folly::Promise<folly::Unit> detachablePromise;
  auto detachableFuture = detachablePromise.getSemiFuture();
  connectToServer(
      [&](std::unique_ptr<StreamServiceAsyncClient> client) {
        const auto timeout = std::chrono::milliseconds{100};
        auto stream = client->sync_range(0, 10);
        EXPECT_FALSE(detachableFuture.wait(timeout).isReady());

        folly::Baton<> done;

        toFlowable(std::move(stream).via(&executor_))
            ->subscribe(
                [](int) {},
                [](auto ex) { FAIL() << "Should not call onError: " << ex; },
                [&done]() { done.post(); });

        EXPECT_TRUE(done.try_wait_for(timeout));
        EXPECT_TRUE(detachableFuture.wait(timeout).isReady());
      },
      [promise = std::move(detachablePromise)]() mutable {
        promise.setValue(folly::unit);
      });
}

TEST_F(StreamingTest, ChunkTimeout) {
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

TEST_F(StreamingTest, TwoRequestsOneTimesOut) {
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
        EXPECT_FALSE(detachableFuture.wait(waitForMs).isReady());

        client->sync_sendMessage(2, true, false);
        ASSERT_TRUE(baton.try_wait_for(waitForMs));
        baton.reset();
        CHECK_EQ(last, 2);

        std::move(subscription).join();

        // All streams are cleaned up in the client side
        EXPECT_TRUE(detachableFuture.wait(waitForMs).isReady());
      },
      [promise = std::move(detachablePromise)]() mutable {
        promise.setValue(folly::unit);
      });
}

} // namespace thrift
} // namespace apache
