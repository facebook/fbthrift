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

// IWYU pragma: private, include "thrift/lib/cpp/concurrency/ThreadManager.h"

#pragma once

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

#include <folly/DefaultKeepAliveExecutor.h>
#include <folly/MPMCQueue.h>
#include <folly/ThreadLocal.h>
#include <folly/concurrency/PriorityUnboundedQueueSet.h>
#include <folly/concurrency/QueueObserver.h>
#include <folly/executors/Codel.h>
#include <folly/io/async/Request.h>
#include <folly/synchronization/LifoSem.h>
#include <folly/synchronization/SmallLocks.h>

#include <thrift/lib/cpp/concurrency/Mutex.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>

namespace apache {
namespace thrift {
namespace concurrency {

namespace {
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

constexpr size_t NORMAL_PRIORITY_MINIMUM_THREADS = 1;
} // namespace

class ThreadManager::Task {
 public:
  Task(
      std::shared_ptr<Runnable> runnable,
      const std::chrono::milliseconds& expiration,
      size_t qpriority)
      : runnable_(std::move(runnable)),
        queueBeginTime_(std::chrono::steady_clock::now()),
        expireTime_(
            expiration > std::chrono::milliseconds::zero()
                ? queueBeginTime_ + expiration
                : std::chrono::steady_clock::time_point()),
        context_(folly::RequestContext::saveContext()),
        qpriority_(qpriority) {}

  ~Task() {}

  void run() {
    folly::RequestContextScopeGuard rctx(context_);
    runnable_->run();
  }

  const std::shared_ptr<Runnable>& getRunnable() const {
    return runnable_;
  }

  std::chrono::steady_clock::time_point getExpireTime() const {
    return expireTime_;
  }

  std::chrono::steady_clock::time_point getQueueBeginTime() const {
    return queueBeginTime_;
  }

  bool canExpire() const {
    return expireTime_ != std::chrono::steady_clock::time_point();
  }

  const std::shared_ptr<folly::RequestContext>& getContext() const {
    return context_;
  }

  intptr_t& queueObserverPayload() {
    return queueObserverPayload_;
  }

  size_t queuePriority() const {
    return qpriority_;
  }

