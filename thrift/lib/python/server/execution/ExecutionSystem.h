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

#include <cassert>
#include <memory>
#include <utility>

#include <folly/Executor.h>
#include <thrift/lib/python/server/execution/RequestExecution.h>

namespace apache::thrift::python::execution {

using StartControlRequest = folly::Function<int(_object*) const>;

/**
 * Owns the resources that dispatch Python requests and releases those
 * resources as one lifecycle unit.
 *
 * A processor factory shares one ExecutionSystem across its processors.
 * Multiple request dispatches may run concurrently while the system remains
 * active. The caller serializes lifecycle transitions: it establishes all
 * execution resources before the server accepts requests, starts shutdown only
 * after native request drain, and never overlaps shutdown with dispatch or
 * resource changes. Repeated shutdown calls are safe.
 *
 * These rules ensure that request dispatch never uses released execution
 * resources.
 */
class ExecutionSystem final {
 public:
  static std::shared_ptr<ExecutionSystem> createWithEmptyExecutor(
      [[maybe_unused]] const folly::Executor::KeepAlive<>& controlExecutor) {
    assert(!controlExecutor);
    return std::shared_ptr<ExecutionSystem>(new ExecutionSystem());
  }

  ExecutionSystem(
      folly::Executor* controlExecutor, StartControlRequest startRequest)
      : controlExecutor_(folly::Executor::getKeepAliveToken(controlExecutor)),
        startRequest_(std::move(startRequest).asSharedProxy()) {}

  ~ExecutionSystem() = default;
  ExecutionSystem(const ExecutionSystem&) = delete;
  ExecutionSystem& operator=(const ExecutionSystem&) = delete;
  ExecutionSystem(ExecutionSystem&&) = delete;
  ExecutionSystem& operator=(ExecutionSystem&&) = delete;

  // Idempotent. Call only after native request drain, and never overlap this
  // call with request dispatch.
  void shutdown() noexcept;

  // Supports concurrent calls while active. Call only before shutdown starts.
  void execute(
      PyObject* selectedHandlerFunction, RequestDispatch requestDispatch) const;

 private:
  ExecutionSystem() = default;

  folly::Executor::KeepAlive<> controlExecutor_;
  StartControlRequest::SharedProxy startRequest_{nullptr};
};

} // namespace apache::thrift::python::execution
