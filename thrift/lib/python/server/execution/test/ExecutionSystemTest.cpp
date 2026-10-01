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

#include <thrift/lib/python/server/execution/ExecutionSystem.h>

#include <optional>
#include <ostream>
#include <stdexcept>
#include <utility>

#include <folly/Try.h>
#include <folly/executors/ManualExecutor.h>
#include <folly/futures/Future.h>
#include <folly/portability/GTest.h>

namespace apache::thrift::python::execution {
namespace {

class ResponseCallback final {
 public:
  void complete(folly::Try<int> result) {
    response_ = std::move(result).value();
  }

  const std::optional<int>& response() const { return response_; }

 private:
  std::optional<int> response_;
};

struct DispatchFailureTrace {
  bool exceptionEscaped;
  bool nextWorkExecuted;

  bool operator==(const DispatchFailureTrace&) const = default;

  friend void PrintTo(const DispatchFailureTrace& trace, std::ostream* out) {
    *out << "{exceptionEscaped: " << trace.exceptionEscaped
         << ", nextWorkExecuted: " << trace.nextWorkExecuted << '}';
  }
};

struct SynchronousDispatchFailureTrace {
  bool exceptionEscaped;
  bool nextWorkExecuted;
  bool failingDispatchExecuted;

  bool operator==(const SynchronousDispatchFailureTrace&) const = default;

  friend void PrintTo(
      const SynchronousDispatchFailureTrace& trace, std::ostream* out) {
    *out << "{exceptionEscaped: " << trace.exceptionEscaped
         << ", nextWorkExecuted: " << trace.nextWorkExecuted
         << ", failingDispatchExecuted: " << trace.failingDispatchExecuted
         << '}';
  }
};

struct ResponseTrace {
  PyObject* handlerFunction{nullptr};
  std::optional<int> responseBeforeControlDrain;
  std::optional<int> responseBeforeHandlerCompletion;
  std::optional<int> responseBeforeFinalControlDrain;
  std::optional<int> responseAfterFinalControlDrain;

  bool operator==(const ResponseTrace&) const = default;

