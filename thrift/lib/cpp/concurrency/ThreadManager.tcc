/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp/concurrency/ThreadManager.h>

#include <assert.h>

#include <memory>
#include <queue>
#include <set>
#include <atomic>

#if defined(DEBUG)
#include <iostream>
#endif //defined(DEBUG)

#include <folly/Conv.h>
#include <folly/Logging.h>
#include <folly/MPMCQueue.h>
#include <folly/Memory.h>
#include <folly/io/async/Request.h>
#include <folly/String.h>
#include <thrift/lib/cpp/concurrency/Exception.h>
#include <thrift/lib/cpp/concurrency/Monitor.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <wangle/concurrent/Codel.h>

#include <thrift/lib/cpp/concurrency/ThreadManager-impl.h>

namespace apache { namespace thrift { namespace concurrency {

using std::shared_ptr;
using std::make_shared;
using std::dynamic_pointer_cast;
using std::unique_ptr;
using folly::RequestContext;

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

template <typename SemType>
template <typename SemTypeT>
class ThreadManager::ImplT<SemType>::Worker : public Runnable {
 public:
  Worker(ThreadManager::ImplT<SemTypeT>* manager) :
    manager_(manager) {}

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

      // Getting the current time is moderately expensive,
      // so only get the time if we actually need it.
      SystemClockTimePoint startTime;
      if (task->canExpire() || task->statsEnabled()) {
        startTime = SystemClock::now();

        // Codel auto-expire time algorithm
        auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(
          startTime - task->getQueueBeginTime());

        if (manager_->codel_.overloaded(delay)) {
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
          folly::RWSpinLock::ReadHolder g(manager_->observerLock_);
          if (manager_->observer_) {
            manager_->observer_->preRun(task->getContext().get());
          }
        }
      }

      // Check if the task is expired
      if (task->canExpire() &&
          task->getExpireTime() <= startTime) {
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

      if (task->statsEnabled()) {
        auto endTime = SystemClock::now();
        manager_->reportTaskStats(*task,
                                  startTime,
                                  endTime);
      }
    }
  }

