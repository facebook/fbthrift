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

#include <folly/Baton.h>
#include <folly/ExceptionWrapper.h>
#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/server/BaseThriftServer.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/cpp2/transport/core/testutil/ServerConfigsMock.h>
#include <thrift/lib/cpp2/transport/inmemory/InMemoryConnection.h>
#include <thrift/lib/cpp2/transport/inmemory/test/gen-cpp2/Division.h>
#include <memory>

namespace apache {
namespace thrift {

// The three classes below implement sync, async, and future style servers.

class DivisionHandlerSync : virtual public DivisionSvIf {
 public:
  int divide(int x, int y) override {
    if (y == 0) {
      throw DivideByZero();
    } else {
      return x / y;
    }
  }
};

class DivisionHandlerAsync : virtual public DivisionSvIf {
 public:
  void async_tm_divide(
      std::unique_ptr<HandlerCallback<int32_t>> callback,
      int32_t x,
      int32_t y) override {
    if (y == 0) {
      callback->exception(folly::make_exception_wrapper<DivideByZero>());
    } else {
      callback->result(x / y);
    }
  }
};

class DivisionHandlerFuture : virtual public DivisionSvIf {
 public:
  folly::Future<int32_t> future_divide(int32_t x, int32_t y) override {
    if (y == 0) {
      return folly::makeFuture<int32_t>(DivideByZero());
    } else {
      return folly::makeFuture<int32_t>(x / y);
    }
  }
};

// This class is for the async client calls.
class DivisionRequestCallback : public RequestCallback {
 public:
  DivisionRequestCallback(ClientReceiveState* result, folly::Baton<>* baton)
      : result_(result), baton_(baton) {}

  void requestSent() override {}

  void replyReceived(ClientReceiveState&& state) override {
    *result_ = std::move(state);
    baton_->post();
  }

  void requestError(ClientReceiveState&& state) override {
    *result_ = std::move(state);
    baton_->post();
  }

 private:
  ClientReceiveState* result_;
  folly::Baton<>* baton_;
};

// This method tests sync, async, and future style clients with the
// handler specified as the template parameter.  For each of these 3
// cases, it calls the divide service method 3 times:
//    divide(15, 5); divide(1, 0); divide(7, 1);
template <class Handler>
void testClientWithHandler() {
  auto handler = std::make_shared<Handler>();
  auto pFac =
      std::make_shared<ThriftServerAsyncProcessorFactory<Handler>>(handler);
  apache::thrift::server::ServerConfigsMock serverConfigs;
  auto connection = std::make_shared<InMemoryConnection>(pFac, serverConfigs);
  auto thriftClient = ThriftClient::Ptr(new ThriftClient(connection));
  thriftClient->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);
  auto divisionClient =
      std::make_unique<DivisionAsyncClient>(std::move(thriftClient));

  // Synchronous client
  EXPECT_EQ(3, divisionClient->sync_divide(15, 5));
  EXPECT_THROW({ divisionClient->sync_divide(1, 0); }, DivideByZero);
  EXPECT_EQ(7, divisionClient->sync_divide(7, 1));

  // Asynchronous client
  {
    ClientReceiveState result;
    folly::Baton<> baton;
    auto callback = std::make_unique<DivisionRequestCallback>(&result, &baton);
    divisionClient->divide(std::move(callback), 15, 5);
    baton.wait();
    int32_t val;
    auto ew = divisionClient->recv_wrapped_divide(val, result);
    EXPECT_FALSE(ew);
    EXPECT_EQ(3, val);
  }
  {
    ClientReceiveState result;
    folly::Baton<> baton;
    auto callback = std::make_unique<DivisionRequestCallback>(&result, &baton);
    divisionClient->divide(std::move(callback), 1, 0);
    baton.wait();
    int32_t val;
    auto ew = divisionClient->recv_wrapped_divide(val, result);
    EXPECT_TRUE(ew);
    EXPECT_EQ(typeid(DivideByZero), ew.type());
  }
  {
    ClientReceiveState result;
    folly::Baton<> baton;
    auto callback = std::make_unique<DivisionRequestCallback>(&result, &baton);
    divisionClient->divide(std::move(callback), 7, 1);
    baton.wait();
    int32_t val;
    auto ew = divisionClient->recv_wrapped_divide(val, result);
    EXPECT_FALSE(ew);
    EXPECT_EQ(7, val);
  }

  // Future client
  EXPECT_EQ(3, divisionClient->future_divide(15, 5).get());
  EXPECT_THROW({ divisionClient->future_divide(1, 0).get(); }, DivideByZero);
  EXPECT_EQ(7, divisionClient->future_divide(7, 1).get());
}

// The 3 tests below run the each style of server as described in the
// comments in testClientWithHandler.
//
// I.e., 3 styles of servers X 3 styles of clients X 3 calls each
// (so 27 test cases).

TEST(InMemoryConnection, SyncServer) {
  testClientWithHandler<DivisionHandlerSync>();
}

TEST(InMemoryConnection, AsyncServer) {
  testClientWithHandler<DivisionHandlerAsync>();
}

TEST(InMemoryConnection, FutureServer) {
  testClientWithHandler<DivisionHandlerFuture>();
}

} // namespace thrift
} // namespace apache
