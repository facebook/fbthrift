/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <deque>
#include <iostream>
#include <chrono>
#include <memory>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/test/unit_test.hpp>

#include "thrift/lib/cpp/concurrency/Monitor.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp/concurrency/Util.h"
#include "thrift/lib/cpp/concurrency/Codel.h"

using namespace boost;
using namespace apache::thrift::concurrency;

// Loops until x==y for up to timeout ms.
// The end result is the same as of BOOST_{CHECK,REQUIRE}_EQUAL(x,y)
// (depending on OP) if x!=y after the timeout passes
#define X_EQUAL_SPECIFIC_TIMEOUT(OP, timeout, x, y) do { \
    using std::chrono::steady_clock; \
    using std::chrono::milliseconds;  \
    auto end = steady_clock::now() + milliseconds(timeout);  \
    while ((x) != (y) && steady_clock::now() < end)  \
      ; \
    BOOST_##OP##_EQUAL(x, y); \
  } while (0)

#define CHECK_EQUAL_SPECIFIC_TIMEOUT(timeout, x, y) \
  X_EQUAL_SPECIFIC_TIMEOUT(CHECK, timeout, x, y)
#define REQUIRE_EQUAL_SPECIFIC_TIMEOUT(timeout, x, y) \
  X_EQUAL_SPECIFIC_TIMEOUT(REQUIRE, timeout, x, y)

// A default timeout of 1 sec should be long enough for other threads to
// stabilize the values of x and y, and short enough to catch real errors
// when x is not going to be equal to y anytime soon
#define CHECK_EQUAL_TIMEOUT(x, y) CHECK_EQUAL_SPECIFIC_TIMEOUT(1000, x, y)
#define REQUIRE_EQUAL_TIMEOUT(x, y) REQUIRE_EQUAL_SPECIFIC_TIMEOUT(1000, x, y)

class LoadTask: public Runnable {
 public:
  LoadTask(Monitor* monitor, size_t* count, int64_t timeout)
    : monitor_(monitor),
      count_(count),
      timeout_(timeout),
      startTime_(0),
      endTime_(0) {}

  void run() {
    startTime_ = Util::currentTime();
    usleep(timeout_ * Util::US_PER_MS);
    endTime_ = Util::currentTime();

    {
      Synchronized s(*monitor_);

      (*count_)--;
      if (*count_ == 0) {
        monitor_->notify();
      }
    }
  }

  Monitor* monitor_;
  size_t* count_;
  int64_t timeout_;
  int64_t startTime_;
  int64_t endTime_;
};

/**
 * Dispatch count tasks, each of which blocks for timeout milliseconds then
 * completes. Verify that all tasks completed and that thread manager cleans
 * up properly on delete.
 */
