/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <thread>

#include <folly/Synchronized.h>
#include <folly/executors/Codel.h>
#include <folly/portability/GTest.h>
#include <folly/portability/SysResource.h>
#include <folly/portability/SysTime.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp/concurrency/FunctionRunner.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/concurrency/Util.h>

using namespace apache::thrift::concurrency;

class ThreadManagerTest : public testing::Test {
 public:
  ~ThreadManagerTest() override {
    ThreadManager::setObserver(nullptr);
  }

 private:
  gflags::FlagSaver flagsaver_;
};

// Loops until x==y for up to timeout ms.
// The end result is the same as of {EXPECT,ASSERT}_EQ(x,y)
// (depending on OP) if x!=y after the timeout passes
#define X_EQUAL_SPECIFIC_TIMEOUT(OP, timeout, x, y)         \
  do {                                                      \
    using std::chrono::steady_clock;                        \
    using std::chrono::milliseconds;                        \
    auto end = steady_clock::now() + milliseconds(timeout); \
    while ((x) != (y) && steady_clock::now() < end) {       \
    }                                                       \
    OP##_EQ(x, y);                                          \
  } while (0)

#define CHECK_EQUAL_SPECIFIC_TIMEOUT(timeout, x, y) \
  X_EQUAL_SPECIFIC_TIMEOUT(EXPECT, timeout, x, y)
#define REQUIRE_EQUAL_SPECIFIC_TIMEOUT(timeout, x, y) \
  X_EQUAL_SPECIFIC_TIMEOUT(ASSERT, timeout, x, y)

// A default timeout of 1 sec should be long enough for other threads to
// stabilize the values of x and y, and short enough to catch real errors
// when x is not going to be equal to y anytime soon
#define CHECK_EQUAL_TIMEOUT(x, y) CHECK_EQUAL_SPECIFIC_TIMEOUT(1000, x, y)
#define REQUIRE_EQUAL_TIMEOUT(x, y) REQUIRE_EQUAL_SPECIFIC_TIMEOUT(1000, x, y)

class LoadTask : public Runnable {
 public:
  LoadTask(
      std::mutex* mutex,
      std::condition_variable* cond,
      size_t* count,
      int64_t timeout)
      : mutex_(mutex),
        cond_(cond),
        count_(count),
        timeout_(timeout),
        startTime_(0),
        endTime_(0) {}

  void run() override {
    startTime_ = Util::currentTime();
    usleep(timeout_ * Util::US_PER_MS);
    endTime_ = Util::currentTime();

    {
      std::unique_lock<std::mutex> l(*mutex_);

      (*count_)--;
      if (*count_ == 0) {
        cond_->notify_one();
      }
    }
  }

