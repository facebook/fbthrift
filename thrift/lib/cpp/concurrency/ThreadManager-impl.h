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
        tasks_(3 * numPriorities),
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
    for (size_t i = 1; i < tasks_.priorities(); i += 3) {
      count += tasks_.at_priority(i).size();
      count += tasks_.at_priority(i + 1).size();
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
      apache::thrift::concurrency::ThreadManager::Source
          source) noexcept override;
  using ThreadManager::add;

  /**
   * Implements folly::Executor::add()
   */
  void add(folly::Func f) override {
    add(FunctionRunner::create(std::move(f)));
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
      apache::thrift::concurrency::ThreadManager::Source source) noexcept;

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

static inline std::shared_ptr<ThreadFactory> Factory(
    PosixThreadFactory::PRIORITY prio) {
  return std::make_shared<PosixThreadFactory>(PosixThreadFactory::OTHER, prio);
}

} // namespace concurrency
} // namespace thrift
} // namespace apache