void loadTest(size_t numTasks, int64_t timeout, size_t numWorkers) {
  Monitor monitor;
  size_t tasksLeft = numTasks;

  std::shared_ptr<ThreadManager> threadManager =
    ThreadManager::newSimpleThreadManager(numWorkers, 0, true);
  std::shared_ptr<PosixThreadFactory> threadFactory =
    std::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  threadManager->threadFactory(threadFactory);
  threadManager->start();

  std::set<std::shared_ptr<LoadTask>> tasks;
  for (size_t n = 0; n < numTasks; n++) {
    tasks.insert(std::shared_ptr<LoadTask>(
          new LoadTask(&monitor, &tasksLeft, timeout)));
  }

  int64_t startTime = Util::currentTime();
  for (const auto& task : tasks) {
    threadManager->add(task);
  }

  int64_t tasksStartedTime = Util::currentTime();

  {
    Synchronized s(monitor);
    while (tasksLeft > 0) {
      monitor.wait();
    }
  }
  int64_t endTime = Util::currentTime();

  int64_t firstTime = std::numeric_limits<int64_t>::max();
  int64_t lastTime = 0;
  double averageTime = 0;
  int64_t minTime = std::numeric_limits<int64_t>::max();
  int64_t maxTime = 0;

  for (const auto& task : tasks) {
    BOOST_CHECK_GT(task->startTime_, 0);
    BOOST_CHECK_GT(task->endTime_, 0);

    int64_t delta = task->endTime_ - task->startTime_;
    assert(delta > 0);

    firstTime = std::min(firstTime, task->startTime_);
    lastTime = std::max(lastTime, task->endTime_);
    minTime = std::min(minTime, delta);
    maxTime = std::max(maxTime, delta);

    averageTime += delta;
  }
  averageTime /= numTasks;

  BOOST_TEST_MESSAGE("first start: " << firstTime << "ms " <<
                     "last end: " << lastTime << "ms " <<
                     "min: " << minTime << "ms " <<
                     "max: " << maxTime << "ms " <<
                     "average: " << averageTime << "ms");

  double idealTime = ((numTasks + (numWorkers - 1)) / numWorkers) * timeout;
  double actualTime = endTime - startTime;
  double taskStartTime = tasksStartedTime - startTime;

  double overheadPct = (actualTime - idealTime) / idealTime;
  if (overheadPct < 0) {
    overheadPct*= -1.0;
  }

  BOOST_TEST_MESSAGE("ideal time: " << idealTime << "ms " <<
                     "actual time: "<< actualTime << "ms " <<
                     "task startup time: " << taskStartTime << "ms " <<
                     "overhead: " << overheadPct * 100.0 << "%");

  // Fail if the test took 10% more time than the ideal time
  BOOST_CHECK_LT(overheadPct, 0.10);

  // Get the task stats
  int64_t waitTimeUs;
  int64_t runTimeUs;
  threadManager->getStats(waitTimeUs, runTimeUs, numTasks * 2);

  // Compute the best possible average wait time
  int64_t fullIterations = numTasks / numWorkers;
  int64_t tasksOnLastIteration = numTasks % numWorkers;
  int64_t expectedTotalWaitTimeMs =
    numWorkers * ((fullIterations * (fullIterations - 1)) / 2) * timeout +
    tasksOnLastIteration * fullIterations * timeout;
  int64_t idealAvgWaitUs =
    (expectedTotalWaitTimeMs * Util::US_PER_MS) / numTasks;

  BOOST_TEST_MESSAGE("avg wait time: " << waitTimeUs << "us " <<
                     "avg run time: " << runTimeUs << "us " <<
                     "ideal wait time: " << idealAvgWaitUs << "us");

  // Verify that the average run time was more than the timeout, but not
  // more than 10% over.
  BOOST_CHECK_GE(runTimeUs, timeout * Util::US_PER_MS);
  BOOST_CHECK_LT(runTimeUs, timeout * Util::US_PER_MS * 1.10);
  // Verify that the average wait time was within 10% of the ideal wait time.
  // The calculation for ideal average wait time assumes all tasks were started
  // instantaneously, in reality, starting 1000 tasks takes some non-zero amount
  // of time, so later tasks will actually end up waiting for *less* than the
  // ideal wait time. Account for this by accepting an actual avg wait time that
  // is less than ideal avg wait time by up to the time it took to start all the
  // tasks.
  BOOST_CHECK_GE(waitTimeUs, idealAvgWaitUs - taskStartTime * Util::US_PER_MS);
  BOOST_CHECK_LT(waitTimeUs, idealAvgWaitUs * 1.10);
}

BOOST_AUTO_TEST_CASE(LoadTest) {
  size_t numTasks = 10000;
  int64_t timeout = 50;
  size_t numWorkers = 100;
  loadTest(numTasks, timeout, numWorkers);
}

class BlockTask: public Runnable {
 public:
  BlockTask(Monitor* monitor, Monitor* bmonitor, bool* blocked, size_t* count)
    : monitor_(monitor),
      bmonitor_(bmonitor),
      blocked_(blocked),
      count_(count),
      started_(false) {}

  void run() {
    started_ = true;
    {
      Synchronized s(*bmonitor_);
      while (*blocked_) {
        bmonitor_->wait();
      }
    }

    {
      Synchronized s(*monitor_);
      (*count_)--;
      if (*count_ == 0) {
        monitor_->notify();
      }
    }
  }