  std::mutex* mutex_;
  std::condition_variable* cond_;
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
static void loadTest(size_t numTasks, int64_t timeout, size_t numWorkers) {
  std::mutex mutex;
  std::condition_variable cond;
  size_t tasksLeft = numTasks;

  auto threadManager = ThreadManager::newSimpleThreadManager(numWorkers, true);
  auto threadFactory = std::make_shared<PosixThreadFactory>();
  threadManager->threadFactory(threadFactory);
  threadManager->start();

  std::set<std::shared_ptr<LoadTask>> tasks;
  for (size_t n = 0; n < numTasks; n++) {
    tasks.insert(
        std::make_shared<LoadTask>(&mutex, &cond, &tasksLeft, timeout));
  }

  int64_t startTime = Util::currentTime();
  for (const auto& task : tasks) {
    threadManager->add(task);
  }

  int64_t tasksStartedTime = Util::currentTime();

  {
    std::unique_lock<std::mutex> l(mutex);
    while (tasksLeft > 0) {
      cond.wait(l);
    }
  }
  int64_t endTime = Util::currentTime();

  int64_t firstTime = std::numeric_limits<int64_t>::max();
  int64_t lastTime = 0;
  double averageTime = 0;
  int64_t minTime = std::numeric_limits<int64_t>::max();
  int64_t maxTime = 0;

  for (const auto& task : tasks) {
    EXPECT_GT(task->startTime_, 0);
    EXPECT_GT(task->endTime_, 0);

    int64_t delta = task->endTime_ - task->startTime_;
    assert(delta > 0);

    firstTime = std::min(firstTime, task->startTime_);
    lastTime = std::max(lastTime, task->endTime_);
    minTime = std::min(minTime, delta);
    maxTime = std::max(maxTime, delta);

    averageTime += delta;
  }
  averageTime /= numTasks;

  LOG(INFO) << "first start: " << firstTime << "ms "
            << "last end: " << lastTime << "ms "
            << "min: " << minTime << "ms "
            << "max: " << maxTime << "ms "
            << "average: " << averageTime << "ms";

  double idealTime = ((numTasks + (numWorkers - 1)) / numWorkers) * timeout;
  double actualTime = endTime - startTime;
  double taskStartTime = tasksStartedTime - startTime;

  double overheadPct = (actualTime - idealTime) / idealTime;
  if (overheadPct < 0) {
    overheadPct *= -1.0;
  }

  LOG(INFO) << "ideal time: " << idealTime << "ms "
            << "actual time: " << actualTime << "ms "
            << "task startup time: " << taskStartTime << "ms "
            << "overhead: " << overheadPct * 100.0 << "%";

  // Fail if the test took 10% more time than the ideal time
  EXPECT_LT(overheadPct, 0.10);

  // Get the task stats
  std::chrono::microseconds waitTimeUs;
  std::chrono::microseconds runTimeUs;
  threadManager->getStats(waitTimeUs, runTimeUs, numTasks * 2);

  // Compute the best possible average wait time
  int64_t fullIterations = numTasks / numWorkers;
  int64_t tasksOnLastIteration = numTasks % numWorkers;
  auto expectedTotalWaitTimeMs = std::chrono::milliseconds(
      numWorkers * ((fullIterations * (fullIterations - 1)) / 2) * timeout +
      tasksOnLastIteration * fullIterations * timeout);
  auto idealAvgWaitUs =
      std::chrono::microseconds(expectedTotalWaitTimeMs) / numTasks;

  LOG(INFO) << "avg wait time: " << waitTimeUs.count() << "us "
            << "avg run time: " << runTimeUs.count() << "us "
            << "ideal wait time: " << idealAvgWaitUs.count() << "us";

  const auto doubleMilliToMicro = [](double val) {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::duration<double, std::milli>(val));
  };
  // Verify that the average run time was more than the timeout, but not
  // more than 10% over.
  EXPECT_GE(runTimeUs, std::chrono::milliseconds(timeout));
  EXPECT_LT(runTimeUs, doubleMilliToMicro(timeout * 1.10));
  // Verify that the average wait time was within 10% of the ideal wait time.
  // The calculation for ideal average wait time assumes all tasks were started
  // instantaneously, in reality, starting 1000 tasks takes some non-zero amount
  // of time, so later tasks will actually end up waiting for *less* than the
  // ideal wait time. Account for this by accepting an actual avg wait time that
  // is less than ideal avg wait time by up to the time it took to start all the
  // tasks.
  EXPECT_GE(waitTimeUs, idealAvgWaitUs - doubleMilliToMicro(taskStartTime));
  EXPECT_LT(waitTimeUs, doubleMilliToMicro(idealAvgWaitUs.count() * 1.10));
}

TEST_F(ThreadManagerTest, LoadTest) {
  size_t numTasks = 10000;
  int64_t timeout = 50;
  size_t numWorkers = 100;
  loadTest(numTasks, timeout, numWorkers);
}

class BlockTask : public Runnable {
 public:
  BlockTask(
      std::mutex* mutex,
      std::condition_variable* cond,
      std::mutex* bmutex,
      std::condition_variable* bcond,
      bool* blocked,
      size_t* count)
      : mutex_(mutex),
        cond_(cond),
        bmutex_(bmutex),
        bcond_(bcond),
        blocked_(blocked),
        count_(count),
        started_(false) {}

  void run() override {
    started_ = true;
    {
      std::unique_lock<std::mutex> l(*bmutex_);
      while (*blocked_) {
        bcond_->wait(l);
      }
    }

    {
      std::unique_lock<std::mutex> l(*mutex_);
      (*count_)--;
      if (*count_ == 0) {
        cond_->notify_one();
      }
    }
  }

  std::mutex* mutex_;
  std::condition_variable* cond_;
  std::mutex* bmutex_;
  std::condition_variable* bcond_;
  bool* blocked_;
  size_t* count_;
  bool started_;
};

static void expireTestCallback(
    std::shared_ptr<Runnable>,
    std::mutex* mutex,
    std::condition_variable* cond,
    size_t* count) {
  std::unique_lock<std::mutex> l(*mutex);
  --(*count);
  if (*count == 0) {
    cond->notify_one();
  }
}

