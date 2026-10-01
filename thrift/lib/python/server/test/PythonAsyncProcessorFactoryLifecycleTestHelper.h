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

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <folly/Executor.h>
#include <thrift/lib/python/server/PythonAsyncProcessorFactory.h>
#include <thrift/lib/python/server/execution/ExecutionSystem.h>

namespace apache::thrift::python::test {

class ForwardingKeepAliveTrackingExecutor final : public folly::Executor {
 public:
  explicit ForwardingKeepAliveTrackingExecutor(folly::Executor* delegate)
      : delegate_(delegate) {}

  void add(folly::Func func) override { delegate_->add(std::move(func)); }

  bool keepAliveAcquire() noexcept override {
    keepAliveCount_.fetch_add(1, std::memory_order_relaxed);
    return true;
  }

  void keepAliveRelease() noexcept override {
    keepAliveCount_.fetch_sub(1, std::memory_order_relaxed);
  }

  std::size_t keepAliveCount() const noexcept {
    return keepAliveCount_.load(std::memory_order_relaxed);
  }

 private:
  folly::Executor* delegate_;
  std::atomic<std::size_t> keepAliveCount_{0};
};

inline bool isFreeThreadedBuild() noexcept {
#ifdef Py_GIL_DISABLED
  return true;
#else
  return false;
#endif
}

inline std::size_t createProcessorsWhileLegacyStopRuns(
    ForwardingKeepAliveTrackingExecutor& controlExecutor,
    const std::size_t iterationCount) {
  auto executionSystem = std::make_shared<execution::ExecutionSystem>(
      &controlExecutor, [](PyObject*) { return 0; });
  auto factory = PythonAsyncProcessorFactory::create(
      nullptr,
      {},
      {},
      std::move(executionSystem),
      folly::getKeepAliveToken(controlExecutor),
      "test.Service");
  std::atomic<std::size_t> readyCount{0};
  std::size_t processorCount = 0;
  auto waitForPeer = [&] {
    readyCount.fetch_add(1, std::memory_order_release);
    while (readyCount.load(std::memory_order_acquire) != 2) {
      std::this_thread::yield();
    }
  };
  std::thread processorThread([&] {
    waitForPeer();
    for (std::size_t i = 0; i < iterationCount; ++i) {
      auto processor = factory->getProcessor();
      ++processorCount;
    }
  });
  std::thread stopThread([&] {
    waitForPeer();
    for (std::size_t i = 0; i < iterationCount; ++i) {
      factory->semifuture_onStopRequested().get();
    }
  });
  processorThread.join();
  stopThread.join();
  return processorCount;
}

inline std::shared_ptr<PythonAsyncProcessorFactory> createHostedTestFactory(
    PyObject* pythonServer,
    FunctionMapType functions,
    std::vector<PyObject*> lifecycleFuncs,
    ForwardingKeepAliveTrackingExecutor& controlExecutor,
    execution::StartControlRequest startRequest) {
  auto executionSystem = std::make_shared<execution::ExecutionSystem>(
      &controlExecutor, std::move(startRequest));
  return PythonAsyncProcessorFactory::create(
      pythonServer,
      std::move(functions),
      std::move(lifecycleFuncs),
      std::move(executionSystem),
      folly::getKeepAliveToken(controlExecutor),
      "test.Service");
}

} // namespace apache::thrift::python::test
