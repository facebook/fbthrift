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
#include <unordered_map>
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
  friend std::shared_ptr<ExecutionSystem> makeWorkerCapableExecutionSystem(
      folly::Executor* controlExecutor,
      LaunchControlRequest launchControlRequest,
      std::map<std::string, PyObject*> controlMethods,
      std::shared_ptr<WorkScheduler> workScheduler,
      LaunchWorkerRequest launchWorkerRequest,
      const std::set<std::string>& workerCapableMethodNames);
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

  // Binds a worker handle to its method table: the handler functions that
  // requests on that worker call. Call once per worker, before its capacity
  // becomes available. Workers bind one at a time during factory context
  // entry, before any request can arrive, so binding needs no lock.
  void bindWorkerMethods(
      WorkerHandle worker, std::map<std::string, PyObject*> methods);

 private:
  // Methods in workerCapableMethodNames run on workers through workScheduler
  // and launchWorkerRequest; every other method runs on control.
  ExecutionSystem(
      folly::Executor* controlExecutor,
      LaunchControlRequest launchControlRequest,
      std::map<std::string, PyObject*> controlMethods,
      std::shared_ptr<WorkScheduler> workScheduler,
      LaunchWorkerRequest launchWorkerRequest,
      const std::set<std::string>& workerCapableMethodNames)
      : controlExecutor_(folly::Executor::getKeepAliveToken(controlExecutor)),
        launchControlRequest_(std::move(launchControlRequest).asSharedProxy()),
        controlMethods_(
            std::make_shared<const std::map<std::string, PyObject*>>(
                std::move(controlMethods))),
        workScheduler_(std::move(workScheduler)),
        launchWorkerRequest_(std::move(launchWorkerRequest).asSharedProxy()) {
    for (const auto& [method, _] : *controlMethods_) {
      executeOn_.emplace(
          method,
          workerCapableMethodNames.contains(method)
              ? &ExecutionSystem::executeOnWorker
              : &ExecutionSystem::executeOnControl);
    }
  }

  using ExecuteOn =
      void (ExecutionSystem::*)(std::string, RequestDispatch) const;

  void executeOnControl(
      std::string method, RequestDispatch requestDispatch) const;

  // Runs one request on an assigned worker. Call only when a scheduler is
  // present.
  void executeOnWorker(
      std::string method, RequestDispatch requestDispatch) const;

  // Returns the handler function that a worker bound for a method. Call only
  // for bound workers while active. Lookups only read the tables, so they
  // need no lock.
  PyObject* getWorkerHandlerFunction(
      const WorkerHandle& worker, const std::string& method) const;

  folly::Executor::KeepAlive<> controlExecutor_;
  LaunchControlRequest::SharedProxy launchControlRequest_{nullptr};
  // The Python factory's function table owns the callable references.
  std::shared_ptr<const std::map<std::string, PyObject*>> controlMethods_;
  // Each method's destination, fixed at construction from its route.
  std::map<std::string, ExecuteOn> executeOn_;

  // Per-worker method tables borrow their callables from the worker
  // handlers, exactly like the shared table above. They need no lock because
  // of their order guarantees: workers bind one at a time during factory
  // context entry, before any request can arrive; requests only read the
  // tables; and shutdown clears them only after worker teardown.
  std::unordered_map<
      WorkerHandle,
      std::shared_ptr<const std::map<std::string, PyObject*>>,
      WorkerHandle::Hash>
      workerMethods_;

  // Null without workers. Worker dispatch needs both set.
  std::shared_ptr<WorkScheduler> workScheduler_;
  LaunchWorkerRequest::SharedProxy launchWorkerRequest_{nullptr};
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

// Builds an execution system that runs the methods in workerCapableMethodNames
// on workers and every other method on control.
inline std::shared_ptr<ExecutionSystem> makeWorkerCapableExecutionSystem(
    folly::Executor* controlExecutor,
    LaunchControlRequest launchControlRequest,
    std::map<std::string, PyObject*> controlMethods,
    std::shared_ptr<WorkScheduler> workScheduler,
    LaunchWorkerRequest launchWorkerRequest,
    const std::set<std::string>& workerCapableMethodNames) {
  return std::shared_ptr<ExecutionSystem>(new ExecutionSystem(
      controlExecutor,
      std::move(launchControlRequest),
      std::move(controlMethods),
      std::move(workScheduler),
      std::move(launchWorkerRequest),
      workerCapableMethodNames));
}

// Builds an execution system that holds no executor, scheduler, or handler
// references and rejects every request. A factory keeps one after it releases
// its execution resources.
inline std::shared_ptr<ExecutionSystem> makeReleasedExecutionSystem() {
  return makeControlOnlyExecutionSystem(nullptr, LaunchControlRequest{}, {});
}

} // namespace apache::thrift::python::execution