static void expireTest(size_t numWorkers, int64_t expirationTimeMs) {
  size_t maxPendingTasks = numWorkers;
  size_t activeTasks = numWorkers + maxPendingTasks;
  std::mutex mutex;
  std::condition_variable cond;

  auto threadManager = ThreadManager::newSimpleThreadManager(numWorkers);
  auto threadFactory = std::make_shared<PosixThreadFactory>();
  threadManager->threadFactory(threadFactory);
  threadManager->setExpireCallback(std::bind(
      expireTestCallback, std::placeholders::_1, &mutex, &cond, &activeTasks));
  threadManager->start();

  // Add numWorkers + maxPendingTasks to fill up the ThreadManager's task queue
  std::vector<std::shared_ptr<BlockTask>> tasks;
  tasks.reserve(activeTasks);

  std::mutex bmutex;
  std::condition_variable bcond;
  bool blocked = true;
  for (size_t n = 0; n < numWorkers + maxPendingTasks; ++n) {
    auto task = std::make_shared<BlockTask>(
        &mutex, &cond, &bmutex, &bcond, &blocked, &activeTasks);
    tasks.push_back(task);
    threadManager->add(task, 0, expirationTimeMs);
  }

  // Sleep for more than the expiration time
  usleep(expirationTimeMs * Util::US_PER_MS * 1.10);

  // Unblock the tasks
  {
    std::unique_lock<std::mutex> l(bmutex);
    blocked = false;
    bcond.notify_all();
  }
  // Wait for all tasks to complete or expire
  {
    std::unique_lock<std::mutex> l(mutex);
    while (activeTasks != 0) {
      cond.wait(l);
    }
  }

  // The first numWorkers tasks should have completed,
  // the remaining ones should have expired without running
  for (size_t index = 0; index < tasks.size(); ++index) {
    if (index < numWorkers) {
      EXPECT_TRUE(tasks[index]->started_);
    } else {
      EXPECT_TRUE(!tasks[index]->started_);
    }
  }
}

TEST_F(ThreadManagerTest, ExpireTest) {
  size_t numWorkers = 100;
  int64_t expireTimeMs = 50;
  expireTest(numWorkers, expireTimeMs);
}

class AddRemoveTask : public Runnable,
                      public std::enable_shared_from_this<AddRemoveTask> {
 public:
  AddRemoveTask(
      uint32_t timeoutUs,
      const std::shared_ptr<ThreadManager>& manager,
      std::mutex* mutex,
      std::condition_variable* cond,
      int64_t* count,
      int64_t* objectCount)
      : timeoutUs_(timeoutUs),
        manager_(manager),
        mutex_(mutex),
        cond_(cond),
        count_(count),
        objectCount_(objectCount) {
    std::unique_lock<std::mutex> l(*mutex_);
    ++*objectCount_;
  }

  ~AddRemoveTask() override {
    std::unique_lock<std::mutex> l(*mutex_);
    --*objectCount_;
  }

  void run() override {
    usleep(timeoutUs_);

    {
      std::unique_lock<std::mutex> l(*mutex_);

      if (*count_ <= 0) {
        // The task count already dropped to 0.
        // We add more tasks than count_, so some of them may still be running
        // when count_ drops to 0.
        return;
      }

      --*count_;
      if (*count_ == 0) {
        cond_->notify_all();
        return;
      }
    }

    // Add ourself to the task queue again
    manager_->add(shared_from_this());
  }

 private:
  int32_t timeoutUs_;
  std::shared_ptr<ThreadManager> manager_;
  std::mutex* mutex_;
  std::condition_variable* cond_;
  int64_t* count_;
  int64_t* objectCount_;
};

class WorkerCountChanger : public Runnable {
 public:
  WorkerCountChanger(
      const std::shared_ptr<ThreadManager>& manager,
      std::mutex* mutex,
      int64_t* count,
      int64_t* addAndRemoveCount)
      : manager_(manager),
        mutex_(mutex),
        count_(count),
        addAndRemoveCount_(addAndRemoveCount) {}

