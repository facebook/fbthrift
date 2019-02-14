/*
 * Copyright 2019 Facebook, Inc.
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

#include <exception>

#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/test/gen-cpp2/Coroutine.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using apache::thrift::Cpp2RequestContext;
using apache::thrift::RequestParams;
using apache::thrift::ScopedServerInterfaceThread;
using apache::thrift::concurrency::ThreadManager;
using folly::EventBase;
using folly::EventBaseManager;

using apache::thrift::test::CoroutineAsyncClient;
using apache::thrift::test::CoroutineSvIf;
using apache::thrift::test::CoroutineSvNull;
using apache::thrift::test::SumRequest;
using apache::thrift::test::SumResponse;

const static int kNoParameterReturnValue = 123;
static int voidReturnValue;

class CoroutineServiceHandler : virtual public CoroutineSvIf {
 public:
  void computeSumNoCoro(
      SumResponse& response,
      std::unique_ptr<SumRequest> request) override {
    response.sum = request->x + request->y;
  }

  folly::coro::Task<std::unique_ptr<SumResponse>> co_computeSum(
      std::unique_ptr<SumRequest> request) override {
    auto response = std::make_unique<SumResponse>();
    response->sum = request->x + request->y;
    co_return response;
  }

  folly::coro::Task<std::unique_ptr<SumResponse>> co_computeSumEb(
      std::unique_ptr<SumRequest> request) override {
    auto response = std::make_unique<SumResponse>();
    response->sum = request->x + request->y;
    co_return response;
  }

  folly::coro::Task<int32_t> co_computeSumPrimitive(int32_t x, int32_t y)
      override {
    co_return x + y;
  }

  folly::coro::Task<void> co_computeSumVoid(int32_t x, int32_t y) override {
    voidReturnValue = x + y;
    co_return;
  }

  folly::coro::Task<std::unique_ptr<SumResponse>> co_computeSumThrows(
      std::unique_ptr<SumRequest> /* request */) override {
    co_await std::experimental::suspend_never{};
    throw std::runtime_error("Not implemented");
  }

  folly::coro::Task<int32_t> co_computeSumThrowsPrimitive(int32_t, int32_t)
      override {
    co_await std::experimental::suspend_never{};
    throw std::runtime_error("Not implemented");
  }

  folly::coro::Task<int32_t> co_noParameters() override {
    co_return kNoParameterReturnValue;
  }

  folly::Future<std::unique_ptr<SumResponse>> future_implementedWithFutures()
      override {
    auto result = std::make_unique<SumResponse>();
    result->sum = kNoParameterReturnValue;
    return folly::makeFuture(std::move(result));
  }

  folly::Future<int32_t> future_implementedWithFuturesPrimitive() override {
    return folly::makeFuture(kNoParameterReturnValue);
  }

  folly::coro::Task<int32_t> co_takesRequestParams(
      RequestParams params) override {
    Cpp2RequestContext* requestContext = params.getRequestContext();
    ThreadManager* threadManager = params.getThreadManager();
    EventBase* eventBase = params.getEventBase();
    // It's hard to check that these pointers are what we expect them to be; we
    // can at least make sure that they point to valid memory, though.
    *(volatile char*)requestContext;
    *(volatile char*)threadManager;
    *(volatile char*)eventBase;
    co_return 0;
  }
};

#include "thrift/lib/cpp2/test/CoroutineCommonTests.h"
