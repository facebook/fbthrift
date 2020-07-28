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

#include <thrift/lib/cpp/concurrency/ThreadManager.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <queue>
#include <set>
#include <string>

#include <glog/logging.h>

#include <folly/Conv.h>
#include <folly/CppAttributes.h>
#include <folly/ExceptionString.h>
#include <folly/GLog.h>
#include <folly/executors/Codel.h>
#include <folly/portability/GFlags.h>
#include <folly/tracing/StaticTracepoint.h>

#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>

DEFINE_bool(codel_enabled, false, "Enable codel queue timeout algorithm");

namespace apache {
namespace thrift {
namespace concurrency {

/**
 * ThreadManager class
 *
 * This class manages a pool of threads. It uses a ThreadFactory to create
 * threads.  It never actually creates or destroys worker threads, rather
 * it maintains statistics on number of idle threads, number of active threads,
 * task backlog, and average wait and service times.
 *
 * @version $Id:$
 */

class ThreadManager::Impl::Worker : public Runnable {
 public:
  Worker(ThreadManager::Impl* manager) : manager_(manager) {}

  ~Worker() override {}

  /**
   * Worker entry point
   *
   * As long as worker thread is running, pull tasks off the task queue and
   * execute.
   */
  void run() override {
    // Inform our manager that we are starting
    manager_->workerStarted(this);

    while (true) {
      // Wait for a task to run
      auto task = manager_->waitOnTask();

      // A nullptr task means that this thread is supposed to exit
      if (!task) {
        manager_->workerExiting(this);
        return;
      }

      if (manager_->queueObservers_) {
        manager_->queueObservers_->at(task->queuePriority())
            ->onDequeued(task->queueObserverPayload());
      }

      auto startTime = std::chrono::steady_clock::now();

      // Codel auto-expire time algorithm
      auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
          startTime - task->getQueueBeginTime());
      if (task->canExpire() && manager_->codel_.overloaded(delay)) {
        if (manager_->codelCallback_) {
          manager_->codelCallback_(task->getRunnable());
        }
        if (manager_->codelEnabled_) {
          FB_LOG_EVERY_MS(WARNING, 10000) << "Queueing delay timeout";

          manager_->onTaskExpired(*task);
          continue;
        }
      }

      if (manager_->observer_) {
        // Hold lock to ensure that observer_ does not get deleted
        folly::SharedMutex::ReadHolder g(manager_->observerLock_);
        if (manager_->observer_) {
          manager_->observer_->preRun(task->getContext().get());
        }
      }

      // Check if the task is expired
      if (task->canExpire() && task->getExpireTime() <= startTime) {
        manager_->onTaskExpired(*task);
        continue;
      }

      try {
        task->run();
      } catch (const std::exception& ex) {
        LOG(ERROR) << "worker task threw unhandled " << folly::exceptionStr(ex);
      } catch (...) {
        LOG(ERROR) << "worker task threw unhandled non-exception object";
      }

      auto endTime = std::chrono::steady_clock::now();
      manager_->reportTaskStats(*task, startTime, endTime);
    }
  }

