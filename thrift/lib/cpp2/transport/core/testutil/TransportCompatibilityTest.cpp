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
#include <thrift/lib/cpp2/transport/core/testutil/TransportCompatibilityTest.h>

#include <folly/ScopeGuard.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/synchronization/Baton.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp2/async/HTTPClientChannel.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RSocketClientChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/core/testutil/MockCallback.h>
#include <thrift/lib/cpp2/transport/core/testutil/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>

DECLARE_bool(use_ssl);
DECLARE_string(transport);

DEFINE_string(host, "::1", "host to connect to");

namespace apache {
namespace thrift {

using namespace async;
using namespace testing;
using namespace apache::thrift;
using namespace apache::thrift::transport;
using namespace testutil::testservice;

TransportCompatibilityTest::TransportCompatibilityTest()
    : handler_(std::make_shared<
               StrictMock<testutil::testservice::TestServiceMock>>()),
      server_(std::make_unique<
              SampleServer<testutil::testservice::TestServiceMock>>(handler_)) {
}

template <typename Service>
SampleServer<Service>::SampleServer(std::shared_ptr<Service> handler)
    : handler_(std::move(handler)) {
  setupServer();
}

// Tears down after the test.
template <typename Service>
SampleServer<Service>::~SampleServer() {
  stopServer();
}

// Event handler to attach to the Thrift server so we know when it is
// ready to serve and also so we can determine the port it is
// listening on.
class TransportCompatibilityTestEventHandler
    : public server::TServerEventHandler {
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

template <typename Service>
void SampleServer<Service>::addRoutingHandler(
    std::unique_ptr<TransportRoutingHandler> routingHandler) {
  DCHECK(server_) << "First call setupServer() function";

  server_->addRoutingHandler(std::move(routingHandler));
}

template <typename Service>
ThriftServer* SampleServer<Service>::getServer() {
  DCHECK(server_) << "First call setupServer() function";

  return server_.get();
}

void TransportCompatibilityTest::addRoutingHandler(
    std::unique_ptr<TransportRoutingHandler> routingHandler) {
  return server_->addRoutingHandler(std::move(routingHandler));
}

ThriftServer* TransportCompatibilityTest::getServer() {
  return server_->getServer();
}

template <typename Service>
void SampleServer<Service>::setupServer() {
  DCHECK(!server_) << "First close the server with stopServer()";

  auto cpp2PFac =
      std::make_shared<ThriftServerAsyncProcessorFactory<Service>>(handler_);

  server_ = std::make_unique<ThriftServer>();
  observer_ = std::make_shared<FakeServerObserver>();
  server_->setObserver(observer_);
  server_->setPort(0);
  server_->setNumIOWorkerThreads(numIOThreads_);
  server_->setNumCPUWorkerThreads(numWorkerThreads_);
  server_->setProcessorFactory(cpp2PFac);
}

template <typename Service>
void SampleServer<Service>::startServer() {
  DCHECK(server_) << "First call setupServer() function";
  auto eventHandler =
      std::make_shared<TransportCompatibilityTestEventHandler>();
  server_->setServerEventHandler(eventHandler);
  server_->setup();

  // Get the port that the server has bound to
  port_ = eventHandler->waitForPortAssignment();
}

void TransportCompatibilityTest::startServer() {
  server_->startServer();
}

template <typename Service>
void SampleServer<Service>::stopServer() {
  if (server_) {
    server_->cleanUp();
    server_.reset();
    handler_.reset();
  }
}

void TransportCompatibilityTest::connectToServer(
    folly::Function<void(std::unique_ptr<TestServiceAsyncClient>)> callMe) {
  connectToServer([callMe = std::move(callMe)](
                      std::unique_ptr<TestServiceAsyncClient> client,
                      auto) mutable { callMe(std::move(client)); });
}

void TransportCompatibilityTest::connectToServer(
    folly::Function<void(
        std::unique_ptr<TestServiceAsyncClient>,
        std::shared_ptr<ClientConnectionIf>)> callMe) {
  server_->connectToServer(
      FLAGS_transport,
      [callMe = std::move(callMe)](
          std::shared_ptr<RequestChannel> channel,
          std::shared_ptr<ClientConnectionIf> connection) mutable {
        auto client =
            std::make_unique<TestServiceAsyncClient>(std::move(channel));
        callMe(std::move(client), std::move(connection));
      });
}

template <typename Service>
void SampleServer<Service>::connectToServer(
    std::string transport,
    folly::Function<void(
        std::shared_ptr<RequestChannel>,
        std::shared_ptr<ClientConnectionIf>)> callMe) {
  CHECK_GT(port_, 0) << "Check if the server has started already";
  if (transport == "header") {
    auto addr = folly::SocketAddress(FLAGS_host, port_);
    TAsyncSocket::UniquePtr sock(
        new TAsyncSocket(folly::EventBaseManager::get()->getEventBase(), addr));
    auto chan = HeaderClientChannel::newChannel(std::move(sock));
    chan->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);
    callMe(std::move(chan), nullptr);
  } else if (transport == "rsocket") {
    std::shared_ptr<RSocketClientChannel> channel;
    evbThread_.getEventBase()->runInEventBaseThreadAndWait([&]() {
      channel = RSocketClientChannel::newChannel(TAsyncSocket::UniquePtr(
          new TAsyncSocket(evbThread_.getEventBase(), FLAGS_host, port_)));
    });
    SCOPE_EXIT {
      evbThread_.getEventBase()->runInEventBaseThreadAndWait(
          [channel = std::move(channel)]() { channel->closeNow(); });
    };
    callMe(channel, nullptr);
  } else if (transport == "legacy-http2") {
    // We setup legacy http2 for synchronous calls only - we do not
    // drive this event base.
    auto executor = std::make_shared<folly::ScopedEventBaseThread>();
    auto eventBase = executor->getEventBase();
    auto channel = PooledRequestChannel::newChannel(
        eventBase,
        std::move(executor),
        [port = std::move(port_)](folly::EventBase& evb) {
          TAsyncSocket::UniquePtr socket(
              new TAsyncSocket(&evb, FLAGS_host, port));
          if (FLAGS_use_ssl) {
            auto sslContext = std::make_shared<folly::SSLContext>();
            sslContext->setAdvertisedNextProtocols({"h2", "http"});
            auto sslSocket = new TAsyncSSLSocket(
                sslContext, &evb, socket->detachFd(), false);
            sslSocket->sslConn(nullptr);
            socket.reset(sslSocket);
          }
          return HTTPClientChannel::newHTTP2Channel(std::move(socket));
        });
    callMe(std::move(channel), nullptr);
  } else {
    auto mgr = ConnectionManager::getInstance();
    auto connection = mgr->getConnection(FLAGS_host, port_);
    auto channel = ThriftClient::Ptr(new ThriftClient(connection));
    channel->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);
    callMe(std::move(channel), std::move(connection));
  }
}

void TransportCompatibilityTest::callSleep(
    TestServiceAsyncClient* client,
    int32_t timeoutMs,
    int32_t sleepMs) {
  auto cb = std::make_unique<MockCallback>(false, timeoutMs < sleepMs);
  RpcOptions opts;
  opts.setTimeout(std::chrono::milliseconds(timeoutMs));
  opts.setQueueTimeout(std::chrono::milliseconds(5000));
  client->sleep(opts, std::move(cb), sleepMs);
}

void TransportCompatibilityTest::TestConnectionStats() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    EXPECT_EQ(0, server_->observer_->connAccepted_);
    EXPECT_EQ(0, server_->observer_->activeConns_);