  Monitor* monitor_;
  Monitor* bmonitor_;
  bool* blocked_;
  size_t* count_;
  bool started_;
};

/**
 * Block test.
 * Create pendingTaskCountMax tasks.  Verify that we block adding the
 * pendingTaskCountMax + 1th task.  Verify that we unblock when a task
 * completes
 */
void blockTest(int64_t timeout, size_t numWorkers) {
  size_t pendingTaskMaxCount = numWorkers;

  std::shared_ptr<ThreadManager> threadManager =
    ThreadManager::newSimpleThreadManager(numWorkers, pendingTaskMaxCount);
  std::shared_ptr<PosixThreadFactory> threadFactory =
    std::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  threadManager->threadFactory(threadFactory);
  threadManager->start();

  Monitor monitor;
  Monitor bmonitor;

  // Add an initial set of tasks, 1 task per worker
  bool blocked1 = true;
  size_t tasksCount1 = numWorkers;
  std::set<std::shared_ptr<BlockTask>> tasks;
  for (size_t ix = 0; ix < numWorkers; ix++) {
    std::shared_ptr<BlockTask> task(new BlockTask(&monitor, &bmonitor,
                                             &blocked1, &tasksCount1));
    tasks.insert(task);
    threadManager->add(task);
  }
  REQUIRE_EQUAL_TIMEOUT(threadManager->totalTaskCount(), numWorkers);

  // Add a second set of tasks.
  // All of these will end up pending since the first set of tasks
  // are using up all of the worker threads and are still blocked
  bool blocked2 = true;
  size_t tasksCount2 = pendingTaskMaxCount;
  for (size_t ix = 0; ix < pendingTaskMaxCount; ix++) {
    std::shared_ptr<BlockTask> task(new BlockTask(&monitor, &bmonitor,
                                             &blocked2, &tasksCount2));
    tasks.insert(task);
    threadManager->add(task);
  }

  REQUIRE_EQUAL_TIMEOUT(threadManager->totalTaskCount(),
                      numWorkers + pendingTaskMaxCount);
  REQUIRE_EQUAL_TIMEOUT(threadManager->pendingTaskCountMax(),
                      pendingTaskMaxCount);

  // Attempt to add one more task.
  // Since the pending task count is full, this should fail
  bool blocked3 = true;
  size_t tasksCount3 = 1;
  std::shared_ptr<BlockTask> extraTask(new BlockTask(&monitor, &bmonitor,
                                                &blocked3, &tasksCount3));
  try {
    threadManager->add(extraTask, 1);
    BOOST_FAIL("Unexpected success adding task in excess "
               "of pending task count");
  } catch (const TooManyPendingTasksException& e) {
    BOOST_FAIL("Should have timed out adding task in excess "
               "of pending task count");
  } catch (const TimedOutException& e) {
    // Expected result
  }

  try {
    threadManager->add(extraTask, -1);
    BOOST_FAIL("Unexpected success adding task in excess "
               "of pending task count");
  } catch (const TimedOutException& e) {
    BOOST_FAIL("Unexpected timeout adding task in excess "
               "of pending task count");
  } catch (const TooManyPendingTasksException& e) {
    // Expected result
  }

  // Unblock the first set of tasks
  {
    Synchronized s(bmonitor);
    blocked1 = false;
    bmonitor.notifyAll();
  }
  // Wait for the first set of tasks to all finish
  {
    Synchronized s(monitor);
    while (tasksCount1 != 0) {
      monitor.wait();
    }
  }

  // We should be able to add the extra task now
  try {
    threadManager->add(extraTask, 1);
  } catch (const TimedOutException& e) {
    BOOST_FAIL("Unexpected timeout adding task");
  } catch (const TooManyPendingTasksException& e) {
    BOOST_FAIL("Unexpected failure adding task");
  }

  // Unblock the second set of tasks
  {
    Synchronized s(bmonitor);
    blocked2 = false;
    bmonitor.notifyAll();
  }
  {
    Synchronized s(monitor);
    while (tasksCount2 != 0) {
      monitor.wait();
    }
  }

  // Unblock the extra task
  {
    Synchronized s(bmonitor);
    blocked3 = false;
    bmonitor.notifyAll();
  }
  {
    Synchronized s(monitor);
    while (tasksCount3 != 0) {
      monitor.wait();
    }
  }

  CHECK_EQUAL_TIMEOUT(threadManager->totalTaskCount(), 0);
}

