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

#include <array>
#include <cstddef>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

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

struct RequestEvent {
  enum class Kind {
    BeforeDispatchDrain,
    Dispatch,
    RequestStart,
    AfterDispatchDrain,
    HandlerCompletion,
    BeforeCompletionDrain,
    Response,
    AfterCompletionDrain,
  };

  Kind kind;
  std::variant<std::monostate, PyObject*, int> subject;

  bool operator==(const RequestEvent&) const = default;

  friend void PrintTo(const RequestEvent& event, std::ostream* out) {
    static constexpr std::array names{
        "before dispatch drain",
        "dispatch",
        "start request",
        "after dispatch drain",
        "complete handler",
        "before completion drain",
        "respond",
        "after completion drain"};
    *out << names[static_cast<std::size_t>(event.kind)] << '('
         << ::testing::PrintToString(event.subject) << ')';
  }
};

TEST(ExecutionSystemTest, ExecuteStartsSelectedRequestOnExecutor) {
  // GIVEN
  std::vector<RequestEvent> events;
  folly::ManualExecutor selectedExecutor;
  ExecutionSystem executionSystem(
      &selectedExecutor, [&events](PyObject* coroutineFactory) {
        events.push_back({RequestEvent::Kind::RequestStart, coroutineFactory});
        return 0;
      });
  auto [handlerPromise, handlerResultFuture] =
      folly::makePromiseContract<int>();
  auto* const selectedHandlerFunction = reinterpret_cast<PyObject*>(42);
  auto* const coroutineFactory = reinterpret_cast<PyObject*>(84);
  const std::vector<RequestEvent> expected{
      {RequestEvent::Kind::BeforeDispatchDrain, std::monostate{}},
      {RequestEvent::Kind::Dispatch, selectedHandlerFunction},
      {RequestEvent::Kind::RequestStart, coroutineFactory},
      {RequestEvent::Kind::AfterDispatchDrain, std::monostate{}},
      {RequestEvent::Kind::HandlerCompletion, std::monostate{}},
      {RequestEvent::Kind::BeforeCompletionDrain, std::monostate{}},
      {RequestEvent::Kind::Response, 42},
      {RequestEvent::Kind::AfterCompletionDrain, std::monostate{}},
  };

  // WHEN
  executionSystem.execute(
      selectedHandlerFunction,
      RequestDispatch([pendingHandlerFuture = std::move(handlerResultFuture),
                       &events,
                       coroutineFactory = coroutineFactory](
                          PyObject* selectedFunction,
                          RequestExecution requestExecution) mutable {
        events.push_back({RequestEvent::Kind::Dispatch, selectedFunction});
        std::move(requestExecution)(coroutineFactory);
        return std::move(pendingHandlerFuture).defer([&events](auto&& result) {
          events.push_back(
              {RequestEvent::Kind::Response, std::move(result).value()});
        });
      }));
  events.push_back({RequestEvent::Kind::BeforeDispatchDrain, std::monostate{}});
  selectedExecutor.drain();
  events.push_back({RequestEvent::Kind::AfterDispatchDrain, std::monostate{}});
  events.push_back({RequestEvent::Kind::HandlerCompletion, std::monostate{}});
  handlerPromise.setValue(42);
  events.push_back(
      {RequestEvent::Kind::BeforeCompletionDrain, std::monostate{}});
  selectedExecutor.drain();
  events.push_back(
      {RequestEvent::Kind::AfterCompletionDrain, std::monostate{}});

  // THEN
  EXPECT_EQ(expected, events);
}

TEST(ExecutionSystemTest, AcceptedWorkCompletesAfterShutdown) {
  // GIVEN
  folly::ManualExecutor controlExecutor;
  ExecutionSystem executionSystem(
      &controlExecutor, [](PyObject*) { return 0; });
  auto [handlerPromise, handlerResultFuture] =
      folly::makePromiseContract<int>();
  ResponseCallback responseCallback;
  auto* const dummySelectedHandlerFunction = reinterpret_cast<PyObject*>(42);
  const std::optional<int> expected = 42;

  // WHEN
  executionSystem.execute(
      dummySelectedHandlerFunction,
      RequestDispatch([pendingHandlerFuture = std::move(handlerResultFuture),
                       &responseCallback](PyObject*, RequestExecution) mutable {
        return std::move(pendingHandlerFuture)
            .defer([&responseCallback](auto&& result) {
              responseCallback.complete(std::move(result));
            });
      }));
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
  ExecutionSystem executionSystem(
      &controlExecutor, [](PyObject*) { return 0; });
  auto* const dummySelectedHandlerFunction = reinterpret_cast<PyObject*>(42);
  const DispatchFailureTrace expected{
      .exceptionEscaped = false,
      .nextWorkExecuted = true,
  };

  // WHEN
  executionSystem.execute(
      dummySelectedHandlerFunction,
      RequestDispatch([](PyObject*, RequestExecution) {
        return folly::makeSemiFuture<folly::Unit>(
            folly::make_exception_wrapper<std::runtime_error>(
                "request dispatch test failure"));
      }));
  bool nextWorkExecuted = false;
  executionSystem.execute(
      dummySelectedHandlerFunction,
      RequestDispatch([&nextWorkExecuted](PyObject*, RequestExecution) {
        nextWorkExecuted = true;
        return folly::makeSemiFuture();
      }));
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
  ExecutionSystem executionSystem(
      &controlExecutor, [](PyObject*) { return 0; });
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
      RequestDispatch(
          [&shouldThrow](
              PyObject*, RequestExecution) -> folly::SemiFuture<folly::Unit> {
            if (std::exchange(shouldThrow, false)) {
              throw std::runtime_error(
                  "synchronous request dispatch test failure");
            }
            return folly::makeSemiFuture();
          }));
  bool nextWorkExecuted = false;
  executionSystem.execute(
      dummySelectedHandlerFunction,
      RequestDispatch([&nextWorkExecuted](PyObject*, RequestExecution) {
        nextWorkExecuted = true;
        return folly::makeSemiFuture();
      }));
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
