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

#include <folly/executors/SerialExecutor.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gflags/gflags.h>
#include <gmock/gmock.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/RSocketClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/core/TransportRoutingHandler.h>
#include <thrift/lib/cpp2/transport/core/testutil/FakeServerObserver.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/TestServiceMock.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>

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
  StreamingTest() : executor_() {
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
      folly::Function<void(
          std::unique_ptr<testutil::testservice::StreamServiceAsyncClient>)>
          callMe) {
    CHECK_GT(port_, 0) << "Check if the server has started already";
    RSocketClientChannel::Ptr channel;
    evbThread_.getEventBase()->runInEventBaseThreadAndWait([&]() {
      channel = RSocketClientChannel::newChannel(TAsyncSocket::UniquePtr(
          new TAsyncSocket(evbThread_.getEventBase(), "::1", port_)));
    });
    callMe(std::make_unique<StreamServiceAsyncClient>(std::move(channel)));
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
  folly::SerialExecutor executor_;
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

TEST_F(StreamingTest, SimpleChannel) {
  connectToServer([this](std::unique_ptr<StreamServiceAsyncClient> client) {
    auto input = yarpl::flowable::Flowable<>::range(0, 10)->map(
        [](auto i) { return (int32_t)i; });
    auto result =
        toFlowable(client->sync_prefixSumIOThread(toStream(input, &executor_))
                       .via(&executor_));
    int j = 0, k = 1;
    folly::Baton<> done;
    result->subscribe(
        [&j, k](auto i) mutable {
          EXPECT_EQ(j, i);
          j = j + k;
          ++k;
        },
        [](auto ex) { FAIL() << "Should not call onError: " << ex.what(); },
        [&done]() { done.post(); });
    EXPECT_TRUE(done.try_wait_for(std::chrono::milliseconds(100)));
    EXPECT_EQ(55, j);
  });
}

TEST_F(StreamingTest, DefaultStreamImplementation) {
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    EXPECT_THROW(
        toFlowable(client->sync_nonImplementedStream("test").via(&executor_)),
        apache::thrift::TApplicationException);
  });
}

TEST_F(StreamingTest, DefaultChannelImplementation) {
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    auto input = yarpl::flowable::Flowable<>::just(Message());
    EXPECT_THROW(
        toFlowable(client
                       ->sync_nonImplementedChannel(
                           toStream(input, &executor_), "test")
                       .via(&executor_)),
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

TEST_F(StreamingTest, ThrowsException) {
  // User function throws an exception.
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    auto input = yarpl::flowable::Flowable<>::just(Message());
    EXPECT_THROW(
        toFlowable(client->sync_throwException(toStream(input, &executor_))
                       .via(&executor_)),
        apache::thrift::TApplicationException);
  });
}

TEST_F(StreamingTest, ThrowsWithResponse) {
  connectToServer([&](std::unique_ptr<StreamServiceAsyncClient> client) {
    EXPECT_THROW(client->sync_throwError(), Error);
  });
}

} // namespace thrift
} // namespace apache