BOOST_AUTO_TEST_CASE(BlockTest) {
  int64_t timeout = 50;
  size_t numWorkers = 100;
  blockTest(timeout, numWorkers);
}

void expireTestCallback(std::shared_ptr<Runnable>, Monitor* monitor, size_t* count) {
  Synchronized s(*monitor);
  --(*count);
  if (*count == 0) {
    monitor->notify();
  }
}

void expireTest(int64_t numWorkers, int64_t expirationTimeMs) {
  int64_t maxPendingTasks = numWorkers;
  size_t activeTasks = numWorkers + maxPendingTasks;
  Monitor monitor;

  std::shared_ptr<ThreadManager> threadManager =
    ThreadManager::newSimpleThreadManager(numWorkers, maxPendingTasks);
  std::shared_ptr<PosixThreadFactory> threadFactory =
    std::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  threadManager->threadFactory(threadFactory);
  threadManager->setExpireCallback(
      std::bind(expireTestCallback, std::placeholders::_1,
                     &monitor, &activeTasks));
  threadManager->start();

  // Add numWorkers + maxPendingTasks to fill up the ThreadManager's task queue
  std::vector<std::shared_ptr<BlockTask>> tasks;
  tasks.reserve(activeTasks);

  Monitor bmonitor;
  bool blocked = true;
  for (int64_t n = 0; n < numWorkers + maxPendingTasks; ++n) {
    std::shared_ptr<BlockTask> task(new BlockTask(&monitor, &bmonitor,
                                             &blocked, &activeTasks));
    tasks.push_back(task);
    threadManager->add(task, 0, expirationTimeMs);
  }

  // Sleep for more than the expiration time
  usleep(expirationTimeMs * Util::US_PER_MS * 1.10);

  // Unblock the tasks
  {
    Synchronized s(bmonitor);
    blocked = false;
    bmonitor.notifyAll();
  }
  // Wait for all tasks to complete or expire
  {
    Synchronized s(monitor);
    while (activeTasks != 0) {
      monitor.wait();
    }
  }

  // The first numWorkers tasks should have completed,
  // the remaining ones should have expired without running
  size_t index = 0;
  for (const auto& task : tasks) {
    if (index < numWorkers) {
      BOOST_CHECK(tasks[index]->started_);
    } else {
      BOOST_CHECK(!tasks[index]->started_);
    }
    ++index;
  }
}

BOOST_AUTO_TEST_CASE(ExpireTest) {
  int64_t numWorkers = 100;
  int64_t expireTimeMs = 50;
  expireTest(numWorkers, expireTimeMs);
}

BOOST_AUTO_TEST_CASE(CodelTest) {
  int64_t numTasks = 1000;
  int64_t numWorkers = 2;
  int64_t expireTimeMs = 5;
  int64_t expectedDrops = 1;

  apache::thrift::Codel c;
  usleep(110000);
  BOOST_CHECK_EQUAL(false, c.overloaded(std::chrono::milliseconds(100)));
  usleep(90000);
  BOOST_CHECK_EQUAL(true, c.overloaded(std::chrono::milliseconds(50)));
  usleep(110000);
  BOOST_CHECK_EQUAL(false, c.overloaded(std::chrono::milliseconds(2)));
  usleep(90000);
  BOOST_CHECK_EQUAL(false, c.overloaded(std::chrono::milliseconds(20)));
}