 private:
  std::shared_ptr<Runnable> runnable_;
  std::chrono::steady_clock::time_point queueBeginTime_;
  std::chrono::steady_clock::time_point expireTime_;
  std::shared_ptr<folly::RequestContext> context_;
  size_t qpriority_;
  intptr_t queueObserverPayload_;
};

class ThreadManager::Impl : public ThreadManager,
                            public folly::DefaultKeepAliveExecutor {
  class Worker;

 public:
  explicit Impl(bool enableTaskStats = false, size_t numPriorities = 1)
      : workerCount_(0),
        intendedWorkerCount_(0),
        idleCount_(0),
        totalTaskCount_(0),
        expiredCount_(0),
        workersToStop_(0),
        enableTaskStats_(enableTaskStats),
        statsLock_{0},
        waitingTimeUs_(0),
        executingTimeUs_(0),
        numTasks_(0),
        state_(ThreadManager::UNINITIALIZED),
        tasks_(2 * numPriorities),
        deadWorkers_(),
        namePrefix_(""),
        namePrefixCounter_(0),
        codelEnabled_(false || FLAGS_codel_enabled) {}

  ~Impl() override {
    stop();
  }

  void start() override;

  void stop() override {
    stopImpl(false);
  }

  void join() override {
    stopImpl(true);
  }

  ThreadManager::STATE state() const override {
    return state_;
  }

  std::shared_ptr<ThreadFactory> threadFactory() const override {
    std::unique_lock<std::mutex> l(mutex_);
    return threadFactory_;
  }

  void threadFactory(std::shared_ptr<ThreadFactory> value) override {
    std::unique_lock<std::mutex> l(mutex_);
    threadFactory_ = value;
  }

  std::string getNamePrefix() const override {
    std::unique_lock<std::mutex> l(mutex_);
    return namePrefix_;
  }

  void setNamePrefix(const std::string& name) override {
    std::unique_lock<std::mutex> l(mutex_);
    namePrefix_ = name;
  }

  void addWorker(size_t value) override;

  void removeWorker(size_t value) override;

  size_t idleWorkerCount() const override {
    return idleCount_;
  }

  size_t workerCount() const override {
    return workerCount_;
  }

  size_t pendingTaskCount() const override {
    return tasks_.size();
  }

  size_t pendingUpstreamTaskCount() const override {
    size_t count = 0;
    for (size_t i = 1; i < tasks_.priorities(); i += 2) {
      count += tasks_.at_priority(i).size();
    }
    return count;
  }

  size_t totalTaskCount() const override {
    return totalTaskCount_;
  }

  size_t expiredTaskCount() override {
    std::unique_lock<std::mutex> l(mutex_);
    size_t result = expiredCount_;
    expiredCount_ = 0;
    return result;
  }

  bool canSleep();

  void add(
      std::shared_ptr<Runnable> value,
      int64_t timeout,
      int64_t expiration,
      bool cancellable) noexcept override;

  /**
   * Implements folly::Executor::add()
   */
  void add(folly::Func f) override {
    add(FunctionRunner::create(std::move(f)), 0LL, 0LL, false);
  }

  void remove(std::shared_ptr<Runnable> task) override;

  std::shared_ptr<Runnable> removeNextPending() override;

  void clearPending() override;

  void setExpireCallback(ExpireCallback expireCallback) override;
  void setCodelCallback(ExpireCallback expireCallback) override;
  void setThreadInitCallback(InitCallback initCallback) override {
    initCallback_ = initCallback;
  }

  void getStats(
      std::chrono::microseconds& waitTime,
      std::chrono::microseconds& runTime,
      int64_t maxItems) override;
  void enableCodel(bool) override;
  folly::Codel* getCodel() override;

  // Methods to be invoked by workers
  void workerStarted(Worker* worker);
  void workerExiting(Worker* worker);
  void reportTaskStats(
      const Task& task,
      const std::chrono::steady_clock::time_point& workBegin,
      const std::chrono::steady_clock::time_point& workEnd);
  std::unique_ptr<Task> waitOnTask();
  void onTaskExpired(const Task& task);

  folly::Codel codel_;

 protected:
  void add(
      size_t priority,
      std::shared_ptr<Runnable> value,
      int64_t timeout,
      int64_t expiration,
      bool cancellable) noexcept;

  // returns a string to attach to namePrefix when recording
  // stats
  virtual std::string statContext(const Task&) {
    return "";
  }

 private:
  void stopImpl(bool joinArg);
  void removeWorkerImpl(
      std::unique_lock<std::mutex>& lock,
      size_t value,
      bool afterTasks = false);
  bool shouldStop();
  void setupQueueObservers();

  size_t workerCount_;
  // intendedWorkerCount_ tracks the number of worker threads that we currently
  // want to have.  This may be different from workerCount_ while we are
  // attempting to remove workers:  While we are waiting for the workers to
  // exit, intendedWorkerCount_ will be smaller than workerCount_.
  size_t intendedWorkerCount_;
  std::atomic<size_t> idleCount_;
  std::atomic<size_t> totalTaskCount_;
  size_t expiredCount_;
  std::atomic<int> workersToStop_;

  const bool enableTaskStats_;
  folly::MicroSpinLock statsLock_;
  std::chrono::microseconds waitingTimeUs_;
  std::chrono::microseconds executingTimeUs_;
  int64_t numTasks_;

  ExpireCallback expireCallback_;
  ExpireCallback codelCallback_;
  InitCallback initCallback_;

  ThreadManager::STATE state_;
  std::shared_ptr<ThreadFactory> threadFactory_;

  folly::PriorityUMPMCQueueSet<std::unique_ptr<Task>, /* MayBlock = */ false>
      tasks_;

  mutable std::mutex mutex_;
  std::mutex stateUpdateMutex_;
  // cond_ is signaled on any of the following events:
  // - a new task is added to the task queue
  // - state_ changes
  std::condition_variable cond_;
  folly::LifoSem waitSem_;
  // deadWorkerCond_ is signaled whenever a worker thread exits
  std::condition_variable deadWorkerCond_;
  std::deque<std::shared_ptr<Thread>> deadWorkers_;

  folly::ThreadLocal<bool> isThreadManagerThread_{
      [] { return new bool(false); }};
  std::string namePrefix_;
  uint32_t namePrefixCounter_;

  bool codelEnabled_;

  folly::Optional<std::vector<std::unique_ptr<folly::QueueObserver>>>
      queueObservers_;
};

class SimpleThreadManager : public ThreadManager::Impl {
 public:
  explicit SimpleThreadManager(
      size_t workerCount = 4,
      bool enableTaskStats = false)
      : ThreadManager::Impl(enableTaskStats), workerCount_(workerCount) {}

  void start() override {
    if (this->state() == this->STARTED) {
      return;
    }
    ThreadManager::Impl::start();
    this->addWorker(workerCount_);
  }

