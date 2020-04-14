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
#include <memory>
#include <queue>
#include <set>

#include <folly/Conv.h>
#include <folly/DefaultKeepAliveExecutor.h>
#include <folly/GLog.h>
#include <folly/MPMCQueue.h>
#include <folly/Memory.h>
#include <folly/String.h>
#include <folly/executors/Codel.h>
#include <folly/io/async/Request.h>

#include <thrift/lib/cpp/concurrency/Exception.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/Thread.h>

#include <thrift/lib/cpp/concurrency/ThreadManager-impl.h>

namespace apache {
namespace thrift {
namespace concurrency {

using folly::RequestContext;
using std::dynamic_pointer_cast;
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;

/* Translates from wangle priorities (normal at 0, higher is higher)
   to thrift priorities */

inline PRIORITY translatePriority(int8_t priority) {
  if (priority >= 3) {
    return PRIORITY::HIGH_IMPORTANT;
  } else if (priority == 2) {
    return PRIORITY::HIGH;
  } else if (priority == 1) {
    return PRIORITY::IMPORTANT;
  } else if (priority == 0) {
    return PRIORITY::NORMAL;
  } else if (priority <= -1) {
    return PRIORITY::BEST_EFFORT;
  }
  folly::assume_unreachable();
}

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
  Worker(ThreadManager::ImplT<SemTypeT>* manager) : manager_(manager) {}

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
      std::chrono::steady_clock::time_point startTime;
      if (task->canExpire() || task->statsEnabled()) {
        startTime = std::chrono::steady_clock::now();

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

      if (task->statsEnabled()) {
        auto endTime = std::chrono::steady_clock::now();
        manager_->reportTaskStats(*task, startTime, endTime);
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

template <typename SemType>
void ThreadManager::ImplT<SemType>::workerStarted(Worker<SemType>* worker) {
  InitCallback initCallback;
  {
    std::unique_lock<std::mutex> l(mutex_);
    assert(idleCount_ > 0);
    --idleCount_;
    ++totalTaskCount_;
    shared_ptr<Thread> thread = worker->thread();
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

template <typename SemType>
void ThreadManager::ImplT<SemType>::workerExiting(Worker<SemType>* worker) {
  std::unique_lock<std::mutex> l(mutex_);

  shared_ptr<Thread> thread = worker->thread();

  --workerCount_;
  --totalTaskCount_;
  deadWorkers_.push_back(thread);
  deadWorkerCond_.notify_one();
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::start() {
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
    cond_.notify_all();
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::stopImpl(bool joinArg) {
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

template <typename SemType>
void ThreadManager::ImplT<SemType>::removeWorker(size_t value) {
  std::unique_lock<std::mutex> l(mutex_);
  removeWorkerImpl(l, value);
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::removeWorkerImpl(
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

    shared_ptr<Thread> thread = deadWorkers_.front();
    deadWorkers_.pop_front();
    thread->join();
  }
}

template <typename SemType>
bool ThreadManager::ImplT<SemType>::canSleep() {
  return !(*isThreadManagerThread_);
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::add(
    shared_ptr<Runnable> value,
    int64_t timeout,
    int64_t expiration,
    bool cancellable) noexcept {
  add(1, value, timeout, expiration, cancellable);
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::add(
    size_t priority,
    shared_ptr<Runnable> value,
    int64_t /*timeout*/,
    int64_t expiration,
    bool /*cancellable*/) noexcept {
  CHECK(
      state_ != ThreadManager::UNINITIALIZED &&
      state_ != ThreadManager::STARTING)
      << "ThreadManager::Impl::add ThreadManager not started";

  if (state_ != ThreadManager::STARTED) {
    LOG(WARNING) << "abort add() that got called after join() or stop()";
    return;
  }

  auto task = std::make_unique<Task>(
      std::move(value), std::chrono::milliseconds{expiration});
  auto const qpriority = std::min(tasks_.priorities() - 1, priority);
  tasks_.at_priority(qpriority).enqueue(std::move(task));

  ++totalTaskCount_;

  if (idleCount_ > 0) {
    // If an idle thread is available notify it, otherwise all worker threads
    // are running and will get around to this task in time.
    waitSem_.post();
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::remove(shared_ptr<Runnable> /*task*/) {
  std::unique_lock<std::mutex> l(mutex_);
  if (state_ != ThreadManager::STARTED) {
    throw IllegalStateException(
        "ThreadManager::Impl::remove ThreadManager not "
        "started");
  }

  throw IllegalStateException("ThreadManager::Impl::remove() not implemented");
}

template <typename SemType>
std::shared_ptr<Runnable> ThreadManager::ImplT<SemType>::removeNextPending() {
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

template <typename SemType>
void ThreadManager::ImplT<SemType>::clearPending() {
  while (removeNextPending() != nullptr) {
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

template <typename SemType>
void ThreadManager::ImplT<SemType>::onTaskExpired(const Task& task) {
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

template <typename SemType>
void ThreadManager::ImplT<SemType>::setExpireCallback(
    ExpireCallback expireCallback) {
  expireCallback_ = expireCallback;
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::setCodelCallback(
    ExpireCallback expireCallback) {
  codelCallback_ = expireCallback;
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::getStats(
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

template <typename SemType>
void ThreadManager::ImplT<SemType>::reportTaskStats(
    const Task& task,
    const std::chrono::steady_clock::time_point& workBegin,
    const std::chrono::steady_clock::time_point& workEnd) {
  auto queueBegin = task.getQueueBeginTime();
  auto waitTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
      workBegin - queueBegin);
  auto runTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
      workEnd - workBegin);
  if (enableTaskStats_) {
    folly::MSLGuard g(statsLock_);
    waitingTimeUs_ += waitTimeUs;
    executingTimeUs_ += runTimeUs;
    ++numTasks_;
  }

  // Optimistic check lock free
  if (ThreadManager::ImplT<SemType>::observer_) {
    // Hold lock to ensure that observer_ does not get deleted.
    folly::SharedMutex::ReadHolder g(
        ThreadManager::ImplT<SemType>::observerLock_);
    if (ThreadManager::ImplT<SemType>::observer_) {
      // Note: We are assuming the namePrefix_ does not change after the thread
      // is started.
      // TODO: enforce this.
      auto seriesName = folly::to<std::string>(namePrefix_, statContext(task));
      ThreadManager::ImplT<SemType>::observer_->postRun(
          task.getContext().get(),
          {seriesName, queueBegin, workBegin, workEnd});
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
  explicit SimpleThreadManager(
      size_t workerCount = 4,
      bool enableTaskStats = false)
      : ThreadManager::Impl(enableTaskStats), workerCount_(workerCount) {}

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
    bool enableTaskStats) {
  return make_shared<SimpleThreadManager<SemType>>(count, enableTaskStats);
}

template <typename SemType>
shared_ptr<ThreadManager> ThreadManager::newSimpleThreadManager(
    const std::string& name,
    size_t count,
    bool enableTaskStats) {
  auto simpleThreadManager =
      make_shared<SimpleThreadManager<SemType>>(count, enableTaskStats);
  simpleThreadManager->setNamePrefix(name);
  return simpleThreadManager;
}

template <typename SemType>
class PriorityQueueThreadManager : public ThreadManager::ImplT<SemType> {
 public:
  typedef apache::thrift::concurrency::PRIORITY PRIORITY;
  explicit PriorityQueueThreadManager(
      size_t numThreads,
      bool enableTaskStats = false)
      : ThreadManager::ImplT<SemType>(enableTaskStats, N_PRIORITIES),
        numThreads_(numThreads) {}

  class PriorityFunctionRunner
      : public virtual apache::thrift::concurrency::PriorityRunnable,
        public virtual FunctionRunner {
   public:
    PriorityFunctionRunner(
        apache::thrift::concurrency::PriorityThreadManager::PRIORITY priority,
        folly::Func&& f)
        : FunctionRunner(std::move(f)), priority_(priority) {}

    apache::thrift::concurrency::PRIORITY getPriority() const override {
      return priority_;
    }

   private:
    apache::thrift::concurrency::PriorityThreadManager::PRIORITY priority_;
  };

  using ThreadManager::ImplT<SemType>::add;

  void add(
      std::shared_ptr<Runnable> task,
      int64_t timeout = 0,
      int64_t expiration = 0,
      bool cancellable = false) noexcept override {
    PriorityRunnable* p = dynamic_cast<PriorityRunnable*>(task.get());
    PRIORITY prio = p ? p->getPriority() : NORMAL;
    ThreadManager::ImplT<SemType>::add(
        prio, std::move(task), timeout, expiration, cancellable);
  }

  /**
   * Implements folly::Executor::add()
   */
  void add(folly::Func f) override {
    // We default adds of this kind to highest priority; as ThriftServer
    // doesn't use this itself, this is typically used by the application,
    // and we want to prioritize inflight requests over admitting new request.
    // arguably, we may even want a priority above the max we ever allow for
    // initial queueing
    ThreadManager::ImplT<SemType>::add(
        HIGH_IMPORTANT,
        make_shared<PriorityFunctionRunner>(HIGH_IMPORTANT, std::move(f)),
        0,
        0,
        false);
  }

  /**
   * Implements folly::Executor::addWithPriority()
   */
  void addWithPriority(folly::Func f, int8_t priority) override {
    auto prio = translatePriority(priority);
    ThreadManager::ImplT<SemType>::add(
        prio,
        make_shared<PriorityFunctionRunner>(prio, std::move(f)),
        0,
        0,
        false);
  }

  uint8_t getNumPriorities() const override {
    return N_PRIORITIES;
  }

  void start() override {
    ThreadManager::ImplT<SemType>::start();
    ThreadManager::ImplT<SemType>::addWorker(numThreads_);
  }

  void setNamePrefix(const std::string& name) override {
    // This isn't thread safe, but neither is PriorityThreadManager's version
    // This should only be called at initialization
    ThreadManager::ImplT<SemType>::setNamePrefix(name);
    for (int i = 0; i < N_PRIORITIES; i++) {
      statContexts_[i] = folly::to<std::string>(name, "-pri", i);
    }
  }

  using Task = typename ThreadManager::ImplT<SemType>::Task;

  std::string statContext(const Task& task) override {
    PriorityRunnable* p =
        dynamic_cast<PriorityRunnable*>(task.getRunnable().get());
    PRIORITY prio = p ? p->getPriority() : NORMAL;
    return statContexts_[prio];
  }

 private:
  size_t numThreads_;
  std::string statContexts_[N_PRIORITIES];
};

static inline shared_ptr<ThreadFactory> Factory(
    PosixThreadFactory::PRIORITY prio) {
  return make_shared<PosixThreadFactory>(PosixThreadFactory::OTHER, prio);
}

template <typename SemType>
shared_ptr<ThreadManager> ThreadManager::newPriorityQueueThreadManager(
    size_t numThreads,
    bool enableTaskStats) {
  auto tm = make_shared<PriorityQueueThreadManager<SemType>>(
      numThreads, enableTaskStats);
  tm->threadFactory(Factory(PosixThreadFactory::NORMAL_PRI));
  return tm;
}

template <typename SemType>
class PriorityThreadManager::PriorityImplT
    : public PriorityThreadManager,
      public folly::DefaultKeepAliveExecutor {
 public:
  PriorityImplT(
      const std::array<
          std::pair<shared_ptr<ThreadFactory>, size_t>,
          N_PRIORITIES>& factories,
      bool enableTaskStats = false) {
    for (int i = 0; i < N_PRIORITIES; i++) {
      unique_ptr<ThreadManager> m(
          new ThreadManager::ImplT<SemType>(enableTaskStats));
      m->threadFactory(factories[i].first);
      managers_[i] = std::move(m);
      counts_[i] = factories[i].second;
    }
  }

  ~PriorityImplT() {
    if (!std::exchange(keepAliveJoined_, true)) {
      joinKeepAlive();
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
    joinKeepAliveOnce();
    for (auto& m : managers_) {
      m->stop();
    }
  }

  void join() override {
    Guard g(mutex_);
    joinKeepAliveOnce();
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

  size_t workerCount(PRIORITY priority) override {
    return managers_[priority]->workerCount();
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

  void add(
      std::shared_ptr<Runnable> task,
      int64_t timeout = 0,
      int64_t expiration = 0,
      bool cancellable = false) noexcept override {
    PriorityRunnable* p = dynamic_cast<PriorityRunnable*>(task.get());
    PRIORITY prio = p ? p->getPriority() : NORMAL;
    add(prio, std::move(task), timeout, expiration, cancellable);
  }

  void add(
      PRIORITY priority,
      std::shared_ptr<Runnable> task,
      int64_t timeout = 0,
      int64_t expiration = 0,
      bool cancellable = false) noexcept override {
    managers_[priority]->add(std::move(task), timeout, expiration, cancellable);
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
    auto prio = translatePriority(priority);
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

  size_t pendingTaskCount(PRIORITY priority) const override {
    return managers_[priority]->pendingTaskCount();
  }

  size_t totalTaskCount() const override {
    return sum(&ThreadManager::totalTaskCount);
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

  void clearPending() override {
    for (const auto& m : managers_) {
      m->clearPending();
    }
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
  void joinKeepAliveOnce() {
    if (!std::exchange(keepAliveJoined_, true)) {
      joinKeepAlive();
    }
  }

  unique_ptr<ThreadManager> managers_[N_PRIORITIES];
  size_t counts_[N_PRIORITIES];
  Mutex mutex_;
  bool keepAliveJoined_{false};
};

static const size_t NORMAL_PRIORITY_MINIMUM_THREADS = 1;

template <typename SemType>
shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    const std::array<
        std::pair<shared_ptr<ThreadFactory>, size_t>,
        N_PRIORITIES>& factories,
    bool enableTaskStats) {
  auto copy = factories;
  if (copy[PRIORITY::NORMAL].second < NORMAL_PRIORITY_MINIMUM_THREADS) {
    LOG(INFO) << "Creating minimum threads of NORMAL priority: "
              << NORMAL_PRIORITY_MINIMUM_THREADS;
    copy[PRIORITY::NORMAL].second = NORMAL_PRIORITY_MINIMUM_THREADS;
  }
  return std::make_shared<PriorityThreadManager::PriorityImplT<SemType>>(
      copy, enableTaskStats);
}

template <typename SemType>
shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    const std::array<size_t, N_PRIORITIES>& counts,
    bool enableTaskStats) {
  static_assert(N_PRIORITIES == 5, "Implementation is out-of-date");
  // Note that priorities for HIGH and IMPORTANT are the same, the difference
  // is in the number of threads.
  const std::array<std::pair<shared_ptr<ThreadFactory>, size_t>, N_PRIORITIES>
      factories{{
          {Factory(PosixThreadFactory::HIGHER_PRI),
           counts[PRIORITY::HIGH_IMPORTANT]},
          {Factory(PosixThreadFactory::HIGH_PRI), counts[PRIORITY::HIGH]},
          {Factory(PosixThreadFactory::HIGH_PRI), counts[PRIORITY::IMPORTANT]},
          {Factory(PosixThreadFactory::NORMAL_PRI), counts[PRIORITY::NORMAL]},
          {Factory(PosixThreadFactory::LOW_PRI), counts[PRIORITY::BEST_EFFORT]},
      }};
  return newPriorityThreadManager<SemType>(factories, enableTaskStats);
}

template <typename SemType>
shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    size_t normalThreadsCount,
    bool enableTaskStats) {
  return newPriorityThreadManager<SemType>(
      {{2, 2, 2, normalThreadsCount, 2}}, enableTaskStats);
}

} // namespace concurrency
} // namespace thrift
} // namespace apache