    EXPECT_CALL(*handler_.get(), sumTwoNumbers_(1, 2)).Times(1);
    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());

    EXPECT_EQ(1, server_->observer_->connAccepted_);
    EXPECT_EQ(server_->numIOThreads_, server_->observer_->activeConns_);
  });
}

void TransportCompatibilityTest::TestObserverSendReceiveRequests() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    EXPECT_CALL(*handler_.get(), sumTwoNumbers_(1, 2)).Times(2);
    EXPECT_CALL(*handler_.get(), add_(1));
    EXPECT_CALL(*handler_.get(), add_(2));
    EXPECT_CALL(*handler_.get(), add_(5));

    // Send a message
    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());
    EXPECT_EQ(1, client->future_add(1).get());

    auto future = client->future_add(2);
    EXPECT_EQ(3, future.get());

    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());
    EXPECT_EQ(8, client->future_add(5).get());

    // Now check the stats
    EXPECT_EQ(5, server_->observer_->sentReply_);
    EXPECT_EQ(5, server_->observer_->receivedRequest_);
  });
}

void TransportCompatibilityTest::TestRequestResponse_Simple() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    EXPECT_CALL(*handler_.get(), sumTwoNumbers_(1, 2)).Times(2);
    EXPECT_CALL(*handler_.get(), add_(1));
    EXPECT_CALL(*handler_.get(), add_(2));
    EXPECT_CALL(*handler_.get(), add_(5));

    // Send a message
    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());
    EXPECT_EQ(1, client->future_add(1).get());

    auto future = client->future_add(2);
    EXPECT_EQ(3, future.get());

    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());
    EXPECT_EQ(8, client->future_add(5).get());
  });
}