  void run() override {
    // Continue adding and removing threads until the tasks are all done
    while (true) {
      {
        std::unique_lock<std::mutex> l(*mutex_);
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
    std::uniform_int_distribution<> workerDist(1, 10);
    uint32_t workersToAdd = workerDist(rng_);
    manager_->addWorker(workersToAdd);

    std::uniform_int_distribution<> taskDist(1, 50);
    uint32_t tasksToAdd = taskDist(rng_);
    (void)tasksToAdd;

    // Sleep for a random amount of time
    std::uniform_int_distribution<> sleepDist(1000, 5000);
    uint32_t sleepUs = sleepDist(rng_);
    usleep(sleepUs);

    // Remove the same number of workers we added
    manager_->removeWorker(workersToAdd);
  }

 private:
  std::mt19937 rng_;
  std::shared_ptr<ThreadManager> manager_;
  std::mutex* mutex_;
  int64_t* count_;
  int64_t* addAndRemoveCount_;
};

// Run lots of tasks, while several threads are all changing
// the number of worker threads.
TEST_F(ThreadManagerTest, AddRemoveWorker) {
  // Number of tasks to run
  int64_t numTasks = 100000;
  // Minimum number of workers to keep at any point in time
  size_t minNumWorkers = 10;
  // Number of threads that will be adding and removing workers
  int64_t numAddRemoveWorkers = 30;
  // Number of tasks to run in parallel
  int64_t numParallelTasks = 200;

  auto threadManager = ThreadManager::newSimpleThreadManager(minNumWorkers);
  auto threadFactory = std::make_shared<PosixThreadFactory>();
  threadManager->threadFactory(threadFactory);
  threadManager->start();

  std::mutex mutex;
  std::condition_variable cond;
  int64_t currentTaskObjects = 0;
  int64_t count = numTasks;
  int64_t addRemoveCount = 0;

  std::mt19937 rng;
  std::uniform_int_distribution<> taskTimeoutDist(1, 3000);
  for (int64_t n = 0; n < numParallelTasks; ++n) {
    int64_t taskTimeoutUs = taskTimeoutDist(rng);
    auto task = std::make_shared<AddRemoveTask>(
        taskTimeoutUs,
        threadManager,
        &mutex,
        &cond,
        &count,
        &currentTaskObjects);
    threadManager->add(task);
  }

  auto addRemoveFactory = std::make_shared<PosixThreadFactory>();
  addRemoveFactory->setDetached(false);
  std::deque<std::shared_ptr<Thread>> addRemoveThreads;
  for (int64_t n = 0; n < numAddRemoveWorkers; ++n) {
    auto worker = std::make_shared<WorkerCountChanger>(
        threadManager, &mutex, &count, &addRemoveCount);
    auto thread = addRemoveFactory->newThread(worker);
    addRemoveThreads.push_back(thread);
    thread->start();
  }

  while (!addRemoveThreads.empty()) {
    addRemoveThreads.front()->join();
    addRemoveThreads.pop_front();
  }

  LOG(INFO) << "add remove count: " << addRemoveCount;
  EXPECT_GT(addRemoveCount, 0);

  // Stop the ThreadManager, and ensure that all Task objects have been
  // destroyed.
  threadManager->stop();
  EXPECT_EQ(0, currentTaskObjects);
}

TEST_F(ThreadManagerTest, NeverStartedTest) {
  // Test destroying a ThreadManager that was never started.
  // This ensures that calling stop() on an unstarted ThreadManager works
  // properly.
  {
    auto threadManager = ThreadManager::newSimpleThreadManager(10); //
  }

  // Destroy a ThreadManager that has a ThreadFactory but was never started.
  {
    auto threadManager = ThreadManager::newSimpleThreadManager(10);
    auto threadFactory = std::make_shared<PosixThreadFactory>();
    threadManager->threadFactory(threadFactory);
  }
}

TEST_F(ThreadManagerTest, OnlyStartedTest) {
  // Destroy a ThreadManager that has a ThreadFactory and was started.
  for (int i = 0; i < 1000; ++i) {
    auto threadManager = ThreadManager::newSimpleThreadManager(10);
    auto threadFactory = std::make_shared<PosixThreadFactory>();
    threadManager->threadFactory(threadFactory);
    threadManager->start();
  }
}

class TestObserver : public ThreadManager::Observer {
 public:
  TestObserver(int64_t timeout, const std::string& expectedName)
      : timesCalled(0), timeout(timeout), expectedName(expectedName) {}

  void preRun(folly::RequestContext*) override {}
  void postRun(folly::RequestContext*, const ThreadManager::RunStats& stats)
      override {
    EXPECT_EQ(expectedName, stats.threadPoolName);

    // Note: Technically could fail if system clock changes.
    EXPECT_GT((stats.workBegin - stats.queueBegin).count(), 0);
    EXPECT_GT((stats.workEnd - stats.workBegin).count(), 0);
    EXPECT_GT((stats.workEnd - stats.workBegin).count(), timeout - 1);
    ++timesCalled;
  }

  uint64_t timesCalled;
  int64_t timeout;
  std::string expectedName;
};

class FailThread : public PthreadThread {
 public:
  FailThread(
      int policy,
      int priority,
      int stackSize,
      bool detached,
      std::shared_ptr<Runnable> runnable)
      : PthreadThread(policy, priority, stackSize, detached, runnable) {}

  void start() override {
    throw 2;
  }
};

class FailThreadFactory : public PosixThreadFactory {
 public:
  class FakeImpl : public Impl {
   public:
    FakeImpl(
        POLICY policy,
        PosixThreadFactory::PRIORITY priority,
        int stackSize,
        DetachState detached)
        : Impl(policy, priority, stackSize, detached) {}

    std::shared_ptr<Thread> newThread(
        const std::shared_ptr<Runnable>& runnable,
        DetachState detachState) const override {
      auto result = std::make_shared<FailThread>(
          toPthreadPolicy(policy_),
          toPthreadPriority(policy_, priority_),
          stackSize_,
          detachState == DETACHED,
          runnable);
      result->weakRef(result);
      runnable->thread(result);
      return result;
    }
  };

  explicit FailThreadFactory(
      POLICY /*policy*/ = kDefaultPolicy,
      PRIORITY /*priority*/ = kDefaultPriority,
      int /*stackSize*/ = kDefaultStackSizeMB,
      bool detached = true) {
    impl_ = std::make_shared<FailThreadFactory::FakeImpl>(
        kDefaultPolicy,
        kDefaultPriority,
        kDefaultStackSizeMB,
        detached ? DETACHED : ATTACHED);
  }
};

class DummyFailureClass {
 public:
  DummyFailureClass() {
    threadManager_ = ThreadManager::newSimpleThreadManager(20);
    threadManager_->setNamePrefix("foo");
    auto threadFactory = std::make_shared<FailThreadFactory>();
    threadManager_->threadFactory(threadFactory);
    threadManager_->start();
  }

 private:
  std::shared_ptr<ThreadManager> threadManager_;
};

TEST_F(ThreadManagerTest, ThreadStartFailureTest) {
  for (int i = 0; i < 10; i++) {
    EXPECT_THROW(DummyFailureClass(), int);
  }
}

TEST_F(ThreadManagerTest, ObserverTest) {
  auto observer = std::make_shared<TestObserver>(1000, "foo");
  ThreadManager::setObserver(observer);

  std::mutex mutex;
  std::condition_variable cond;
  size_t tasks = 1;

  auto threadManager = ThreadManager::newSimpleThreadManager(10);
  threadManager->setNamePrefix("foo");
  threadManager->threadFactory(std::make_shared<PosixThreadFactory>());
  threadManager->start();

  auto task = std::make_shared<LoadTask>(&mutex, &cond, &tasks, 1000);
  threadManager->add(task);
  threadManager->join();
  EXPECT_EQ(1, observer->timesCalled);
}

TEST_F(ThreadManagerTest, ObserverAssignedAfterStart) {
  class MyTask : public Runnable {
   public:
    void run() override {}
  };
  class MyObserver : public ThreadManager::Observer {
   public:
    MyObserver(std::string name, std::shared_ptr<std::string> tgt)
        : name_(std::move(name)), tgt_(std::move(tgt)) {}
    void preRun(folly::RequestContext*) override {}
    void postRun(folly::RequestContext*, const ThreadManager::RunStats&)
        override {
      *tgt_ = name_;
    }

   private:
    std::string name_;
    std::shared_ptr<std::string> tgt_;
  };

  // start a tm
  auto tm = ThreadManager::newSimpleThreadManager(1);
  tm->setNamePrefix("foo");
  tm->threadFactory(std::make_shared<PosixThreadFactory>());
  tm->start();
  // set the observer w/ observable side-effect
  auto tgt = std::make_shared<std::string>();
  ThreadManager::setObserver(std::make_shared<MyObserver>("bar", tgt));
  // add a task - observable side-effect should trigger
  tm->add(std::make_shared<MyTask>());
  tm->join();
  // confirm the side-effect
  EXPECT_EQ("bar", *tgt);
}

TEST_F(ThreadManagerTest, PosixThreadFactoryPriority) {
  auto getNiceValue = [](PosixThreadFactory::PRIORITY prio) -> int {
    PosixThreadFactory factory(PosixThreadFactory::OTHER, prio);
    factory.setDetached(false);
    int result = 0;
    auto t = factory.newThread(
        FunctionRunner::create([&] { result = getpriority(PRIO_PROCESS, 0); }));
    t->start();
    t->join();
    return result;
  };

  // NOTE: Test may not have permission to raise priority,
  // so use prio <= NORMAL.
  EXPECT_EQ(0, getNiceValue(PosixThreadFactory::NORMAL_PRI));
  EXPECT_LT(0, getNiceValue(PosixThreadFactory::LOW_PRI));
  auto th = std::thread([&] {
    for (int i = 0; i < 20; ++i) {
      if (setpriority(PRIO_PROCESS, 0, i) != 0) {
        PLOG(WARNING) << "failed setpriority(" << i << ")";
        continue;
      }
      EXPECT_EQ(i, getNiceValue(PosixThreadFactory::INHERITED_PRI));
    }
  });
  th.join();
}

TEST_F(ThreadManagerTest, PriorityThreadManagerWorkerCount) {
  auto threadManager = PriorityThreadManager::newPriorityThreadManager({{
      1 /*HIGH_IMPORTANT*/,
      2 /*HIGH*/,
      3 /*IMPORTANT*/,
      4 /*NORMAL*/,
      5 /*BEST_EFFORT*/
  }});
  threadManager->start();

  EXPECT_EQ(1, threadManager->workerCount(PRIORITY::HIGH_IMPORTANT));
  EXPECT_EQ(2, threadManager->workerCount(PRIORITY::HIGH));
  EXPECT_EQ(3, threadManager->workerCount(PRIORITY::IMPORTANT));
  EXPECT_EQ(4, threadManager->workerCount(PRIORITY::NORMAL));
  EXPECT_EQ(5, threadManager->workerCount(PRIORITY::BEST_EFFORT));

  threadManager->addWorker(PRIORITY::HIGH_IMPORTANT, 1);
  threadManager->addWorker(PRIORITY::HIGH, 1);
  threadManager->addWorker(PRIORITY::IMPORTANT, 1);
  threadManager->addWorker(PRIORITY::NORMAL, 1);
  threadManager->addWorker(PRIORITY::BEST_EFFORT, 1);

  EXPECT_EQ(2, threadManager->workerCount(PRIORITY::HIGH_IMPORTANT));
  EXPECT_EQ(3, threadManager->workerCount(PRIORITY::HIGH));
  EXPECT_EQ(4, threadManager->workerCount(PRIORITY::IMPORTANT));
  EXPECT_EQ(5, threadManager->workerCount(PRIORITY::NORMAL));
  EXPECT_EQ(6, threadManager->workerCount(PRIORITY::BEST_EFFORT));

  threadManager->removeWorker(PRIORITY::HIGH_IMPORTANT, 1);
  threadManager->removeWorker(PRIORITY::HIGH, 1);
  threadManager->removeWorker(PRIORITY::IMPORTANT, 1);
  threadManager->removeWorker(PRIORITY::NORMAL, 1);
  threadManager->removeWorker(PRIORITY::BEST_EFFORT, 1);

  EXPECT_EQ(1, threadManager->workerCount(PRIORITY::HIGH_IMPORTANT));
  EXPECT_EQ(2, threadManager->workerCount(PRIORITY::HIGH));
  EXPECT_EQ(3, threadManager->workerCount(PRIORITY::IMPORTANT));
  EXPECT_EQ(4, threadManager->workerCount(PRIORITY::NORMAL));
  EXPECT_EQ(5, threadManager->workerCount(PRIORITY::BEST_EFFORT));
}

TEST_F(ThreadManagerTest, PriorityQueueThreadManagerExecutor) {
  auto threadManager =
      ThreadManager::newPriorityQueueThreadManager(1, true /*stats*/);
  threadManager->start();
  folly::Baton<> reqSyncBaton;
  folly::Baton<> reqDoneBaton;
  // block the TM
  threadManager->add([&] { reqSyncBaton.wait(); });

  std::string foo = "";
  threadManager->addWithPriority(
      [&] {
        foo += "a";
        reqDoneBaton.post();
      },
      0);
  // Should be added by default at highest priority
  threadManager->add([&] { foo += "b"; });
  threadManager->addWithPriority([&] { foo += "c"; }, 1);

  // unblock the TM
  reqSyncBaton.post();

  // wait until the request that's supposed to finish last is done
  reqDoneBaton.wait();

  EXPECT_EQ("bca", foo);
}
