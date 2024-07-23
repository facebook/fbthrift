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

#include <folly/experimental/coro/GtestHelpers.h>
#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/async/ClientInterceptorBase.h>
#include <thrift/lib/cpp2/async/HTTPClientChannel.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/async/tests/gen-cpp2/ClientInterceptor_clients.h>
#include <thrift/lib/cpp2/async/tests/gen-cpp2/ClientInterceptor_handlers.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace apache::thrift;
using namespace ::testing;

namespace {

using TransportType = Cpp2ConnContext::TransportType;

std::unique_ptr<HTTP2RoutingHandler> createHTTP2RoutingHandler(
    ThriftServer& server) {
  auto h2Options = std::make_unique<proxygen::HTTPServerOptions>();
  h2Options->threads = static_cast<size_t>(server.getNumIOWorkerThreads());
  h2Options->idleTimeout = server.getIdleTimeout();
  h2Options->shutdownOn = {SIGINT, SIGTERM};
  return std::make_unique<HTTP2RoutingHandler>(
      std::move(h2Options), server.getThriftProcessor(), server);
}

struct TestHandler
    : apache::thrift::ServiceHandler<test::ClientInterceptorTest> {
  folly::coro::Task<void> co_noop() override { co_return; }

  folly::coro::Task<std::unique_ptr<std::string>> co_echo(
      std::unique_ptr<std::string> str) override {
    if (*str == "throw") {
      throw std::runtime_error("You asked for it!");
    }
    co_return std::move(str);
  }
};

class ClientInterceptorTestP : public ::testing::TestWithParam<TransportType> {
 public:
  TransportType transportType() const { return GetParam(); }

 private:
  void SetUp() override {
    runner = std::make_unique<ScopedServerInterfaceThread>(
        std::make_shared<TestHandler>());
    if (transportType() == TransportType::HTTP2) {
      auto& thriftServer = runner->getThriftServer();
      thriftServer.addRoutingHandler(createHTTP2RoutingHandler(thriftServer));
    }
  }
  std::unique_ptr<ScopedServerInterfaceThread> runner;

  ScopedServerInterfaceThread::MakeChannelFunc channelFor(
      TransportType transportType) {
    return [transportType](
               folly::AsyncSocket::UniquePtr socket) -> RequestChannel::Ptr {
      switch (transportType) {
        case TransportType::HEADER:
          return HeaderClientChannel::newChannel(std::move(socket));
        case TransportType::ROCKET:
          return RocketClientChannel::newChannel(std::move(socket));
        case TransportType::HTTP2: {
          auto channel = HTTPClientChannel::newHTTP2Channel(std::move(socket));
          channel->setProtocolId(protocol::T_COMPACT_PROTOCOL);
          return channel;
        }
        default:
          throw std::logic_error{"Unreachable!"};
      }
    };
  }

  std::shared_ptr<RequestChannel> makeChannel() {
    return runner
        ->newClient<apache::thrift::Client<test::ClientInterceptorTest>>(
            nullptr, channelFor(transportType()))
        ->getChannelShared();
  }

 public:
  template <typename ServiceTag = test::ClientInterceptorTest>
  std::unique_ptr<apache::thrift::Client<ServiceTag>> makeClient(
      std::shared_ptr<std::vector<std::shared_ptr<ClientInterceptorBase>>>
          interceptors) {
    return std::make_unique<apache::thrift::Client<ServiceTag>>(
        makeChannel(), std::move(interceptors));
  }
};

template <class... InterceptorPtrs>
std::shared_ptr<std::vector<std::shared_ptr<ClientInterceptorBase>>>
makeInterceptorsList(InterceptorPtrs&&... interceptors) {
  auto list =
      std::make_shared<std::vector<std::shared_ptr<ClientInterceptorBase>>>();
  (list->emplace_back(std::forward<InterceptorPtrs>(interceptors)), ...);
  return list;
}

struct NamedClientInterceptor : public ClientInterceptorBase {
  explicit NamedClientInterceptor(std::string name) : name_(std::move(name)) {}

  std::string getName() const override { return name_; }

 private:
  std::string name_;
};

class ClientInterceptorCountingCalls : public NamedClientInterceptor {
 public:
  using NamedClientInterceptor::NamedClientInterceptor;

  folly::coro::Task<void> internal_onRequest(RequestInfo) override {
    onRequestCount++;
    co_return;
  }

  folly::coro::Task<void> internal_onResponse(ResponseInfo) override {
    onResponseCount++;
    co_return;
  }