class AddRemoveTask : public Runnable,
                      public std::enable_shared_from_this<AddRemoveTask> {
 public:
  AddRemoveTask(uint32_t timeoutUs,
                const std::shared_ptr<ThreadManager>& manager,
                Monitor* monitor,
                int64_t* count,
                int64_t* objectCount)
    : timeoutUs_(timeoutUs),
      manager_(manager),
      monitor_(monitor),
      count_(count),
      objectCount_(objectCount) {
    Synchronized s(monitor_);
    ++*objectCount_;
  }

  ~AddRemoveTask() {
    Synchronized s(monitor_);
    --*objectCount_;
  }

  void run() {
    usleep(timeoutUs_);

    {
      Synchronized s(monitor_);

      if (*count_ <= 0) {
        // The task count already dropped to 0.
        // We add more tasks than count_, so some of them may still be running
        // when count_ drops to 0.
        return;
      }

      --*count_;
      if (*count_ == 0) {
        monitor_->notifyAll();
        return;
      }
    }

    // Add ourself to the task queue again
    manager_->add(shared_from_this());
  }

 private:
  int32_t timeoutUs_;
  std::shared_ptr<ThreadManager> manager_;
  Monitor* monitor_;
  int64_t* count_;
  int64_t* objectCount_;
};

class WorkerCountChanger : public Runnable {
 public:
  WorkerCountChanger(const std::shared_ptr<ThreadManager>& manager,
                     Monitor* monitor,
                     int64_t *count,
                     int64_t* addAndRemoveCount)
    : manager_(manager),
      monitor_(monitor),
      count_(count),
      addAndRemoveCount_(addAndRemoveCount) {}

  void run() {
    // Continue adding and removing threads until the tasks are all done
    while (true) {
      {
        Synchronized s(monitor_);
        if (*count_ == 0) {
          return;
        }
        ++*addAndRemoveCount_;
      }
      addAndRemove();
    }
  }

  void addAndRemove() {
    // Add a random number of workers
    boost::uniform_int<> workerDist(1, 10);
    uint32_t workersToAdd = workerDist(rng_);
    manager_->addWorker(workersToAdd);

    boost::uniform_int<> taskDist(1, 50);
    uint32_t tasksToAdd = taskDist(rng_);

    // Sleep for a random amount of time
    boost::uniform_int<> sleepDist(1000, 5000);
    uint32_t sleepUs = sleepDist(rng_);
    usleep(sleepUs);

    // Remove the same number of workers we added
    manager_->removeWorker(workersToAdd);
  }

 private:
  boost::mt19937 rng_;
  std::shared_ptr<ThreadManager> manager_;
  Monitor* monitor_;
  int64_t* count_;
  int64_t* addAndRemoveCount_;
};

// Run lots of tasks, while several threads are all changing
// the number of worker threads.
BOOST_AUTO_TEST_CASE(AddRemoveWorker) {
  // Number of tasks to run
  int64_t numTasks = 100000;
  // Minimum number of workers to keep at any point in time
  size_t minNumWorkers = 10;
  // Number of threads that will be adding and removing workers
  int64_t numAddRemoveWorkers = 30;
  // Number of tasks to run in parallel
  int64_t numParallelTasks = 200;

  std::shared_ptr<ThreadManager> threadManager =
    ThreadManager::newSimpleThreadManager(minNumWorkers);
  std::shared_ptr<PosixThreadFactory> threadFactory =
    std::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  threadManager->threadFactory(threadFactory);
  threadManager->start();

  Monitor monitor;
  int64_t currentTaskObjects = 0;
  int64_t count = numTasks;
  int64_t addRemoveCount = 0;

  boost::mt19937 rng;
  boost::uniform_int<> taskTimeoutDist(1, 3000);
  for (int64_t n = 0; n < numParallelTasks; ++n) {
    int64_t taskTimeoutUs = taskTimeoutDist(rng);
    std::shared_ptr<AddRemoveTask> task(new AddRemoveTask(
          taskTimeoutUs, threadManager, &monitor, &count,
          &currentTaskObjects));
    threadManager->add(task);
  }

  std::shared_ptr<PosixThreadFactory> addRemoveFactory =
    std::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  addRemoveFactory->setDetached(false);
  std::deque<std::shared_ptr<Thread> > addRemoveThreads;
  for (int64_t n = 0; n < numAddRemoveWorkers; ++n) {
    std::shared_ptr<WorkerCountChanger> worker(new WorkerCountChanger(
          threadManager, &monitor, &count, &addRemoveCount));
    std::shared_ptr<Thread> thread(addRemoveFactory->newThread(worker));
    addRemoveThreads.push_back(thread);
    thread->start();
  }

  while (!addRemoveThreads.empty()) {
    addRemoveThreads.front()->join();
    addRemoveThreads.pop_front();
  }

  BOOST_TEST_MESSAGE("add remove count: " << addRemoveCount);
  BOOST_CHECK_GT(addRemoveCount, 0);

  // Stop the ThreadManager, and ensure that all Task objects have been
  // destroyed.
  threadManager->stop();
  BOOST_CHECK_EQUAL(currentTaskObjects, 0);
}