void TransportCompatibilityTest::TestRequestResponse_Sync() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    EXPECT_CALL(*handler_.get(), sumTwoNumbers_(1, 2)).Times(2);
    EXPECT_CALL(*handler_.get(), add_(1));
    EXPECT_CALL(*handler_.get(), add_(2));
    EXPECT_CALL(*handler_.get(), add_(5));

    // Send a message
    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());
    EXPECT_EQ(1, client->future_add(1).get());
    EXPECT_EQ(3, client->future_add(2).get());
    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());
    EXPECT_EQ(8, client->future_add(5).get());
  });
}

void TransportCompatibilityTest::TestRequestResponse_MultipleClients() {
  const int clientCount = 10;
  EXPECT_CALL(*handler_.get(), sumTwoNumbers_(1, 2)).Times(2 * clientCount);
  EXPECT_CALL(*handler_.get(), add_(1)).Times(clientCount);
  EXPECT_CALL(*handler_.get(), add_(2)).Times(clientCount);
  EXPECT_CALL(*handler_.get(), add_(5)).Times(clientCount);

  auto lambda = [](std::unique_ptr<TestServiceAsyncClient> client) {
    // Send a message
    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());
    EXPECT_LE(1, client->future_add(1).get());

    auto future = client->future_add(2);
    EXPECT_LE(3, future.get());

    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());
    EXPECT_LE(8, client->future_add(5).get());
  };

  std::vector<folly::ScopedEventBaseThread> threads(clientCount);
  std::vector<folly::Promise<folly::Unit>> promises(clientCount);
  std::vector<folly::Future<folly::Unit>> futures;
  for (int i = 0; i < clientCount; ++i) {
    auto& promise = promises[i];
    futures.emplace_back(promise.getFuture());
    threads[i].getEventBase()->runInEventBaseThread([&promise, lambda, this]() {
      connectToServer(lambda);
      promise.setValue();
    });
  }
  folly::collectAll(futures).get();
  threads.clear();
}

void TransportCompatibilityTest::TestRequestResponse_ExpectedException() {
  EXPECT_THROW(
      connectToServer(
          [&](auto client) { client->future_throwExpectedException(1).get(); }),
      TestServiceException);

  EXPECT_THROW(
      connectToServer(
          [&](auto client) { client->future_throwExpectedException(1).get(); }),
      TestServiceException);
}

void TransportCompatibilityTest::TestRequestResponse_UnexpectedException() {
  EXPECT_THROW(
      connectToServer([&](auto client) {
        client->future_throwUnexpectedException(2).get();
      }),
      apache::thrift::TApplicationException);

  EXPECT_THROW(
      connectToServer([&](auto client) {
        client->future_throwUnexpectedException(2).get();
      }),
      apache::thrift::TApplicationException);
}

void TransportCompatibilityTest::TestRequestResponse_Timeout() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    // These are all async calls.  The first batch of calls get
    // dispatched immediately, then there is a sleep, and then the
    // second batch of calls get dispatched.  All calls have separate
    // timeouts and different delays on the server side.
    callSleep(client.get(), 1, 100);
    callSleep(client.get(), 100, 0);
    callSleep(client.get(), 1, 100);
    callSleep(client.get(), 100, 0);
    callSleep(client.get(), 2000, 500);
    /* sleep override */
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    callSleep(client.get(), 100, 1000);
    callSleep(client.get(), 200, 0);
    /* Sleep to give time for all callbacks to be completed */
    /* sleep override */
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    CHECK_EQ(3, server_->observer_->taskTimeout_);
    CHECK_EQ(0, server_->observer_->queueTimeout_);
  });
}

