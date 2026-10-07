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

#include <thrift/lib/python/server/execution/WorkScheduler.h>

#include <folly/executors/InlineExecutor.h>
#include <folly/executors/SerialExecutor.h>
#include <folly/synchronization/Baton.h>

namespace apache::thrift::python::execution {

WorkerCapacityLease::~WorkerCapacityLease() {
  workScheduler_->makeWorkerCapacityAvailable(std::move(worker_));
}

WorkScheduler::WorkScheduler()
    : scheduler_(
          folly::SerialExecutor::create(
              folly::Executor::getKeepAliveToken(
                  folly::InlineExecutor::instance()))) {}

WorkerHandle WorkScheduler::registerWorker(
    folly::Executor::KeepAlive<> executor) {
  WorkerHandle worker(std::make_shared<const WorkerHandle::Identity>());
  workers_.emplace(worker, std::move(executor));
  return worker;
}

void WorkScheduler::unregisterWorker(WorkerHandle worker) {
  scheduler_->add([self = folly::Executor::getKeepAliveToken(*this),
                   worker = std::move(worker)] {
    self->workers_.erase(worker);
    std::erase(self->workersWithAvailableCapacity_, worker);
  });
}

void WorkScheduler::close() noexcept {
  folly::Baton<> closed;
  scheduler_->add([self = folly::Executor::getKeepAliveToken(*this), &closed] {
    self->workersWithAvailableCapacity_.clear();
    self->workers_.clear();
    closed.post();
  });
  closed.wait();
}

void WorkScheduler::makeWorkerCapacityAvailable(WorkerHandle worker) {
  scheduler_->add([self = folly::Executor::getKeepAliveToken(*this),
                   worker = std::move(worker)]() mutable {
    if (!self->workers_.contains(worker)) {
      return;
    }
    self->workersWithAvailableCapacity_.push_back(std::move(worker));
    self->dispatchNextAvailableWorkItem();
  });
}

void WorkScheduler::addAsynchronousWorkWithPriority(
    AsynchronousWork workItem, int8_t priority) {
  enqueue(std::move(workItem), workPriorityFor(priority));
}

void WorkScheduler::add(folly::Func workItem) {
  addWithPriority(std::move(workItem), folly::Executor::MID_PRI);
}

void WorkScheduler::addWithPriority(folly::Func workItem, int8_t priority) {
  addAsynchronousWorkWithPriority(
      [workItem = std::move(workItem)](
          WorkerHandle, std::shared_ptr<WorkerCapacityLease> lease) mutable {
        auto executor = lease->executor_;
        executor->add([lease = std::move(lease)] {});
        workItem();
      },
      priority);
}

WorkScheduler::WorkPriority WorkScheduler::workPriorityFor(int8_t priority) {
  return priority == folly::Executor::LO_PRI ? WorkPriority::Low
                                             : WorkPriority::Normal;
}

std::deque<WorkScheduler::AsynchronousWork>& WorkScheduler::selectQueue(
    WorkPriority priority) {
  return priority == WorkPriority::Normal ? normalPriorityWork_
                                          : lowPriorityWork_;
}

void WorkScheduler::enqueue(AsynchronousWork workItem, WorkPriority priority) {
  scheduler_->add([self = folly::Executor::getKeepAliveToken(*this),
                   workItem = std::move(workItem),
                   priority]() mutable {
    self->selectQueue(priority).push_back(std::move(workItem));
    self->dispatchNextAvailableWorkItem();
  });
}

bool WorkScheduler::dispatchOne(std::deque<AsynchronousWork>& queue) {
  if (queue.empty() || workersWithAvailableCapacity_.empty()) {
    return false;
  }

  auto worker = std::move(workersWithAvailableCapacity_.front());
  workersWithAvailableCapacity_.pop_front();
  auto executor = workers_.at(worker);
  auto lease = std::shared_ptr<WorkerCapacityLease>(new WorkerCapacityLease(
      executor, folly::Executor::getKeepAliveToken(*this), this, worker));
  auto workItem = std::move(queue.front());
  queue.pop_front();
  executor->add([workItem = std::move(workItem),
                 worker,
                 lease = std::move(lease)]() mutable {
    std::move(workItem)(std::move(worker), std::move(lease));
  });
  return true;
}

void WorkScheduler::dispatchNextAvailableWorkItem() {
  if (!dispatchOne(normalPriorityWork_)) {
    dispatchOne(lowPriorityWork_);
  }
}

} // namespace apache::thrift::python::execution
