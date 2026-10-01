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
#include <utility>
#include <vector>

#include <folly/Executor.h>
#include <thrift/lib/python/server/PythonAsyncProcessorFactory.h>

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

inline std::shared_ptr<PythonAsyncProcessorFactory> createHostedTestFactory(
    PyObject* pythonServer,
    std::vector<PyObject*> lifecycleFuncs,
    ForwardingKeepAliveTrackingExecutor& controlExecutor) {
  return PythonAsyncProcessorFactory::create(
      pythonServer,
      {},
      std::move(lifecycleFuncs),
      folly::getKeepAliveToken(controlExecutor),
      "test.Service");
}

} // namespace apache::thrift::python::test