void TransportCompatibilityTest::TestRequestResponse_Header() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    { // Future
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      auto future = client->header_future_headers(rpcOptions);
      auto tHeader = future.get().second;
      auto keyValue = tHeader->getHeaders();
      EXPECT_NE(keyValue.end(), keyValue.find("header_from_server"));
      EXPECT_STREQ("1", keyValue.find("header_from_server")->second.c_str());
    }

    { // Callback
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      folly::Promise<folly::Unit> executed;
      auto future = executed.getFuture();
      client->headers(
          rpcOptions,
          std::unique_ptr<RequestCallback>(
              new FunctionReplyCallback([&](ClientReceiveState&& state) {
                auto keyValue = state.header()->getHeaders();
                EXPECT_NE(keyValue.end(), keyValue.find("header_from_server"));
                EXPECT_STREQ(
                    "1", keyValue.find("header_from_server")->second.c_str());

                auto exw = TestServiceAsyncClient::recv_wrapped_headers(state);
                EXPECT_FALSE(exw);
                executed.setValue();
              })));
      auto& waited = future.wait(folly::Duration(100));
      EXPECT_TRUE(waited.isReady());
    }
  });
}

void TransportCompatibilityTest::
    TestRequestResponse_Header_ExpectedException() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    { // Future
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("expected_exception", "1");
      auto future = client->header_future_headers(rpcOptions);
      auto& waited = future.wait();
      auto& ftry = waited.getTry();
      EXPECT_TRUE(ftry.hasException());
      EXPECT_THAT(
          ftry.tryGetExceptionObject()->what(),
          HasSubstr("TestServiceException"));
    }

    { // Callback
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("expected_exception", "1");
      folly::Promise<folly::Unit> executed;
      auto future = executed.getFuture();
      client->headers(
          rpcOptions,
          std::unique_ptr<RequestCallback>(
              new FunctionReplyCallback([&](ClientReceiveState&& state) {
                auto exw = TestServiceAsyncClient::recv_wrapped_headers(state);
                EXPECT_TRUE(exw.get_exception());
                EXPECT_THAT(
                    exw.what().c_str(), HasSubstr("TestServiceException"));
                executed.setValue();
              })));
      auto& waited = future.wait(folly::Duration(100));
      CHECK(waited.isReady());
    }
  });
}

void TransportCompatibilityTest::
    TestRequestResponse_Header_UnexpectedException() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    { // Future
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("unexpected_exception", "1");
      auto future = client->header_future_headers(rpcOptions);
      EXPECT_THROW(future.get(), apache::thrift::TApplicationException);
    }

    { // Callback
      apache::thrift::RpcOptions rpcOptions;
      rpcOptions.setWriteHeader("header_from_client", "2");
      rpcOptions.setWriteHeader("unexpected_exception", "1");
      folly::Promise<folly::Unit> executed;
      auto future = executed.getFuture();
      client->headers(
          rpcOptions,
          std::unique_ptr<RequestCallback>(
              new FunctionReplyCallback([&](ClientReceiveState&& state) {
                auto exw = TestServiceAsyncClient::recv_wrapped_headers(state);
                EXPECT_TRUE(exw.get_exception());
                EXPECT_THAT(
                    exw.what().c_str(), HasSubstr("TApplicationException"));
                executed.setValue();
              })));
      auto& waited = future.wait(folly::Duration(100));
      EXPECT_TRUE(waited.isReady());
    }
  });
}

void TransportCompatibilityTest::TestRequestResponse_Saturation() {
  connectToServer([this](auto client, auto connection) {
    EXPECT_CALL(*handler_.get(), add_(3)).Times(2);
    // note that no EXPECT_CALL for add_(5)

    connection->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { connection->setMaxPendingRequests(0u); });
    EXPECT_THROW(client->sync_add(5), TTransportException);

    connection->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { connection->setMaxPendingRequests(1u); });
    EXPECT_EQ(3, client->sync_add(3));
    EXPECT_EQ(6, client->sync_add(3));
  });
}