 private:
  ThreadManager::Impl* manager_;
};

void ThreadManager::Impl::addWorker(size_t value) {
  for (size_t ix = 0; ix < value; ix++) {
    auto worker = std::make_shared<Worker>(this);
    auto thread = threadFactory_->newThread(worker, ThreadFactory::ATTACHED);
    {
      // We need to increment idle count
      std::unique_lock<std::mutex> l(mutex_);
      if (state_ != STARTED) {
        throw IllegalStateException(
            "ThreadManager::addWorker(): "
            "ThreadManager not running");
      }
      idleCount_++;
    }

    try {
      thread->start();
    } catch (...) {
      // If thread is started unsuccessfully, we need to decrement the
      // count we incremented above
      std::unique_lock<std::mutex> l(mutex_);
      idleCount_--;
      throw;
    }

    std::unique_lock<std::mutex> l(mutex_);
    workerCount_++;
    intendedWorkerCount_++;
  }
}

void ThreadManager::Impl::workerStarted(Worker* worker) {
  InitCallback initCallback;
  {
    std::unique_lock<std::mutex> l(mutex_);
    assert(idleCount_ > 0);
    --idleCount_;
    ++totalTaskCount_;
    std::shared_ptr<Thread> thread = worker->thread();
    *isThreadManagerThread_ = true;
    initCallback = initCallback_;
    if (!namePrefix_.empty()) {
      thread->setName(
          folly::to<std::string>(namePrefix_, "-", ++namePrefixCounter_));
    }
  }

  if (initCallback) {
    initCallback();
  }
}

void ThreadManager::Impl::workerExiting(Worker* worker) {
  std::unique_lock<std::mutex> l(mutex_);

  std::shared_ptr<Thread> thread = worker->thread();

  --workerCount_;
  --totalTaskCount_;
  deadWorkers_.push_back(thread);
  deadWorkerCond_.notify_one();
}

void ThreadManager::Impl::start() {
  std::unique_lock<std::mutex> sl(stateUpdateMutex_);
  std::unique_lock<std::mutex> l(mutex_);

  if (state_ == ThreadManager::STOPPED) {
    return;
  }

  if (state_ == ThreadManager::UNINITIALIZED) {
    if (threadFactory_ == nullptr) {
      throw InvalidArgumentException();
    }
    state_ = ThreadManager::STARTED;
    setupQueueObservers();
    cond_.notify_all();
  }
}

void ThreadManager::Impl::stopImpl(bool joinArg) {
  std::unique_lock<std::mutex> sl(stateUpdateMutex_);

  if (state_ == ThreadManager::UNINITIALIZED) {
    // The thread manager was never started.  Just ignore the stop() call.
    // This will happen if the ThreadManager is destroyed without ever being
    // started.
    joinKeepAlive();
    std::unique_lock<std::mutex> l(mutex_);
    state_ = ThreadManager::STOPPED;
  } else if (state_ == ThreadManager::STARTED) {
    joinKeepAlive();
    std::unique_lock<std::mutex> l(mutex_);
    if (joinArg) {
      state_ = ThreadManager::JOINING;
      removeWorkerImpl(l, intendedWorkerCount_, true);
      assert(tasks_.empty());
    } else {
      state_ = ThreadManager::STOPPING;
      removeWorkerImpl(l, intendedWorkerCount_);
      // Empty the task queue, in case we stopped without running
      // all of the tasks.
      totalTaskCount_ -= tasks_.size();
      std::unique_ptr<Task> task;
      while (tasks_.try_dequeue(task)) {
      }
    }
    state_ = ThreadManager::STOPPED;
    cond_.notify_all();
  } else {
    std::unique_lock<std::mutex> l(mutex_);
    // Another stopImpl() call is already in progress.
    // Just wait for the state to change to STOPPED
    while (state_ != ThreadManager::STOPPED) {
      cond_.wait(l);
    }
  }

  assert(workerCount_ == 0);
  assert(intendedWorkerCount_ == 0);
  assert(idleCount_ == 0);
  assert(totalTaskCount_ == 0);
}

void ThreadManager::Impl::removeWorker(size_t value) {
  std::unique_lock<std::mutex> l(mutex_);
  removeWorkerImpl(l, value);
}

void ThreadManager::Impl::removeWorkerImpl(
    std::unique_lock<std::mutex>& lock,
    size_t value,
    bool afterTasks) {
  assert(lock.owns_lock());

  if (value > intendedWorkerCount_) {
    throw InvalidArgumentException();
  }
  intendedWorkerCount_ -= value;

  if (afterTasks) {
    // Insert nullptr tasks onto the tasks queue to ask workers to exit
    // after all current tasks are completed
    for (size_t n = 0; n < value; ++n) {
      auto const qpriority = tasks_.priorities() / 2; // median priority
      tasks_.at_priority(qpriority).enqueue(nullptr);
      ++totalTaskCount_;
    }
    cond_.notify_all();
    for (size_t n = 0; n < value; ++n) {
      waitSem_.post();
    }
  } else {
    // Ask threads to exit ASAP
    workersToStop_ += value;
    cond_.notify_all();
    for (size_t n = 0; n < value; ++n) {
      waitSem_.post();
    }
  }

  // Wait for the specified number of threads to exit
  for (size_t n = 0; n < value; ++n) {
    while (deadWorkers_.empty()) {
      deadWorkerCond_.wait(lock);
    }

    std::shared_ptr<Thread> thread = deadWorkers_.front();
    deadWorkers_.pop_front();
    thread->join();
  }
}

bool ThreadManager::Impl::canSleep() {
  return !(*isThreadManagerThread_);
}

void ThreadManager::Impl::add(
    std::shared_ptr<Runnable> value,
    int64_t timeout,
    int64_t expiration,
    bool upstream) noexcept {
  add(0, value, timeout, expiration, upstream);
}

void ThreadManager::Impl::add(
    size_t priority,
    std::shared_ptr<Runnable> value,
    int64_t /*timeout*/,
    int64_t expiration,
    bool upstream) noexcept {
  CHECK(
      state_ != ThreadManager::UNINITIALIZED &&
      state_ != ThreadManager::STARTING)
      << "ThreadManager::Impl::add ThreadManager not started";

  if (state_ != ThreadManager::STARTED) {
    LOG(WARNING) << "abort add() that got called after join() or stop()";
    return;
  }

  priority = 2 * priority + (upstream ? 1 : 0);
  auto const qpriority = std::min(tasks_.priorities() - 1, priority);
  auto task = std::make_unique<Task>(
      std::move(value), std::chrono::milliseconds{expiration}, qpriority);
  if (queueObservers_) {
    task->queueObserverPayload() = queueObservers_->at(qpriority)->onEnqueued();
  }
  tasks_.at_priority(qpriority).enqueue(std::move(task));

  ++totalTaskCount_;

  if (idleCount_ > 0) {
    // If an idle thread is available notify it, otherwise all worker threads
    // are running and will get around to this task in time.
    waitSem_.post();
  }
}

void ThreadManager::Impl::remove(std::shared_ptr<Runnable> /*task*/) {
  std::unique_lock<std::mutex> l(mutex_);
  if (state_ != ThreadManager::STARTED) {
    throw IllegalStateException(
        "ThreadManager::Impl::remove ThreadManager not "
        "started");
  }

  throw IllegalStateException("ThreadManager::Impl::remove() not implemented");
}

std::shared_ptr<Runnable> ThreadManager::Impl::removeNextPending() {
  std::unique_lock<std::mutex> l(mutex_);
  if (state_ != ThreadManager::STARTED) {
    throw IllegalStateException(
        "ThreadManager::Impl::removeNextPending "
        "ThreadManager not started");
  }

  std::unique_ptr<Task> task;
  if (tasks_.try_dequeue(task)) {
    std::shared_ptr<Runnable> r = task->getRunnable();
    --totalTaskCount_;
    return r;
  } else {
    return std::shared_ptr<Runnable>();
  }
}

void ThreadManager::Impl::clearPending() {
  while (removeNextPending() != nullptr) {
  }
}

bool ThreadManager::Impl::shouldStop() {
  // in normal cases, only do a read (prevents cache line bounces)
  if (workersToStop_ <= 0) {
    return false;
  }
  // modify only if needed
  if (workersToStop_-- > 0) {
    return true;
  } else {
    workersToStop_++;
    return false;
  }
}

std::unique_ptr<ThreadManager::Task> ThreadManager::Impl::waitOnTask() {
  if (shouldStop()) {
    return nullptr;
  }

  std::unique_ptr<Task> task;

  // Fast path - if tasks are ready, get one
  if (tasks_.try_dequeue(task)) {
    --totalTaskCount_;
    return task;
  }

  // Otherwise, no tasks on the horizon, so go sleep
  std::unique_lock<std::mutex> l(mutex_);
  if (shouldStop()) {
    // check again because it might have changed by the time we got the mutex
    return nullptr;
  }

  ++idleCount_;
  --totalTaskCount_;
  l.unlock();
  while (!tasks_.try_dequeue(task)) {
    waitSem_.wait();
    if (shouldStop()) {
      std::unique_lock<std::mutex> l2(mutex_);
      --idleCount_;
      ++totalTaskCount_;
      return nullptr;
    }
  }
  --idleCount_;
  // totalTaskCount_ doesn't change:
  // the decrement of idleCount_ and the dequeueing of a task cancel each other

  return task;
}

void ThreadManager::Impl::onTaskExpired(const Task& task) {
  ExpireCallback expireCallback;
  {
    std::unique_lock<std::mutex> l(mutex_);
    expiredCount_++;
    expireCallback = expireCallback_;
  }

  if (expireCallback) {
    // Expired callback should _not_ be called holding mutex_
    expireCallback(task.getRunnable());
  }
}

void ThreadManager::Impl::setExpireCallback(ExpireCallback expireCallback) {
  expireCallback_ = expireCallback;
}

void ThreadManager::Impl::setCodelCallback(ExpireCallback expireCallback) {
  codelCallback_ = expireCallback;
}

void ThreadManager::Impl::getStats(
    std::chrono::microseconds& waitTime,
    std::chrono::microseconds& runTime,
    int64_t maxItems) {
  folly::MSLGuard g(statsLock_);
  if (numTasks_) {
    if (numTasks_ >= maxItems) {
      waitingTimeUs_ /= numTasks_;
      executingTimeUs_ /= numTasks_;
      numTasks_ = 1;
    }
    waitTime = waitingTimeUs_ / numTasks_;
    runTime = executingTimeUs_ / numTasks_;
  } else {
    waitTime = std::chrono::microseconds::zero();
    runTime = std::chrono::microseconds::zero();
  }
}

void ThreadManager::Impl::reportTaskStats(
    const Task& task,
    const std::chrono::steady_clock::time_point& workBegin,
    const std::chrono::steady_clock::time_point& workEnd) {
  auto queueBegin = task.getQueueBeginTime();
  auto waitTime = workBegin - queueBegin;
  auto runTime = workEnd - workBegin;

  // Times in this USDT use granularity of std::chrono::steady_clock::duration,
  // which is platform dependent. On Facebook servers, the granularity is
  // nanoseconds. We explicitly do not perform any unit conversions to avoid
  // unneccessary costs and leave it to consumers of this data to know what
  // effective clock resolution is.
  FOLLY_SDT(
      thrift,
      thread_manager_task_stats,
      namePrefix_.c_str(),
      task.getContext() ? task.getContext()->getRootId() : 0,
      queueBegin.time_since_epoch().count(),
      waitTime.count(),
      runTime.count());

  if (enableTaskStats_) {
    folly::MSLGuard g(statsLock_);
    auto waitTimeUs =
        std::chrono::duration_cast<std::chrono::microseconds>(waitTime);
    auto runTimeUs =
        std::chrono::duration_cast<std::chrono::microseconds>(runTime);
    waitingTimeUs_ += waitTimeUs;
    executingTimeUs_ += runTimeUs;
    ++numTasks_;
  }

  // Optimistic check lock free
  if (ThreadManager::Impl::observer_) {
    // Hold lock to ensure that observer_ does not get deleted.
    folly::SharedMutex::ReadHolder g(ThreadManager::Impl::observerLock_);
    if (ThreadManager::Impl::observer_) {
      // Note: We are assuming the namePrefix_ does not change after the thread
      // is started.
      // TODO: enforce this.
      auto seriesName = folly::to<std::string>(namePrefix_, statContext(task));
      ThreadManager::Impl::observer_->postRun(
          task.getContext().get(),
          {seriesName, queueBegin, workBegin, workEnd});
    }
  }
}

void ThreadManager::Impl::enableCodel(bool enabled) {
  codelEnabled_ = enabled || FLAGS_codel_enabled;
}

folly::Codel* ThreadManager::Impl::getCodel() {
  return &codel_;
}

void ThreadManager::Impl::setupQueueObservers() {
  if (auto factory = folly::QueueObserverFactory::make(
          "tm." + (namePrefix_.empty() ? "unk" : namePrefix_),
          tasks_.priorities())) {
    queueObservers_.emplace(tasks_.priorities());
    for (size_t pri = 0; pri < tasks_.priorities(); ++pri) {
      queueObservers_->at(pri) = factory->create(pri);
    }
  }
}

std::shared_ptr<ThreadManager> ThreadManager::newThreadManager() {
  return std::make_shared<ThreadManager::Impl>();
}

void ThreadManager::setObserver(
    std::shared_ptr<ThreadManager::Observer> observer) {
  {
    folly::SharedMutex::WriteHolder g(observerLock_);
    observer_.swap(observer);
  }
}

std::shared_ptr<ThreadManager> ThreadManager::newSimpleThreadManager(
    size_t count,
    bool enableTaskStats) {
  return std::make_shared<SimpleThreadManager>(count, enableTaskStats);
}

std::shared_ptr<ThreadManager> ThreadManager::newSimpleThreadManager(
    const std::string& name,
    size_t count,
    bool enableTaskStats) {
  auto simpleThreadManager =
      std::make_shared<SimpleThreadManager>(count, enableTaskStats);
  simpleThreadManager->setNamePrefix(name);
  return simpleThreadManager;
}

std::shared_ptr<ThreadManager> ThreadManager::newPriorityQueueThreadManager(
    size_t numThreads,
    bool enableTaskStats) {
  auto tm =
      std::make_shared<PriorityQueueThreadManager>(numThreads, enableTaskStats);
  tm->threadFactory(Factory(PosixThreadFactory::NORMAL_PRI));
  return tm;
}

std::shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    const std::array<
        std::pair<std::shared_ptr<ThreadFactory>, size_t>,
        N_PRIORITIES>& factories,
    bool enableTaskStats) {
  auto copy = factories;
  if (copy[PRIORITY::NORMAL].second < NORMAL_PRIORITY_MINIMUM_THREADS) {
    LOG(INFO) << "Creating minimum threads of NORMAL priority: "
              << NORMAL_PRIORITY_MINIMUM_THREADS;
    copy[PRIORITY::NORMAL].second = NORMAL_PRIORITY_MINIMUM_THREADS;
  }
  return std::make_shared<PriorityThreadManager::PriorityImpl>(
      copy, enableTaskStats);
}

std::shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    const std::array<size_t, N_PRIORITIES>& counts,
    bool enableTaskStats) {
  static_assert(N_PRIORITIES == 5, "Implementation is out-of-date");
  // Note that priorities for HIGH and IMPORTANT are the same, the difference
  // is in the number of threads.
  const std::array<
      std::pair<std::shared_ptr<ThreadFactory>, size_t>,
      N_PRIORITIES>
      factories{{
          {Factory(PosixThreadFactory::HIGHER_PRI),
           counts[PRIORITY::HIGH_IMPORTANT]},
          {Factory(PosixThreadFactory::HIGH_PRI), counts[PRIORITY::HIGH]},
          {Factory(PosixThreadFactory::HIGH_PRI), counts[PRIORITY::IMPORTANT]},
          {Factory(PosixThreadFactory::NORMAL_PRI), counts[PRIORITY::NORMAL]},
          {Factory(PosixThreadFactory::LOW_PRI), counts[PRIORITY::BEST_EFFORT]},
      }};
  return newPriorityThreadManager(factories, enableTaskStats);
}

std::shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    size_t normalThreadsCount,
    bool enableTaskStats) {
  return newPriorityThreadManager(
      {{2, 2, 2, normalThreadsCount, 2}}, enableTaskStats);
}

folly::SharedMutex ThreadManager::observerLock_;
std::shared_ptr<ThreadManager::Observer> ThreadManager::observer_;

} // namespace concurrency
} // namespace thrift
} // namespace apache
