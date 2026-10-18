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

#include <thrift/lib/python/server/execution/WorkerExecutorFacade.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include <folly/executors/ManualExecutor.h>
#include <folly/portability/GTest.h>

namespace apache::thrift::python::execution {
namespace {

TEST(WorkerExecutorFacadeTest, PlainWorkRunsOnControl) {
  // GIVEN
  const std::size_t expectedRunOnControl = 1;
  folly::ManualExecutor control;
  auto* facade =
      WorkerExecutorFacade::create(std::make_shared<WorkScheduler>(), &control);
  bool ran = false;

  // WHEN
  facade->add([&] { ran = true; });
  const auto actualRunOnControl = control.drain();
  facade->releaseCreatorReference();

  // THEN
  EXPECT_EQ(expectedRunOnControl, actualRunOnControl);
  EXPECT_TRUE(ran);
}

TEST(WorkerExecutorFacadeTest, PrioritizedWorkRunsOnWorker) {
  // GIVEN
  const std::size_t expectedRunOnControl = 0;
  folly::ManualExecutor control;
  folly::ManualExecutor workerExecutor;
  auto scheduler = std::make_shared<WorkScheduler>();
  const auto worker = scheduler->registerWorker(
      folly::Executor::getKeepAliveToken(workerExecutor));
  auto* facade = WorkerExecutorFacade::create(scheduler, &control);
  bool ran = false;

  // WHEN
  facade->addWithPriority([&] { ran = true; }, folly::Executor::LO_PRI);
  scheduler->makeWorkerCapacityAvailable(worker);
  const auto actualRunOnControl = control.drain();
  workerExecutor.drain();
  facade->releaseCreatorReference();

  // THEN
  EXPECT_EQ(expectedRunOnControl, actualRunOnControl);
  EXPECT_TRUE(ran);
}

TEST(WorkerExecutorFacadeTest, AdvertisesTwoPriorities) {
  // GIVEN
  const std::uint8_t expectedNumberOfPriorities = 2;
  folly::ManualExecutor control;
  auto* facade =
      WorkerExecutorFacade::create(std::make_shared<WorkScheduler>(), &control);

  // WHEN
  const auto actualNumberOfPriorities = facade->getNumPriorities();
  facade->releaseCreatorReference();

  // THEN
  // The concurrency controller submits request intake through addWithPriority
  // only when its executor advertises more than one priority. The work
  // scheduler has exactly two, normal and low; changing that changes the
  // contract.
  EXPECT_EQ(expectedNumberOfPriorities, actualNumberOfPriorities);
}

TEST(WorkerExecutorFacadeTest, KeepAliveHolderKeepsFacadeAlive) {
  // GIVEN
  folly::ManualExecutor control;
  auto scheduler = std::make_shared<WorkScheduler>();
  // The facade becomes the scheduler's only owner, so the scheduler lives
  // exactly as long as the facade.
  std::weak_ptr<WorkScheduler> schedulerObserver = scheduler;
  auto* facade = WorkerExecutorFacade::create(std::move(scheduler), &control);
  auto keepAlive = folly::getKeepAliveToken(facade);
  bool ran = false;

  // WHEN
  // The creator lets go before the holder, as a factory can before its server.
  facade->releaseCreatorReference();
  const bool actualAliveWhileHeld = !schedulerObserver.expired();
  keepAlive->add([&] { ran = true; });
  control.drain();
  keepAlive.reset();
  const bool actualAliveAfterLastRelease = !schedulerObserver.expired();

  // THEN
  EXPECT_TRUE(actualAliveWhileHeld);
  EXPECT_TRUE(ran);
  EXPECT_FALSE(actualAliveAfterLastRelease);
}

} // namespace
} // namespace apache::thrift::python::execution