void TransportCompatibilityTest::TestRequestResponse_Connection_CloseNow() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    // It should not reach to server: no EXPECT_CALL for add_(3)

    // Observe the behavior if the connection is closed already
    auto channel = static_cast<ClientChannel*>(client->getChannel());
    channel->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { channel->closeNow(); });

    try {
      client->future_add(3).get();
      EXPECT_TRUE(false) << "future_add should have thrown";
    } catch (TTransportException& ex) {
      EXPECT_EQ(TTransportException::NOT_OPEN, ex.getType());
    }
  });
}

void TransportCompatibilityTest::TestRequestResponse_ServerQueueTimeout() {
  connectToServer([this](
                      std::unique_ptr<TestServiceAsyncClient> client) mutable {
    int32_t numCores = sysconf(_SC_NPROCESSORS_ONLN);
    int callCount = numCores + 1; // more than the core count!

    // Queue expiration - executes some of the tasks ( = thread count)
    server_->getServer()->setQueueTimeout(std::chrono::milliseconds(10));
    server_->getServer()->setTaskExpireTime(std::chrono::milliseconds(10));
    std::vector<folly::Future<folly::Unit>> futures(callCount);
    for (int i = 0; i < callCount; ++i) {
      RpcOptions opts;
      opts.setTimeout(std::chrono::milliseconds(10));
      futures[i] = client->future_sleep(100);
    }
    int taskTimeoutCount = 0;
    int successCount = 0;
    for (auto& future : futures) {
      auto& waitedFuture = future.wait();
      auto& triedFuture = waitedFuture.getTry();
      if (triedFuture.withException([](TApplicationException& ex) {
            EXPECT_EQ(
                TApplicationException::TApplicationExceptionType::TIMEOUT,
                ex.getType());
          })) {
        ++taskTimeoutCount;
      } else {
        CHECK(!triedFuture.hasException());
        ++successCount;
      }
    }
    EXPECT_LE(1, taskTimeoutCount) << "at least 1 task is expected to timeout";
    EXPECT_LE(1, successCount) << "at least 1 task is expected to succeed";

    // Task expires - even though starts executing the tasks, all expires
    server_->getServer()->setQueueTimeout(std::chrono::milliseconds(1000));
    server_->getServer()->setUseClientTimeout(false);
    server_->getServer()->setTaskExpireTime(std::chrono::milliseconds(1));
    for (int i = 0; i < callCount; ++i) {
      futures[i] = client->future_sleep(100 + i);
    }
    taskTimeoutCount = 0;
    for (auto& future : futures) {
      auto& waitedFuture = future.wait();
      auto& triedFuture = waitedFuture.getTry();
      if (triedFuture.withException([](TApplicationException& ex) {
            EXPECT_EQ(
                TApplicationException::TApplicationExceptionType::TIMEOUT,
                ex.getType());
          })) {
        ++taskTimeoutCount;
      } else {
        CHECK(!triedFuture.hasException());
      }
    }
    EXPECT_EQ(callCount, taskTimeoutCount)
        << "all tasks are expected to be timed out";
  });
}

void TransportCompatibilityTest::TestRequestResponse_ResponseSizeTooBig() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    // Execute the function, but fail when sending the response
    EXPECT_CALL(*handler_.get(), hello_(_));

    server_->getServer()->setMaxResponseSize(1);
    try {
      std::string longName(1, 'f');
      auto result = client->future_hello(longName).get();
      EXPECT_TRUE(false) << "future_hello should have thrown";
    } catch (TApplicationException& ex) {
      EXPECT_EQ(TApplicationException::INTERNAL_ERROR, ex.getType());
    }
  });
}

void TransportCompatibilityTest::TestOneway_Simple() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    EXPECT_CALL(*handler_.get(), add_(0));
    EXPECT_CALL(*handler_.get(), addAfterDelay_(0, 5));

    client->future_addAfterDelay(0, 5).get();
    // Sleep a bit for oneway call to complete on server
    /* sleep override */
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(5, client->future_add(0).get());
  });
}

void TransportCompatibilityTest::TestOneway_WithDelay() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    EXPECT_CALL(*handler_.get(), add_(0)).Times(2);
    EXPECT_CALL(*handler_.get(), addAfterDelay_(800, 5));

    // Perform an add on the server after a delay
    client->future_addAfterDelay(800, 5).get();
    // Call add to get result before the previous addAfterDelay takes
    // place - this verifies that the addAfterDelay call is really
    // oneway.
    EXPECT_EQ(0, client->future_add(0).get());
    // Sleep to wait for oneway call to complete on server
    /* sleep override */
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    EXPECT_EQ(5, client->future_add(0).get());
  });
}

