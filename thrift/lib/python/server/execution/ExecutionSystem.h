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

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include <folly/Executor.h>
#include <thrift/lib/python/server/execution/RequestDispatch.h>
#include <thrift/lib/python/server/execution/WorkScheduler.h>

namespace apache::thrift::python::execution {

using LaunchControlRequest = folly::Function<int(_object*) const>;
using LaunchWorkerRequest =
    folly::Function<int(_object*, std::shared_ptr<WorkerCapacityLease>)>;

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
  friend std::shared_ptr<ExecutionSystem> makeControlOnlyExecutionSystem(
      folly::Executor* controlExecutor,
      LaunchControlRequest launchControlRequest,
      std::map<std::string, PyObject*> controlMethods);
  friend std::shared_ptr<ExecutionSystem> makeReleasedExecutionSystem();

  ~ExecutionSystem() = default;
  ExecutionSystem(const ExecutionSystem&) = delete;
  ExecutionSystem& operator=(const ExecutionSystem&) = delete;
  ExecutionSystem(ExecutionSystem&&) = delete;
  ExecutionSystem& operator=(ExecutionSystem&&) = delete;

  // Idempotent. Call only after native request drain, and never overlap this
  // call with request dispatch.
  void shutdown() noexcept;

  // Supports concurrent calls while active. Call only before shutdown starts.
  void execute(std::string method, RequestDispatch requestDispatch) const;

 private:
  // Every method runs on control; the scheduler, worker launcher, and worker
  // methods do not affect dispatch.
  ExecutionSystem(
      folly::Executor* controlExecutor,
      LaunchControlRequest launchControlRequest,
      std::map<std::string, PyObject*> controlMethods,
      [[maybe_unused]] std::shared_ptr<WorkScheduler> workScheduler,
      [[maybe_unused]] LaunchWorkerRequest launchWorkerRequest,
      [[maybe_unused]] const std::set<std::string>& workerCapableMethodNames)
      : controlExecutor_(folly::Executor::getKeepAliveToken(controlExecutor)),
        launchControlRequest_(std::move(launchControlRequest).asSharedProxy()),
        controlMethods_(
            std::make_shared<const std::map<std::string, PyObject*>>(
                std::move(controlMethods))) {
    for (const auto& [method, _] : *controlMethods_) {
      executeOn_.emplace(method, &ExecutionSystem::executeOnControl);
    }
  }

  using ExecuteOn =
      void (ExecutionSystem::*)(std::string, RequestDispatch) const;

  void executeOnControl(
      std::string method, RequestDispatch requestDispatch) const;

  folly::Executor::KeepAlive<> controlExecutor_;
  LaunchControlRequest::SharedProxy launchControlRequest_{nullptr};
  // The Python factory's function table owns the callable references.
  std::shared_ptr<const std::map<std::string, PyObject*>> controlMethods_;
  // Every known method starts on control. A later step flips entries for
  // methods that may run elsewhere.
  std::map<std::string, ExecuteOn> executeOn_;
};

// Builds an execution system that dispatches every method on control.
inline std::shared_ptr<ExecutionSystem> makeControlOnlyExecutionSystem(
    folly::Executor* controlExecutor,
    LaunchControlRequest launchControlRequest,
    std::map<std::string, PyObject*> controlMethods) {
  return std::shared_ptr<ExecutionSystem>(new ExecutionSystem(
      controlExecutor,
      std::move(launchControlRequest),
      std::move(controlMethods),
      nullptr,
      LaunchWorkerRequest{},
      {}));
}

// Builds an execution system that holds no executor or handler references and
// rejects every request. A factory keeps one after it releases its execution
// resources.
inline std::shared_ptr<ExecutionSystem> makeReleasedExecutionSystem() {
  return makeControlOnlyExecutionSystem(nullptr, LaunchControlRequest{}, {});
}

} // namespace apache::thrift::python::execution