BOOST_AUTO_TEST_CASE(NeverStartedTest) {
  // Test destroying a ThreadManager that was never started.
  // This ensures that calling stop() on an unstarted ThreadManager works
  // properly.
  {
    std::shared_ptr<ThreadManager> threadManager =
      ThreadManager::newSimpleThreadManager(10);
  }

  // Destroy a ThreadManager that has a ThreadFactory but was never started.
  {
    std::shared_ptr<ThreadManager> threadManager =
      ThreadManager::newSimpleThreadManager(10);
    std::shared_ptr<PosixThreadFactory> threadFactory =
      std::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
  }
}

BOOST_AUTO_TEST_CASE(OnlyStartedTest) {
  // Destroy a ThreadManager that has a ThreadFactory and was started.
  for (int i = 0; i < 1000; ++i) {
    std::shared_ptr<ThreadManager> threadManager =
      ThreadManager::newSimpleThreadManager(10);
    std::shared_ptr<PosixThreadFactory> threadFactory =
      std::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();
  }
}

class TestObserver : public ThreadManager::Observer {
 public:
  TestObserver(int64_t timeout, const std::string& expectedName)
    : timesCalled(0)
    , timeout(timeout)
    , expectedName(expectedName) {}

  void addStats(const std::string& threadPoolName,
                const SystemClockTimePoint& queueBegin,
                const SystemClockTimePoint& workBegin,
                const SystemClockTimePoint& workEnd) {
    BOOST_CHECK_EQUAL(threadPoolName, expectedName);

    // Note: Technically could fail if system clock changes.
    BOOST_CHECK_GT((workBegin - queueBegin).count(), 0);
    BOOST_CHECK_GT((workEnd - workBegin).count(), 0);
    BOOST_CHECK_GT((workEnd - workBegin).count(), timeout - 1);
    ++timesCalled;
  }

  uint64_t timesCalled;
  int64_t timeout;
  std::string expectedName;
};

BOOST_AUTO_TEST_CASE(ObserverTest) {
  int64_t timeout = 1000;
  auto observer = std::make_shared<TestObserver>(1000, "foo");
  ThreadManager::setObserver(observer);

  Monitor monitor;
  size_t tasks = 1;

  std::shared_ptr<ThreadManager> threadManager =
      ThreadManager::newSimpleThreadManager(10);
  threadManager->setNamePrefix("foo");
  std::shared_ptr<PosixThreadFactory> threadFactory =
    std::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
  threadManager->threadFactory(threadFactory);
  threadManager->start();

  auto task = std::make_shared<LoadTask>(&monitor, &tasks, 1000);
  threadManager->add(task);
  threadManager->stop();
  BOOST_CHECK_EQUAL(observer->timesCalled, 1);
}

///////////////////////////////////////////////////////////////////////////
// init_unit_test_suite()
///////////////////////////////////////////////////////////////////////////

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value = "ThreadManagerTests";

  if (argc != 1) {
    std::cerr << "error: unhandled arguments:";
    for (int n = 1; n < argc; ++n) {
      std::cerr << " " << argv[n];
    }
    std::cerr << "\n";
    exit(1);
  }

  return nullptr;
}
