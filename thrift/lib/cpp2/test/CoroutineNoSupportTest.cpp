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

using apache::thrift::HandlerCallback;
using apache::thrift::ScopedServerInterfaceThread;
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

  folly::Future<std::unique_ptr<SumResponse>> future_computeSum(
      std::unique_ptr<SumRequest> request) override {
    auto response = std::make_unique<SumResponse>();
    response->sum = request->x + request->y;
    return folly::makeFuture(std::move(response));
  }

  folly::Future<std::unique_ptr<SumResponse>> future_computeSumEb(
      std::unique_ptr<SumRequest> request) override {
    auto response = std::make_unique<SumResponse>();
    response->sum = request->x + request->y;
    return folly::makeFuture(std::move(response));
  }

  folly::Future<int32_t> future_computeSumPrimitive(int32_t x, int32_t y)
      override {
    return folly::makeFuture(x + y);
  }

  folly::Future<folly::Unit> future_computeSumVoid(int32_t x, int32_t y)
      override {
    voidReturnValue = x + y;
    return folly::makeFuture(folly::Unit{});
  }

  folly::Future<std::unique_ptr<SumResponse>> future_computeSumThrows(
      std::unique_ptr<SumRequest> /* request */) override {
    return folly::makeFuture<std::unique_ptr<SumResponse>>(
        folly::exception_wrapper(
            folly::in_place, std::runtime_error("Not implemented")));
  }

  folly::Future<int32_t> future_computeSumThrowsPrimitive(int32_t, int32_t)
      override {
    return folly::makeFuture<int32_t>(folly::exception_wrapper(
        folly::in_place, std::runtime_error("Not implemented")));
  }

  folly::Future<int32_t> future_noParameters() override {
    return folly::makeFuture(kNoParameterReturnValue);
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
};

#include "thrift/lib/cpp2/test/CoroutineCommonTests.h"