  friend void PrintTo(const ResponseTrace& trace, std::ostream* out) {
    auto printResponse = [out](const std::optional<int>& response) {
      if (response.has_value()) {
        *out << *response;
      } else {
        *out << "none";
      }
    };
    *out << "{handlerFunction: " << trace.handlerFunction
         << ", responseBeforeControlDrain: ";
    printResponse(trace.responseBeforeControlDrain);
    *out << ", responseBeforeHandlerCompletion: ";
    printResponse(trace.responseBeforeHandlerCompletion);
    *out << ", responseBeforeFinalControlDrain: ";
    printResponse(trace.responseBeforeFinalControlDrain);
    *out << ", responseAfterFinalControlDrain: ";
    printResponse(trace.responseAfterFinalControlDrain);
    *out << '}';
  }
};

TEST(
    ExecutionSystemTest,
    ExecuteSuppliesSelectedHandlerFunctionOnControlExecutor) {
  // GIVEN
  folly::ManualExecutor controlExecutor;
  ExecutionSystem executionSystem(&controlExecutor);
  auto [handlerPromise, handlerResultFuture] =
      folly::makePromiseContract<int>();
  ResponseCallback responseCallback;
  auto* const selectedHandlerFunction = reinterpret_cast<PyObject*>(42);
  const ResponseTrace expected{
      .handlerFunction = selectedHandlerFunction,
      .responseBeforeControlDrain = std::nullopt,
      .responseBeforeHandlerCompletion = std::nullopt,
      .responseBeforeFinalControlDrain = std::nullopt,
      .responseAfterFinalControlDrain = 42,
  };

  // WHEN
  PyObject* handlerFunction = nullptr;
  executionSystem.execute(
      selectedHandlerFunction,
      RequestDispatch([pendingHandlerFuture = std::move(handlerResultFuture),
                       &handlerFunction,
                       &responseCallback](PyObject* selectedFunction) mutable {
        handlerFunction = selectedFunction;
        return std::move(pendingHandlerFuture)
            .defer([&responseCallback](auto&& result) {
              responseCallback.complete(std::move(result));
            });
      }));
  const auto responseBeforeControlDrain = responseCallback.response();
  controlExecutor.drain();
  const auto responseBeforeHandlerCompletion = responseCallback.response();
  handlerPromise.setValue(42);
  const auto responseBeforeFinalControlDrain = responseCallback.response();
  controlExecutor.drain();
  const ResponseTrace actual{
      .handlerFunction = handlerFunction,
      .responseBeforeControlDrain = responseBeforeControlDrain,
      .responseBeforeHandlerCompletion = responseBeforeHandlerCompletion,
      .responseBeforeFinalControlDrain = responseBeforeFinalControlDrain,
      .responseAfterFinalControlDrain = responseCallback.response(),
  };

  // THEN
  EXPECT_EQ(expected, actual);
}

TEST(ExecutionSystemTest, AcceptedWorkCompletesAfterShutdown) {
  // GIVEN
  folly::ManualExecutor controlExecutor;
  ExecutionSystem executionSystem(&controlExecutor);
  auto [handlerPromise, handlerResultFuture] =
      folly::makePromiseContract<int>();
  ResponseCallback responseCallback;
  auto* const dummySelectedHandlerFunction = reinterpret_cast<PyObject*>(42);
  const std::optional<int> expected = 42;

  // WHEN
  executionSystem.execute(
      dummySelectedHandlerFunction,
      [pendingHandlerFuture = std::move(handlerResultFuture),
       &responseCallback](PyObject*) mutable {
        return std::move(pendingHandlerFuture)
            .defer([&responseCallback](auto&& result) {
              responseCallback.complete(std::move(result));
            });
      });
  executionSystem.shutdown();
  // The first drain starts the request. The handler result queues the callback,
  // and the second drain runs it.
  controlExecutor.drain();
  handlerPromise.setValue(42);
  controlExecutor.drain();
  const auto actual = responseCallback.response();

  // THEN
  EXPECT_EQ(expected, actual);
}

TEST(
    ExecutionSystemTest,
    AsynchronousDispatchFailureDoesNotEscapeControlExecutor) {
  // GIVEN
  folly::ManualExecutor controlExecutor;
  ExecutionSystem executionSystem(&controlExecutor);
  auto* const dummySelectedHandlerFunction = reinterpret_cast<PyObject*>(42);
  const DispatchFailureTrace expected{
      .exceptionEscaped = false,
      .nextWorkExecuted = true,
  };

  // WHEN
  executionSystem.execute(dummySelectedHandlerFunction, [](PyObject*) {
    return folly::makeSemiFuture<folly::Unit>(
        folly::make_exception_wrapper<std::runtime_error>(
            "request dispatch test failure"));
  });
  bool nextWorkExecuted = false;
  executionSystem.execute(
      dummySelectedHandlerFunction, [&nextWorkExecuted](PyObject*) {
        nextWorkExecuted = true;
        return folly::makeSemiFuture();
      });
  bool exceptionEscaped = false;
  try {
    controlExecutor.drain();
  } catch (const std::runtime_error&) {
    exceptionEscaped = true;
  }
  const DispatchFailureTrace actual{
      .exceptionEscaped = exceptionEscaped,
      .nextWorkExecuted = nextWorkExecuted,
  };

  // THEN
  EXPECT_EQ(expected, actual);
}

TEST(ExecutionSystemTest, SynchronousDispatchFailureDoesNotEscape) {
  // GIVEN
  folly::ManualExecutor controlExecutor;
  ExecutionSystem executionSystem(&controlExecutor);
  auto* const dummySelectedHandlerFunction = reinterpret_cast<PyObject*>(42);
  bool shouldThrow = true;
  const SynchronousDispatchFailureTrace expected{
      .exceptionEscaped = false,
      .nextWorkExecuted = true,
      .failingDispatchExecuted = true,
  };

  // WHEN
  executionSystem.execute(
      dummySelectedHandlerFunction,
      [&shouldThrow](PyObject*) -> folly::SemiFuture<folly::Unit> {
        if (std::exchange(shouldThrow, false)) {
          throw std::runtime_error("synchronous request dispatch test failure");
        }
        return folly::makeSemiFuture();
      });
  bool nextWorkExecuted = false;
  executionSystem.execute(
      dummySelectedHandlerFunction, [&nextWorkExecuted](PyObject*) {
        nextWorkExecuted = true;
        return folly::makeSemiFuture();
      });
  bool exceptionEscaped = false;
  try {
    controlExecutor.drain();
  } catch (const std::runtime_error&) {
    exceptionEscaped = true;
  }
  const SynchronousDispatchFailureTrace actual{
      .exceptionEscaped = exceptionEscaped,
      .nextWorkExecuted = nextWorkExecuted,
      .failingDispatchExecuted = !shouldThrow,
  };

  // THEN
  EXPECT_EQ(expected, actual);
}

} // namespace
} // namespace apache::thrift::python::execution
