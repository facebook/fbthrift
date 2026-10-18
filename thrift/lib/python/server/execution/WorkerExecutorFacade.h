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

#include <folly/Executor.h>
#include <thrift/lib/python/server/execution/WorkScheduler.h>

namespace apache::thrift::python::execution {

/**
 * The executor that ThriftServer receives in worker mode.
 *
 * Request intake goes to the work scheduler, so a request leaves the server's
 * queue only when a worker can take it and the server's queue timeout still
 * applies. ThriftServer's concurrency controller submits intake through
 * addWithPriority whenever its executor advertises more than one priority,
 * which this facade does. Everything else the server runs on its executor,
 * such as lifecycle hooks and follow-up work, arrives through add and goes to
 * control.
 *
 * The facade counts its creator's reference and every keep-alive holder, as the
 * asyncio executor used for control does. That executor deletes itself in
 * drop() after running callbacks until every holder lets go; the facade runs no
 * callbacks of its own, so it deletes itself on the last release instead. A
 * server that outlives the creating factory therefore never uses a freed
 * executor.
 */
class WorkerExecutorFacade final : public folly::Executor {
 public:
  static WorkerExecutorFacade* create(
      std::shared_ptr<WorkScheduler> scheduler, folly::Executor* control) {
    return new WorkerExecutorFacade(std::move(scheduler), control);
  }

  void releaseCreatorReference() noexcept { keepAliveRelease(); }

  void add(folly::Func func) override { control_->add(std::move(func)); }

  void addWithPriority(folly::Func func, int8_t priority) override {
    scheduler_->addWithPriority(std::move(func), priority);
  }

  uint8_t getNumPriorities() const override {
    return scheduler_->getNumPriorities();
  }

 protected:
  bool keepAliveAcquire() noexcept override {
    references_.fetch_add(1, std::memory_order_relaxed);
    return true;
  }

  void keepAliveRelease() noexcept override {
    if (references_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      delete this;
    }
  }

 private:
  WorkerExecutorFacade(
      std::shared_ptr<WorkScheduler> scheduler, folly::Executor* control)
      : scheduler_(std::move(scheduler)),
        control_(folly::getKeepAliveToken(control)) {}

  ~WorkerExecutorFacade() override = default;

  std::atomic<std::size_t> references_{1};
  std::shared_ptr<WorkScheduler> scheduler_;
  folly::Executor::KeepAlive<> control_;
};

} // namespace apache::thrift::python::execution