void TransportCompatibilityTest::TestOneway_Saturation() {
  connectToServer([this](auto client, auto connection) {
    EXPECT_CALL(*handler_.get(), add_(3));
    // note that no EXPECT_CALL for addAfterDelay_(0, 5)

    connection->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { connection->setMaxPendingRequests(0u); });
    client->sync_addAfterDelay(0, 5);

    connection->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { connection->setMaxPendingRequests(1u); });
    EXPECT_EQ(3, client->sync_add(3));
  });
}

void TransportCompatibilityTest::TestOneway_UnexpectedException() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    EXPECT_CALL(*handler_.get(), onewayThrowsUnexpectedException_(100));
    EXPECT_CALL(*handler_.get(), onewayThrowsUnexpectedException_(0));
    client->future_onewayThrowsUnexpectedException(100).get();
    client->future_onewayThrowsUnexpectedException(0).get();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  });
}

void TransportCompatibilityTest::TestOneway_Connection_CloseNow() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    // It should not reach server - no EXPECT_CALL for addAfterDelay_(0, 5)

    // Observe the behavior if the connection is closed already
    auto channel = static_cast<ClientChannel*>(client->getChannel());
    channel->getEventBase()->runInEventBaseThreadAndWait(
        [&]() { channel->closeNow(); });

    EXPECT_THROW(
        client->future_addAfterDelay(0, 5).get(),
        apache::thrift::TTransportException);
  });
}

void TransportCompatibilityTest::TestOneway_ServerQueueTimeout() {
  // TODO: Even though we observe that the timeout functionality works fine for
  // Oneway PRC calls, the AsyncProcesor still executes the `cancelled`
  // requests.
  connectToServer(
      [this](std::unique_ptr<TestServiceAsyncClient> client) mutable {
        int32_t numCores = sysconf(_SC_NPROCESSORS_ONLN);
        int callCount = numCores + 1; // more than the core count!

        // TODO: fixme T22871783: Oneway tasks don't get cancelled
        EXPECT_CALL(*handler_.get(), addAfterDelay_(100, 5))
            .Times(AtMost(2 * callCount));

        server_->getServer()->setQueueTimeout(std::chrono::milliseconds(1));
        for (int i = 0; i < callCount; ++i) {
          EXPECT_NO_THROW(client->future_addAfterDelay(100, 5).get());
        }

        server_->getServer()->setQueueTimeout(std::chrono::milliseconds(1000));
        server_->getServer()->setUseClientTimeout(false);
        server_->getServer()->setTaskExpireTime(std::chrono::milliseconds(1));
        for (int i = 0; i < callCount; ++i) {
          EXPECT_NO_THROW(client->future_addAfterDelay(100, 5).get());
        }
      });
}

void TransportCompatibilityTest::TestRequestContextIsPreserved() {
  EXPECT_CALL(*handler_.get(), add_(5)).Times(1);

  // A separate server/client is spun up to verify that a client backed by a new
  // transport behaves correctly and does not trample the currently set
  // RequestContext. In this case, a THeader server is spun up, which is known
  // to correctly set RequestContext. A request/response is made through the
  // transport being tested, and it's verified that the RequestContext doesn't
  // change.

  auto service = std::make_shared<StrictMock<IntermHeaderService>>(
      FLAGS_host, server_->port_);

  SampleServer<IntermHeaderService> server(service);
  server.startServer();

  server.connectToServer(
      "header", [](std::shared_ptr<RequestChannel> channel, auto) mutable {
        auto client = std::make_unique<IntermHeaderServiceAsyncClient>(
            std::move(channel));
        EXPECT_EQ(5, client->sync_callAdd(5));
      });

  server.stopServer();
}