  int onRequestCount = 0;
  int onResponseCount = 0;
};

class ClientInterceptorThatThrowsOnRequest
    : public ClientInterceptorCountingCalls {
 public:
  using ClientInterceptorCountingCalls::ClientInterceptorCountingCalls;

  folly::coro::Task<void> internal_onRequest(RequestInfo requestInfo) override {
    co_await ClientInterceptorCountingCalls::internal_onRequest(
        std::move(requestInfo));
    throw std::runtime_error("Oh no!");
  }
};

class ClientInterceptorThatThrowsOnResponse
    : public ClientInterceptorCountingCalls {
 public:
  using ClientInterceptorCountingCalls::ClientInterceptorCountingCalls;

  folly::coro::Task<void> internal_onResponse(
      ResponseInfo responseInfo) override {
    co_await ClientInterceptorCountingCalls::internal_onResponse(
        std::move(responseInfo));
    throw std::runtime_error("Oh no!");
  }
};

} // namespace

CO_TEST_P(ClientInterceptorTestP, BasicCoro) {
  auto interceptor =
      std::make_shared<ClientInterceptorCountingCalls>("Interceptor1");
  auto client = makeClient(makeInterceptorsList(interceptor));

  co_await client->co_echo("foo");
  EXPECT_EQ(interceptor->onRequestCount, 1);
  EXPECT_EQ(interceptor->onResponseCount, 1);

  co_await client->co_noop();
  EXPECT_EQ(interceptor->onRequestCount, 2);
  EXPECT_EQ(interceptor->onResponseCount, 2);
}

CO_TEST_P(ClientInterceptorTestP, CoroOnRequestException) {
  auto interceptor1 =
      std::make_shared<ClientInterceptorThatThrowsOnRequest>("Interceptor1");
  auto interceptor2 =
      std::make_shared<ClientInterceptorCountingCalls>("Interceptor2");
  auto interceptor3 =
      std::make_shared<ClientInterceptorThatThrowsOnRequest>("Interceptor3");
  auto client = makeClient(
      makeInterceptorsList(interceptor1, interceptor2, interceptor3));

  EXPECT_THROW(
      {
        try {
          co_await client->co_noop();
        } catch (const ClientInterceptorException& ex) {
          EXPECT_EQ(ex.causes().size(), 2);
          EXPECT_EQ(ex.causes()[0].sourceInterceptorName, "Interceptor1");
          EXPECT_EQ(ex.causes()[1].sourceInterceptorName, "Interceptor3");
          EXPECT_THAT(ex.what(), HasSubstr("[Interceptor1]"));
          EXPECT_THAT(ex.what(), Not(HasSubstr("Interceptor2")));
          EXPECT_THAT(ex.what(), HasSubstr("[Interceptor3]"));
          EXPECT_THAT(ex.what(), HasSubstr("ClientInterceptor::onRequest"));
          throw;
        }
      },
      ClientInterceptorException);
  EXPECT_EQ(interceptor1->onRequestCount, 1);
  EXPECT_EQ(interceptor2->onRequestCount, 1);
  EXPECT_EQ(interceptor3->onRequestCount, 1);
  EXPECT_EQ(interceptor1->onResponseCount, 0);
  EXPECT_EQ(interceptor2->onResponseCount, 0);
  EXPECT_EQ(interceptor3->onResponseCount, 0);
}

CO_TEST_P(ClientInterceptorTestP, CoroOnResponseException) {
  auto interceptor1 =
      std::make_shared<ClientInterceptorThatThrowsOnResponse>("Interceptor1");
  auto interceptor2 =
      std::make_shared<ClientInterceptorCountingCalls>("Interceptor2");
  auto interceptor3 =
      std::make_shared<ClientInterceptorThatThrowsOnResponse>("Interceptor3");
  auto client = makeClient(
      makeInterceptorsList(interceptor1, interceptor2, interceptor3));

  EXPECT_THROW(
      {
        try {
          co_await client->co_noop();
        } catch (const ClientInterceptorException& ex) {
          EXPECT_EQ(ex.causes().size(), 2);
          EXPECT_EQ(ex.causes()[0].sourceInterceptorName, "Interceptor1");
          EXPECT_EQ(ex.causes()[1].sourceInterceptorName, "Interceptor3");
          EXPECT_THAT(ex.what(), HasSubstr("[Interceptor1]"));
          EXPECT_THAT(ex.what(), Not(HasSubstr("Interceptor2")));
          EXPECT_THAT(ex.what(), HasSubstr("[Interceptor3]"));
          EXPECT_THAT(ex.what(), HasSubstr("ClientInterceptor::onResponse"));
          throw;
        }
      },
      ClientInterceptorException);
  EXPECT_EQ(interceptor1->onRequestCount, 1);
  EXPECT_EQ(interceptor2->onRequestCount, 1);
  EXPECT_EQ(interceptor3->onRequestCount, 1);
  EXPECT_EQ(interceptor1->onResponseCount, 1);
  EXPECT_EQ(interceptor2->onResponseCount, 1);
  EXPECT_EQ(interceptor3->onResponseCount, 1);
}

INSTANTIATE_TEST_SUITE_P(
    ClientInterceptorTestP,
    ClientInterceptorTestP,
    ::testing::Values(
        TransportType::HEADER, TransportType::ROCKET, TransportType::HTTP2));