 private:
  const size_t workerCount_;
};

class PriorityQueueThreadManager : public ThreadManager::Impl {
 public:
  typedef apache::thrift::concurrency::PRIORITY PRIORITY;
  explicit PriorityQueueThreadManager(
      size_t numThreads,
      bool enableTaskStats = false)
      : ThreadManager::Impl(enableTaskStats, N_PRIORITIES),
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

  using ThreadManager::Impl::add;

  void add(
      std::shared_ptr<Runnable> task,
      int64_t timeout = 0,
      int64_t expiration = 0,
      bool upstream = false) noexcept override {
    PriorityRunnable* p = dynamic_cast<PriorityRunnable*>(task.get());
    PRIORITY prio = p ? p->getPriority() : NORMAL;
    ThreadManager::Impl::add(
        prio, std::move(task), timeout, expiration, upstream);
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
    ThreadManager::Impl::add(
        HIGH_IMPORTANT,
        std::make_shared<PriorityFunctionRunner>(HIGH_IMPORTANT, std::move(f)),
        0,
        0,
        false);
  }

  /**
   * Implements folly::Executor::addWithPriority()
   */
  void addWithPriority(folly::Func f, int8_t priority) override {
    auto prio = translatePriority(priority);
    ThreadManager::Impl::add(
        prio,
        std::make_shared<PriorityFunctionRunner>(prio, std::move(f)),
        0,
        0,
        false);
  }

  uint8_t getNumPriorities() const override {
    return N_PRIORITIES;
  }

  void start() override {
    ThreadManager::Impl::start();
    ThreadManager::Impl::addWorker(numThreads_);
  }

  void setNamePrefix(const std::string& name) override {
    // This isn't thread safe, but neither is PriorityThreadManager's version
    // This should only be called at initialization
    ThreadManager::Impl::setNamePrefix(name);
    for (int i = 0; i < N_PRIORITIES; i++) {
      statContexts_[i] = folly::to<std::string>(name, "-pri", i);
    }
  }

  using Task = typename ThreadManager::Impl::Task;

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

static inline std::shared_ptr<ThreadFactory> Factory(
    PosixThreadFactory::PRIORITY prio) {
  return std::make_shared<PosixThreadFactory>(PosixThreadFactory::OTHER, prio);
}

class PriorityThreadManager::PriorityImpl
    : public PriorityThreadManager,
      public folly::DefaultKeepAliveExecutor {
 public:
  PriorityImpl(
      const std::array<
          std::pair<std::shared_ptr<ThreadFactory>, size_t>,
          N_PRIORITIES>& factories,
      bool enableTaskStats = false) {
    for (int i = 0; i < N_PRIORITIES; i++) {
      std::unique_ptr<ThreadManager> m(
          new ThreadManager::Impl(enableTaskStats));
      m->threadFactory(factories[i].first);
      managers_[i] = std::move(m);
      counts_[i] = factories[i].second;
    }
  }

  ~PriorityImpl() {
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
      bool upstream = false) noexcept override {
    PriorityRunnable* p = dynamic_cast<PriorityRunnable*>(task.get());
    PRIORITY prio = p ? p->getPriority() : NORMAL;
    add(prio, std::move(task), timeout, expiration, upstream);
  }

  void add(
      PRIORITY priority,
      std::shared_ptr<Runnable> task,
      int64_t timeout = 0,
      int64_t expiration = 0,
      bool upstream = false) noexcept override {
    managers_[priority]->add(std::move(task), timeout, expiration, upstream);
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

  size_t idleWorkerCount(PRIORITY priority) const override {
    return managers_[priority]->idleWorkerCount();
  }

  size_t workerCount() const override {
    return sum(&ThreadManager::workerCount);
  }

  size_t pendingTaskCount() const override {
    return sum(&ThreadManager::pendingTaskCount);
  }

  size_t pendingUpstreamTaskCount() const override {
    return sum(&ThreadManager::pendingUpstreamTaskCount);
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

  folly::Codel* getCodel() override {
    return getCodel(NORMAL);
  }

  folly::Codel* getCodel(PRIORITY priority) override {
    return managers_[priority]->getCodel();
  }

 private:
  void joinKeepAliveOnce() {
    if (!std::exchange(keepAliveJoined_, true)) {
      joinKeepAlive();
    }
  }

  std::unique_ptr<ThreadManager> managers_[N_PRIORITIES];
  size_t counts_[N_PRIORITIES];
  Mutex mutex_;
  bool keepAliveJoined_{false};
};

} // namespace concurrency
} // namespace thrift
} // namespace apache