void TransportCompatibilityTest::TestBadPayload() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    auto cb = std::make_unique<MockCallback>(true, false);
    auto channel = static_cast<ClientChannel*>(client->getChannel());
    channel->getEventBase()->runInEventBaseThreadAndWait([&]() {
      auto metadata = std::make_unique<RequestRpcMetadata>();
      metadata->set_clientTimeoutMs(10000);
      metadata->set_kind(RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE);
      metadata->set_name("name");
      metadata->set_seqId(0);
      metadata->set_protocol(ProtocolId::BINARY);

      // Put a bad payload!
      auto payload = std::make_unique<folly::IOBuf>();

      RpcOptions rpcOptions;
      auto ctx = std::make_unique<ContextStack>("temp");
      auto header = std::make_shared<THeader>();
      channel->sendRequest(
          rpcOptions,
          std::move(cb),
          std::move(ctx),
          std::move(payload),
          std::move(header));
    });
  });
}

void TransportCompatibilityTest::TestEvbSwitch() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    EXPECT_CALL(*handler_.get(), sumTwoNumbers_(1, 2)).Times(3);

    folly::ScopedEventBaseThread sevbt;

    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());

    auto channel = static_cast<ClientChannel*>(client->getChannel());
    auto evb = channel->getEventBase();
    evb->runInEventBaseThreadAndWait([&]() {
      EXPECT_TRUE(channel->isDetachable());

      channel->detachEventBase();
    });

    sevbt.getEventBase()->runInEventBaseThreadAndWait(
        [&]() { channel->attachEventBase(sevbt.getEventBase()); });

    // Execution happens on the new event base
    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());

    // Attach the old one back
    sevbt.getEventBase()->runInEventBaseThreadAndWait([&]() {
      EXPECT_TRUE(channel->isDetachable());
      channel->detachEventBase();
    });

    evb->runInEventBaseThreadAndWait([&]() { channel->attachEventBase(evb); });

    // Execution happens on the old event base, along with the destruction
    EXPECT_EQ(3, client->future_sumTwoNumbers(1, 2).get());
  });
}

void TransportCompatibilityTest::TestEvbSwitch_Failure() {
  connectToServer([this](std::unique_ptr<TestServiceAsyncClient> client) {
    auto channel = static_cast<ClientChannel*>(client->getChannel());
    auto evb = channel->getEventBase();
    // If isDetachable() is called when a function is executing, it should
    // not be detachable
    callSleep(client.get(), 5000, 1000);
    /* sleep override - make sure request is started */
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    evb->runInEventBaseThreadAndWait([&]() {
      // As we have an active request, it should not be detachable!
      EXPECT_FALSE(channel->isDetachable());
    });

    // Once the request finishes, it should be detachable again
    /* sleep override - make sure request is finished */
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    evb->runInEventBaseThreadAndWait([&]() {
      // As we have an active request, it should not be detachable!
      EXPECT_TRUE(channel->isDetachable());
    });

    // If the latest request is sent while previous ones are still finishing
    // it should still not be detachable
    EXPECT_CALL(*handler_.get(), sumTwoNumbers_(1, 2)).Times(1);
    callSleep(client.get(), 5000, 1000);
    /* sleep override - make sure request is started */
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    client->future_sumTwoNumbers(1, 2).get();
    evb->runInEventBaseThreadAndWait([&]() {
      // As we have an active request, it should not be detachable!
      EXPECT_FALSE(channel->isDetachable());
    });

    /* sleep override - make sure request is finished */
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    evb->runInEventBaseThreadAndWait([&]() {
      // Should be detachable now
      EXPECT_TRUE(channel->isDetachable());
      // Detach to prove that we can destroy the object even if evb is detached
      channel->detachEventBase();
    });
  });
}

class CloseCallbackTest : public CloseCallback {
 public:
  void channelClosed() override {
    EXPECT_FALSE(closed_);
    closed_ = true;
  }
  bool isClosed() {
    return closed_;
  }

 private:
  bool closed_{false};
};

void TransportCompatibilityTest::TestCloseCallback() {
  connectToServer([](std::unique_ptr<TestServiceAsyncClient> client) {
    auto closeCb = std::make_unique<CloseCallbackTest>();
    auto channel = static_cast<ClientChannel*>(client->getChannel());
    channel->setCloseCallback(closeCb.get());

    EXPECT_FALSE(closeCb->isClosed());
    auto evb = channel->getEventBase();
    evb->runInEventBaseThreadAndWait([&]() { channel->closeNow(); });
    EXPECT_TRUE(closeCb->isClosed());
  });
}

} // namespace thrift
} // namespace apache