 private:
  ThreadManager::ImplT<SemType>* manager_;
};

template <typename SemType>
void ThreadManager::ImplT<SemType>::addWorker(size_t value) {
  for (size_t ix = 0; ix < value; ix++) {
    auto worker = make_shared<Worker<SemType>>(this);
    auto thread = threadFactory_->newThread(worker,
                                            ThreadFactory::ATTACHED);
    {
      // We need to increment idle count
      Guard g(mutex_);
      if (state_ != STARTED) {
        throw IllegalStateException("ThreadManager::addWorker(): "
                                    "ThreadManager not running");
      }
      idleCount_++;
    }

    try {
      thread->start();
    } catch (...) {
      // If thread is started unsuccessfully, we need to decrement the
      // count we incremented above
      Guard g(mutex_);
      idleCount_--;
      throw;
    }

    Guard g(mutex_);
    workerCount_++;
    intendedWorkerCount_++;
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::workerStarted(Worker<SemType>* worker) {
  InitCallback initCallback;
  {
    Guard g(mutex_);
    assert(idleCount_ > 0);
    --idleCount_;
    ++totalTaskCount_;
    shared_ptr<Thread> thread = worker->thread();
    idMap_.insert(std::make_pair(thread->getId(), thread));
    initCallback = initCallback_;
    if (!namePrefix_.empty()) {
      thread->setName(folly::to<std::string>(namePrefix_, "-",
                                             ++namePrefixCounter_));
    }
  }

  if (initCallback) {
    initCallback();
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::workerExiting(Worker<SemType>* worker) {
  Guard g(mutex_);

  shared_ptr<Thread> thread = worker->thread();
  size_t numErased = idMap_.erase(thread->getId());
  DCHECK_EQ(numErased, 1);

  --workerCount_;
  --totalTaskCount_;
  deadWorkers_.push_back(thread);
  deadWorkerMonitor_.notify();
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::start() {
  Guard g(mutex_);

  if (state_ == ThreadManager::STOPPED) {
    return;
  }

  if (state_ == ThreadManager::UNINITIALIZED) {
    if (threadFactory_ == nullptr) {
      throw InvalidArgumentException();
    }
    state_ = ThreadManager::STARTED;
    monitor_.notifyAll();
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::stopImpl(bool joinArg) {
  Guard g(mutex_);

  if (state_ == ThreadManager::UNINITIALIZED) {
    // The thread manager was never started.  Just ignore the stop() call.
    // This will happen if the ThreadManager is destroyed without ever being
    // started.
  } else if (state_ == ThreadManager::STARTED) {
    if (joinArg) {
      state_ = ThreadManager::JOINING;
      removeWorkerImpl(intendedWorkerCount_, true);
      assert(tasks_.isEmpty());
    } else {
      state_ = ThreadManager::STOPPING;
      removeWorkerImpl(intendedWorkerCount_);
      // Empty the task queue, in case we stopped without running
      // all of the tasks.
      totalTaskCount_ -= tasks_.size();
      std::unique_ptr<Task> task;
      while (tasks_.read(task)) { }
    }
    state_ = ThreadManager::STOPPED;
    monitor_.notifyAll();
    g.release();
  } else {
    // Another stopImpl() call is already in progress.
    // Just wait for the state to change to STOPPED
    while (state_ != ThreadManager::STOPPED) {
      monitor_.wait();
    }
  }

  assert(workerCount_ == 0);
  assert(intendedWorkerCount_ == 0);
  assert(idleCount_ == 0);
  assert(totalTaskCount_ == 0);
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::removeWorker(size_t value) {
  Guard g(mutex_);
  removeWorkerImpl(value);
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::removeWorkerImpl(size_t value, bool afterTasks) {
  assert(mutex_.isLocked());

  if (value > intendedWorkerCount_) {
    throw InvalidArgumentException();
  }
  intendedWorkerCount_ -= value;

  if (afterTasks) {
    // Insert nullptr tasks onto the tasks queue to ask workers to exit
    // after all current tasks are completed
    size_t bad = 0;
    for (size_t n = 0; n < value; ++n) {
      if (!tasks_.write(nullptr)) {
        LOG(ERROR) << "Can't remove worker. Increase maxQueueLen?";
        bad++;
        continue;
      }
      ++totalTaskCount_;
    }
    monitor_.notifyAll();
    for (size_t n = 0; n < value; ++n) {
      waitSem_.post();
    }
    intendedWorkerCount_ += bad;
    value -= bad;
  } else {
    // Ask threads to exit ASAP
    workersToStop_ += value;
    monitor_.notifyAll();
    for (size_t n = 0; n < value; ++n) {
      waitSem_.post();
    }
  }

  // Wait for the specified number of threads to exit
  for (size_t n = 0; n < value; ++n) {
    while (deadWorkers_.empty()) {
      deadWorkerMonitor_.wait();
    }

    shared_ptr<Thread> thread = deadWorkers_.front();
    deadWorkers_.pop_front();
    thread->join();
  }
}

template <typename SemType>
bool ThreadManager::ImplT<SemType>::canSleep() {
  assert(mutex_.isLocked());
  const Thread::id_t id = threadFactory_->getCurrentThreadId();
  return idMap_.find(id) == idMap_.end();
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::add(shared_ptr<Runnable> value,
                                        int64_t timeout,
                                        int64_t expiration,
                                        bool /*cancellable*/,
                                        bool numa) {
  if (numa) {
    VLOG_EVERY_N(1, 100) << "ThreadManager::add called with numa == true, but "
                         << "not a NumaThreadManager";
  }

  if (state_ != ThreadManager::STARTED) {
    throw IllegalStateException("ThreadManager::Impl::add ThreadManager "
                                "not started");
  }

  if (pendingTaskCountMax_ > 0
      && tasks_.size() >= folly::to<ssize_t>(pendingTaskCountMax_)) {
    Guard g(mutex_, timeout);

    if (!g) {
      throw TimedOutException();
    }
    if (canSleep() && timeout >= 0) {
      while (pendingTaskCountMax_ > 0
             && tasks_.size() >= folly::to<ssize_t>(pendingTaskCountMax_)) {
        // This is thread safe because the mutex is shared between monitors.
        maxMonitor_.wait(timeout);
      }
    } else {
      throw TooManyPendingTasksException();
    }
  }

  auto task = folly::make_unique<Task>(std::move(value),
                                       std::chrono::milliseconds{expiration});
  if (!tasks_.write(std::move(task))) {
    LOG(ERROR) << "Failed to enqueue item. Increase maxQueueLen?";
    throw TooManyPendingTasksException();
  }

  ++totalTaskCount_;

  if (idleCount_ > 0) {
    // If an idle thread is available notify it, otherwise all worker threads
    // are running and will get around to this task in time.
    waitSem_.post();
  }
}

template <typename SemType>
bool ThreadManager::ImplT<SemType>::tryAdd(shared_ptr<Runnable> value) {
  if (state_ != ThreadManager::STARTED) {
    return false;
  }
  if (pendingTaskCountMax_ > 0 &&
      tasks_.size() >= folly::to<ssize_t>(pendingTaskCountMax_)) {
    return false;
  }

  auto task = folly::make_unique<Task>(std::move(value),
                                       std::chrono::milliseconds{0});
  if (!tasks_.write(std::move(task))) {
    return false;
  }

  ++totalTaskCount_;

  if (idleCount_ > 0) {
    // If an idle thread is available notify it, otherwise all worker threads
    // are running and will get around to this task in time.
    waitSem_.post();
  }

  return true;
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::remove(shared_ptr<Runnable> /*task*/) {
  Synchronized s(monitor_);
  if (state_ != ThreadManager::STARTED) {
    throw IllegalStateException("ThreadManager::Impl::remove ThreadManager not "
                                "started");
  }

  throw IllegalStateException("ThreadManager::Impl::remove() not implemented");
}

template <typename SemType>
std::shared_ptr<Runnable> ThreadManager::ImplT<SemType>::removeNextPending() {
  Guard g(mutex_);
  if (state_ != ThreadManager::STARTED) {
    throw IllegalStateException("ThreadManager::Impl::removeNextPending "
                                "ThreadManager not started");
  }

  std::unique_ptr<Task> task;
  if (tasks_.read(task)) {
    std::shared_ptr<Runnable> r = task->getRunnable();
    --totalTaskCount_;
    maybeNotifyMaxMonitor(false);
    return r;
  } else {
    return std::shared_ptr<Runnable>();
  }
}

template <typename SemType>
bool ThreadManager::ImplT<SemType>::shouldStop() {
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

template <typename SemType>
std::unique_ptr<ThreadManager::Task>
ThreadManager::ImplT<SemType>::waitOnTask() {
  if (shouldStop()) {
    return nullptr;
  }

  std::unique_ptr<Task> task;

  // Fast path - if tasks are ready, get one
  if (tasks_.read(task)) {
    --totalTaskCount_;
    maybeNotifyMaxMonitor(true);
    return task;
  }

  // Otherwise, no tasks on the horizon, so go sleep
  Guard g(mutex_);
  if (shouldStop()) {
    // check again because it might have changed by the time we got the mutex
    return nullptr;
  }

  ++idleCount_;
  --totalTaskCount_;
  g.release();
  while (!tasks_.read(task)) {
    waitSem_.wait();
    if (shouldStop()) {
      Guard f(mutex_);
      --idleCount_;
      ++totalTaskCount_;
      return nullptr;
    }
  }
  --idleCount_;
  // totalTaskCount_ doesn't change:
  // the decrement of idleCount_ and the dequeueing of a task cancel each other

  maybeNotifyMaxMonitor(true);
  return task;
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::maybeNotifyMaxMonitor(bool shouldLock) {
  if (pendingTaskCountMax_ != 0
      && tasks_.size() < folly::to<ssize_t>(pendingTaskCountMax_)) {
    if (shouldLock) {
      Guard g(mutex_);
      maxMonitor_.notify();
    } else {
      assert(mutex_.isLocked());
      maxMonitor_.notify();
    }
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::onTaskExpired(const Task& task) {
  ExpireCallback expireCallback;
  {
    Guard g(mutex_);
    expiredCount_++;
    expireCallback = expireCallback_;
  }

  if (expireCallback) {
    // Expired callback should _not_ be called holding mutex_
    expireCallback(task.getRunnable());
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::setExpireCallback(ExpireCallback expireCallback) {
  expireCallback_ = expireCallback;
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::setCodelCallback(ExpireCallback expireCallback) {
  codelCallback_ = expireCallback;
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::getStats(int64_t& waitTimeUs, int64_t& runTimeUs,
                                   int64_t maxItems) {
  folly::MSLGuard g(statsLock_);
  if (numTasks_) {
    if (numTasks_ >= maxItems) {
      waitingTimeUs_ /= numTasks_;
      executingTimeUs_ /= numTasks_;
      numTasks_ = 1;
    }
    waitTimeUs = waitingTimeUs_ / numTasks_;
    runTimeUs = executingTimeUs_ / numTasks_;
  } else {
    waitTimeUs = 0;
    runTimeUs = 0;
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::reportTaskStats(
    const Task& task,
    const SystemClockTimePoint& workBegin,
    const SystemClockTimePoint& workEnd) {
  auto queueBegin = task.getQueueBeginTime();
  int64_t waitTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
      workBegin - queueBegin).count();
  int64_t runTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
      workEnd - workBegin).count();
  if (enableTaskStats_) {
    folly::MSLGuard g(statsLock_);
    waitingTimeUs_ += waitTimeUs;
    executingTimeUs_ += runTimeUs;
    ++numTasks_;
  }

  // Optimistic check lock free
  if (ThreadManager::ImplT<SemType>::observer_) {
    // Hold lock to ensure that observer_ does not get deleted.
    folly::RWSpinLock::ReadHolder g(ThreadManager::ImplT<SemType>::observerLock_);
    if (ThreadManager::ImplT<SemType>::observer_) {
      // Note: We are assuming the namePrefix_ does not change after the thread is
      // started.
      // TODO: enforce this.
      ThreadManager::ImplT<SemType>::observer_->postRun(
          task.getContext().get(),
          {namePrefix_, queueBegin, workBegin, workEnd});
    }
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::enableCodel(bool enabled) {
  codelEnabled_ = enabled || FLAGS_codel_enabled;
}

template <typename SemType>
Codel* ThreadManager::ImplT<SemType>::getCodel() {
  return &codel_;
}

template <typename SemType>
class SimpleThreadManager : public ThreadManager::ImplT<SemType> {

 public:
  explicit SimpleThreadManager(size_t workerCount = 4,
                               size_t pendingTaskCountMax = 0,
                               bool enableTaskStats = false,
                               size_t maxQueueLen = 0) :
    ThreadManager::Impl(pendingTaskCountMax, enableTaskStats, maxQueueLen)
    , workerCount_(workerCount) {
  }

  void start() override {
    if (this->state() == this->STARTED) {
      return;
    }
    ThreadManager::ImplT<SemType>::start();
    this->addWorker(workerCount_);
  }

 private:
  const size_t workerCount_;
};


template <typename SemType>
shared_ptr<ThreadManager> ThreadManager::newSimpleThreadManager(
                                                    size_t count,
                                                    size_t pendingTaskCountMax,
                                                    bool enableTaskStats,
                                                    size_t maxQueueLen) {
  return make_shared<SimpleThreadManager<SemType>>(count, pendingTaskCountMax,
                                          enableTaskStats, maxQueueLen);
}

template <typename SemType>
class PriorityThreadManager::PriorityImplT : public PriorityThreadManager {
 public:
  PriorityImplT(const std::array<std::pair<shared_ptr<ThreadFactory>, size_t>,
                                 N_PRIORITIES>& factories,
                bool enableTaskStats = false,
                size_t maxQueueLen = 0) {
    for (int i = 0; i < N_PRIORITIES; i++) {
      unique_ptr<ThreadManager> m(
        new ThreadManager::ImplT<SemType>(0, enableTaskStats, maxQueueLen));
      m->threadFactory(factories[i].first);
      managers_[i] = std::move(m);
      counts_[i] = factories[i].second;
    }
  }

  void start() override {
    Guard g(mutex_);
    for (int i = 0; i < N_PRIORITIES; i++) {
      if (managers_[i]->state() == STARTED) {
        continue;
      }
      managers_[i]->start();
      managers_[i]->addWorker(counts_[i]);
    }
  }

  void stop() override {
    Guard g(mutex_);
    for (auto& m : managers_) {
      m->stop();
    }
  }

  void join() override {
    Guard g(mutex_);
    for (auto& m : managers_) {
      m->join();
    }
  }

  std::string getNamePrefix() const override {
    return managers_[0]->getNamePrefix();
  }

  void setNamePrefix(const std::string& name) override {
    for (int i = 0; i < N_PRIORITIES; i++) {
      managers_[i]->setNamePrefix(folly::to<std::string>(name, "-pri", i));
    }
  }

  void addWorker(size_t value) override {
    addWorker(NORMAL, value);
  }

  void removeWorker(size_t value) override {
    removeWorker(NORMAL, value);
  }

  void addWorker(PRIORITY priority, size_t value) override {
    managers_[priority]->addWorker(value);
  }

  void removeWorker(PRIORITY priority, size_t value) override {
    managers_[priority]->removeWorker(value);
  }

  STATE state() const override {
    size_t started = 0;
    Guard g(mutex_);
    for (auto& m : managers_) {
      STATE cur_state = m->state();
      switch (cur_state) {
        case UNINITIALIZED:
        case STARTING:
        case JOINING:
        case STOPPING:
          return cur_state;
        case STARTED:
          started++;
          break;
        case STOPPED:
          break;
      }
    }
    if (started == 0) {
      return STOPPED;
    }
    return STARTED;
  }

  std::shared_ptr<ThreadFactory> threadFactory() const override {
    throw IllegalStateException("Not implemented");
    return std::shared_ptr<ThreadFactory>();
  }

  void threadFactory(std::shared_ptr<ThreadFactory> value) override {
    Guard g(mutex_);
    for (auto& m : managers_) {
      m->threadFactory(value);
    }
  }

  void add(std::shared_ptr<Runnable> task,
           int64_t timeout = 0,
           int64_t expiration = 0,
           bool cancellable = false,
           bool numa = false) override {
    PriorityRunnable* p = dynamic_cast<PriorityRunnable*>(task.get());
    PRIORITY prio = p ? p->getPriority() : NORMAL;
    add(prio, std::move(task), timeout, expiration, cancellable, numa);
  }

  void add(PRIORITY priority,
           std::shared_ptr<Runnable> task,
           int64_t timeout = 0,
           int64_t expiration = 0,
           bool cancellable = false,
           bool numa = false) override {
    managers_[priority]->add(
        std::move(task), timeout, expiration, cancellable, numa);
  }

  bool tryAdd(std::shared_ptr<Runnable> task) override {
    PriorityRunnable* p = dynamic_cast<PriorityRunnable*>(task.get());
    PRIORITY prio = p ? p->getPriority() : NORMAL;
    return tryAdd(prio, std::move(task));
  }

  bool tryAdd(PRIORITY priority, std::shared_ptr<Runnable> task) override {
    return managers_[priority]->tryAdd(std::move(task));
  }

  /**
   * Implements folly::Executor::add()
   */
  void add(folly::Func f) override {
    add(FunctionRunner::create(std::move(f)));
  }

  /**
   * Implements folly::Executor::addWithPriority()
   * Maps executor priority task to respective PriorityThreadManager threads:
   *  >= 3 pri tasks to 'HIGH_IMPORTANT' threads,
   *  2 pri tasks to 'HIGH' threads,
   *  1 pri tasks to 'IMPORTANT' threads,
   *  0 pri tasks to 'NORMAL' threads,
   *  <= -1 pri tasks to 'BEST_EFFORT' threads,
   */
  void addWithPriority(folly::Func f, int8_t priority) override {
    PRIORITY prio = PRIORITY::NORMAL;
    if (priority >= 3) {
      prio = PRIORITY::HIGH_IMPORTANT;
    } else if (priority == 2) {
      prio = PRIORITY::HIGH;
    } else if (priority == 1) {
      prio = PRIORITY::IMPORTANT;
    } else if (priority == 0) {
      prio = PRIORITY::NORMAL;
    } else if (priority <= -1) {
      prio = PRIORITY::BEST_EFFORT;
    }
    add(prio, FunctionRunner::create(std::move(f)));
  }

  template <typename T>
  size_t sum(T method) const {
    size_t count = 0;
    for (const auto& m : managers_) {
      count += ((*m).*method)();
    }
    return count;
  }

  size_t idleWorkerCount() const override {
    return sum(&ThreadManager::idleWorkerCount);
  }

  size_t workerCount() const override {
    return sum(&ThreadManager::workerCount);
  }

  size_t pendingTaskCount() const override {
    return sum(&ThreadManager::pendingTaskCount);
  }

  size_t totalTaskCount() const override {
    return sum(&ThreadManager::totalTaskCount);
  }

  size_t pendingTaskCountMax() const override {
    throw IllegalStateException("Not implemented");
    return 0;
  }

  size_t expiredTaskCount() override {
      return sum(&ThreadManager::expiredTaskCount);
  }

  void remove(std::shared_ptr<Runnable> /*task*/) override {
    throw IllegalStateException("Not implemented");
  }

  std::shared_ptr<Runnable> removeNextPending() override {
    throw IllegalStateException("Not implemented");
    return std::shared_ptr<Runnable>();
  }

  void setExpireCallback(ExpireCallback expireCallback) override {
    for (const auto& m : managers_) {
      m->setExpireCallback(expireCallback);
    }
  }

  void setCodelCallback(ExpireCallback expireCallback) override {
    for (const auto& m : managers_) {
      m->setCodelCallback(expireCallback);
    }
  }

  void setThreadInitCallback(InitCallback /*initCallback*/) override {
    throw IllegalStateException("Not implemented");
  }

  void enableCodel(bool enabled) override {
    for (const auto& m : managers_) {
      m->enableCodel(enabled);
    }
  }

  Codel* getCodel() override {
    return getCodel(NORMAL);
  }

  Codel* getCodel(PRIORITY priority) override {
    return managers_[priority]->getCodel();
  }

private:
  unique_ptr<ThreadManager> managers_[N_PRIORITIES];
  size_t counts_[N_PRIORITIES];
  Mutex mutex_;
};

static inline shared_ptr<ThreadFactory> Factory(PosixThreadFactory::PRIORITY prio) {
  return make_shared<PosixThreadFactory>(PosixThreadFactory::OTHER, prio);
}

static const size_t NORMAL_PRIORITY_MINIMUM_THREADS = 1;

template <typename SemType>
shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    const std::array<std::pair<shared_ptr<ThreadFactory>, size_t>,
                     N_PRIORITIES>& factories,
    bool enableTaskStats,
    size_t maxQueueLen) {
  auto copy = factories;
  if (copy[PRIORITY::NORMAL].second < NORMAL_PRIORITY_MINIMUM_THREADS) {
    LOG(INFO) << "Creating minimum threads of NORMAL priority: "
              << NORMAL_PRIORITY_MINIMUM_THREADS;
    copy[PRIORITY::NORMAL].second = NORMAL_PRIORITY_MINIMUM_THREADS;
  }
  return std::make_shared<PriorityThreadManager::PriorityImplT<SemType>>(
      copy, enableTaskStats, maxQueueLen);
}

template <typename SemType>
shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    const std::array<size_t, N_PRIORITIES>& counts,
    bool enableTaskStats,
    size_t maxQueueLen) {
  static_assert(N_PRIORITIES == 5, "Implementation is out-of-date");
  // Note that priorities for HIGH and IMPORTANT are the same, the difference
  // is in the number of threads.
  const std::array<std::pair<shared_ptr<ThreadFactory>, size_t>, N_PRIORITIES>
    factories{{
      {Factory(PosixThreadFactory::HIGHER), counts[PRIORITY::HIGH_IMPORTANT]},
      {Factory(PosixThreadFactory::HIGH),   counts[PRIORITY::HIGH]},
      {Factory(PosixThreadFactory::HIGH),   counts[PRIORITY::IMPORTANT]},
      {Factory(PosixThreadFactory::NORMAL), counts[PRIORITY::NORMAL]},
      {Factory(PosixThreadFactory::LOW),    counts[PRIORITY::BEST_EFFORT]},
  }};
  return newPriorityThreadManager<SemType>(
      factories, enableTaskStats, maxQueueLen);
}

template <typename SemType>
shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    size_t normalThreadsCount,
    bool enableTaskStats,
    size_t maxQueueLen) {
  return newPriorityThreadManager<SemType>({{2, 2, 2, normalThreadsCount, 2}},
                                           enableTaskStats,
                                           maxQueueLen);
}

}}} // apache::thrift::concurrency
