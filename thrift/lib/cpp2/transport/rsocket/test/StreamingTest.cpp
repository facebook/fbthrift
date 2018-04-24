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

#include <folly/io/async/ScopedEventBaseThread.h>
#include <gflags/gflags.h>
#include <gmock/gmock.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RSocketClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/core/TransportRoutingHandler.h>
#include <thrift/lib/cpp2/transport/core/testutil/FakeServerObserver.h>
#include <thrift/lib/cpp2/transport/core/testutil/MockCallback.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/TestServiceMock.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>
#include <yarpl/flowable/TestSubscriber.h>

DECLARE_int32(num_client_connections);
DECLARE_string(transport); // ConnectionManager depends on this flag.

namespace apache {
namespace thrift {

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using namespace testing;
using namespace testutil::testservice;
using testutil::testservice::Message;
using yarpl::flowable::TestSubscriber;

// Testing transport layers for their support to Streaming
class StreamingTest : public testing::Test {
 private:
  // Event handler to attach to the Thrift server so we know when it is
  // ready to serve and also so we can determine the port it is
  // listening on.
  class TestEventHandler : public server::TServerEventHandler {
   public:
    // This is a callback that is called when the Thrift server has
    // initialized and is ready to serve RPCs.
    void preServe(const folly::SocketAddress* address) override {
      port_ = address->getPort();
      baton_.post();
    }

    int32_t waitForPortAssignment() {
      baton_.wait();
      return port_;
    }

   private:
    folly::Baton<> baton_;
    int32_t port_;
  };

 public:
  StreamingTest() {
    // override the default
    FLAGS_transport = "rsocket"; // client's transport

    handler_ = std::make_shared<StrictMock<TestServiceMock>>();
    auto cpp2PFac =
        std::make_shared<ThriftServerAsyncProcessorFactory<TestServiceMock>>(
            handler_);

    server_ = std::make_unique<ThriftServer>();
    observer_ = std::make_shared<FakeServerObserver>();
    server_->setObserver(observer_);
    server_->setPort(0);
    server_->setNumIOWorkerThreads(numIOThreads_);
    server_->setNumCPUWorkerThreads(numWorkerThreads_);
    server_->setProcessorFactory(cpp2PFac);

    server_->addRoutingHandler(
        std::make_unique<apache::thrift::RSRoutingHandler>(
            server_->getThriftProcessor(), *server_));

    auto eventHandler = std::make_shared<TestEventHandler>();
    server_->setServerEventHandler(eventHandler);
    server_->setup();

    // Get the port that the server has bound to
    port_ = eventHandler->waitForPortAssignment();
  }
  virtual ~StreamingTest() {
    if (server_) {
      server_->cleanUp();
      server_.reset();
      handler_.reset();
    }
  }

  void connectToServer(
      folly::Function<void(std::unique_ptr<StreamServiceAsyncClient>)> callMe) {
    CHECK_GT(port_, 0) << "Check if the server has started already";
    auto channel = PooledRequestChannel::newChannel(
        evbThread_.getEventBase(),
        std::make_shared<folly::ScopedEventBaseThread>(),
        [port = port_](folly::EventBase& evb) {
          return RSocketClientChannel::newChannel(
              TAsyncSocket::UniquePtr(new TAsyncSocket(&evb, "::1", port)));
        });
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

 public:
  std::shared_ptr<FakeServerObserver> observer_;
  std::shared_ptr<testing::StrictMock<testutil::testservice::TestServiceMock>>
      handler_;
  std::unique_ptr<ThriftServer> server_;
  uint16_t port_;

  int numIOThreads_{10};
  int numWorkerThreads_{10};

  folly::ScopedEventBaseThread evbThread_;
  folly::ScopedEventBaseThread executor_;
};

TEST_F(StreamingTest, SimpleStream) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    auto result = toFlowable(client->sync_range(0, 10).via(&executor_));
    int j = 0;
    folly::Baton<> done;
    result->subscribe(
        [&j](auto i) mutable { EXPECT_EQ(j++, i); },
        [](auto ex) { FAIL() << "Should not call onError: " << ex.what(); },
        [&done]() { done.post(); });
    EXPECT_TRUE(done.try_wait_for(std::chrono::milliseconds(100)));
    EXPECT_EQ(10, j);
  });
}

TEST_F(StreamingTest, FutureSimpleStream) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    auto futureRange = client->future_range(0, 10);
    auto stream = futureRange.get();
    auto result = toFlowable(std::move(stream).via(&executor_));
    int j = 0;
    folly::Baton<> done;
    result->subscribe(
        [&j](auto i) mutable { EXPECT_EQ(j++, i); },
        [](auto ex) { FAIL() << "Should not call onError: " << ex.what(); },
        [&done]() { done.post(); });
    EXPECT_TRUE(done.try_wait_for(std::chrono::milliseconds(100)));
    EXPECT_EQ(10, j);
  });
}

TEST_F(StreamingTest, CallbackSimpleStream) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    folly::Baton<> done;
    int j = 0;
    auto callback = [&done, &j, this](
                        ::apache::thrift::ClientReceiveState&& receiveState) {
      ASSERT_FALSE(receiveState.isException());
      auto stream = receiveState.extractStream();
      auto result = toFlowable(std::move(stream).via(&executor_));
      result->subscribe(
          [&j](const std::unique_ptr<folly::IOBuf>) mutable { ++j; },
          [](auto ex) { FAIL() << "Should not call onError: " << ex.what(); },
          [&done]() { done.post(); });
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
  auto test = [this,
               withResponse](std::unique_ptr<StreamServiceAsyncClient> client) {
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

} // namespace thrift
} // namespace apache
