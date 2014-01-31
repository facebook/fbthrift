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

#include "thrift/lib/cpp/concurrency/ThreadManager.h"

#include "thrift/lib/cpp/concurrency/Exception.h"
#include "thrift/lib/cpp/concurrency/Monitor.h"
#include "thrift/lib/cpp/concurrency/Thread.h"
#include "thrift/lib/cpp/concurrency/Util.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "folly/Conv.h"
#include "ThreadManager.h"
#include "PosixThreadFactory.h"
#include "folly/MPMCQueue.h"
#include "thrift/lib/cpp/async/Request.h"

#include <memory>

#include <assert.h>
#include <queue>
#include <set>
#include <atomic>

#if defined(DEBUG)
#include <iostream>
#endif //defined(DEBUG)

#include "thrift/lib/cpp/concurrency/ThreadManager-impl.h"

namespace apache { namespace thrift { namespace concurrency {

using std::shared_ptr;
using std::make_shared;
using std::dynamic_pointer_cast;
using std::unique_ptr;
using apache::thrift::async::RequestContext;

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

  ~Worker() {}

  /**
   * Worker entry point
   *
   * As long as worker thread is running, pull tasks off the task queue and
   * execute.
   */
  void run() {
    // Inform our manager that we are starting
    manager_->workerStarted(this);

    while (true) {
      // Wait for a task to run
      Task* task = manager_->waitOnTask();

      // A nullptr task means that this thread is supposed to exit
      if (!task) {
        manager_->workerExiting(this);
        return;
      }

      // Getting the current time is moderately expensive,
      // so only get the time if we actually need it.
      int64_t startTimeUs = 0;
      if (task->getExpireTime() || task->getEntryTime()) {
        startTimeUs = Util::currentTimeUsec();
      }

      // Check if the task is expired
      if (task->getExpireTime() != 0 &&
          task->getExpireTime() <= startTimeUs) {
        manager_->taskExpired(task);
        delete task;
        continue;
      }

      try {
        task->run();
      } catch(const std::exception& ex) {
        T_ERROR("ThreadManager: worker task threw unhandled "
                "%s exception: %s", typeid(ex).name(), ex.what());
      } catch(...) {
        T_ERROR("ThreadManager: worker task threw unhandled "
                "non-exception object");
      }

      if (task->getEntryTime() != 0) {
        int64_t endTimeUs = Util::currentTimeUsec();
        manager_->reportTaskStats(startTimeUs - task->getEntryTime(),
                                  endTimeUs - startTimeUs);
      }
      delete task;
    }
  }

 private:
  ThreadManager::ImplT<SemType>* manager_;
};

template <typename SemType>
void ThreadManager::ImplT<SemType>::addWorker(size_t value) {
  std::vector<shared_ptr<Thread> > newThreads;
  newThreads.reserve(value);
  for (size_t ix = 0; ix < value; ix++) {
    auto worker = make_shared<Worker<SemType>>(this);
    newThreads.push_back(threadFactory_->newThread(worker,
                                                   ThreadFactory::ATTACHED));
  }

  {
    Guard g(mutex_);

    if (state_ != STARTED) {
      throw IllegalStateException("ThreadManager::addWorker(): "
                                  "ThreadManager not running");
    }

    workerCount_ += value;
    intendedWorkerCount_ += value;
    idleCount_ += value;
  }

  // Start all of the threads.
  for (std::vector<shared_ptr<Thread> >::iterator ix = newThreads.begin();
       ix != newThreads.end();
       ix++) {
    (*ix)->start();
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
      thread->setName(folly::to<std::string>(namePrefix_,
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
  assert(numErased == 1);

  --workerCount_;
  --totalTaskCount_;
  deadWorkers_.push_back(thread);
  deadWorkerMonitor_.notify();
}

template <typename SemType>
class ThreadManager::ImplT<SemType>::NotificationWorker : public Runnable {
public:
  NotificationWorker(PosixSemaphore& sem, SemType& mon, std::atomic<bool>& stop)
    : sem_(sem), mon_(mon), stop_(stop) {}
  void run() {
    while (!stop_) {
      sem_.wait();
      mon_.post();
    }
  }
private:
  PosixSemaphore& sem_;
  SemType& mon_;
  std::atomic<bool>& stop_;
};

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
    auto notificationWorker = make_shared<NotificationWorker>(
        notifySem_, waitSem_, stopNotificationThread_);
    notificationThread_ = threadFactory_->newThread(notificationWorker,
                                                    ThreadFactory::ATTACHED);
    notificationThread_->start();
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
      Task* task;
      while (tasks_.read(task)) {
        delete task;
      }
    }
    stopNotificationThread_ = true;
    notifySem_.post();
    state_ = ThreadManager::STOPPED;
    monitor_.notifyAll();
    g.release();
    notificationThread_->join();
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
        T_ERROR("ThreadManager: Can't remove worker. Increase maxQueueLen?");
        bad++;
        continue;
      }
      ++totalTaskCount_;
    }
    monitor_.notifyAll();
    for (int n = 0; n < value; ++n) {
      waitSem_.post();
    }
    intendedWorkerCount_ += bad;
    value -= bad;
  } else {
    // Ask threads to exit ASAP
    workersToStop_ += value;
    monitor_.notifyAll();
    for (int n = 0; n < value; ++n) {
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
                              int64_t expiration) {
  if (state_ != ThreadManager::STARTED) {
    throw IllegalStateException("ThreadManager::Impl::add ThreadManager "
                                "not started");
  }

  if (pendingTaskCountMax_ > 0 && (tasks_.size() >= pendingTaskCountMax_)) {
    Guard g(mutex_, timeout);

    if (!g) {
      throw TimedOutException();
    }
    if (canSleep() && timeout >= 0) {
      while (pendingTaskCountMax_ > 0
             && tasks_.size() >= pendingTaskCountMax_) {
        // This is thread safe because the mutex is shared between monitors.
        maxMonitor_.wait(timeout);
      }
    } else {
      throw TooManyPendingTasksException();
    }
  }

  // If an idle thread is available notify it, otherwise all worker threads are
  // running and will get around to this task in time.
  Task* task = new ThreadManager::Task(value, expiration, enableTaskStats_);
  if (!tasks_.write(task)) {
    T_ERROR("ThreadManager: Failed to enqueue item. Increase maxQueueLen?");
    delete task;
    throw TooManyPendingTasksException();
  }

  ++totalTaskCount_;

  if (idleCount_ > 0) {
    // If an idle thread is available notify it, otherwise all worker threads
    // are running and will get around to this task in time.
    notifySem_.post();
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::remove(shared_ptr<Runnable> task) {
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

  ThreadManager::Task* task;
  if (tasks_.read(task)) {
    std::shared_ptr<Runnable> r = task->getRunnable();
    delete task;
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
ThreadManager::Task* ThreadManager::ImplT<SemType>::waitOnTask() {
  if (shouldStop()) {
    return nullptr;
  }

  ThreadManager::Task* task;

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
  if (pendingTaskCountMax_ != 0 && tasks_.size() < pendingTaskCountMax_) {
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
void ThreadManager::ImplT<SemType>::taskExpired(Task* task) {
  ExpireCallback expireCallback;
  {
    Guard g(mutex_);
    expiredCount_++;
    expireCallback = expireCallback_;
  }

  if (expireCallback) {
    // Expired callback should _not_ be called holding mutex_
    expireCallback(task->getRunnable());
  }
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::setExpireCallback(ExpireCallback expireCallback) {
  expireCallback_ = expireCallback;
}

template <typename SemType>
void ThreadManager::ImplT<SemType>::getStats(int64_t& waitTimeUs, int64_t& runTimeUs,
                                   int64_t maxItems) {
  Guard g(mutex_);
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
void ThreadManager::ImplT<SemType>::reportTaskStats(int64_t waitTimeUs,
                                          int64_t runTimeUs) {
  Guard g(mutex_);
  waitingTimeUs_ += waitTimeUs;
  executingTimeUs_ += runTimeUs;
  ++numTasks_;
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

  void start() {
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
  PriorityImplT(std::array<
               std::pair<shared_ptr<ThreadFactory>, size_t>,
               N_PRIORITIES> factories,
               bool enableTaskStats = false) {
    for (int i = 0; i < N_PRIORITIES; i++) {
      unique_ptr<ThreadManager> m(new ThreadManager::ImplT<SemType>(0, enableTaskStats));
      m->threadFactory(factories[i].first);
      managers_[i] = std::move(m);
      counts_[i] = factories[i].second;
    }
  }

  virtual void start() {
    Guard g(mutex_);
    for (int i = 0; i < N_PRIORITIES; i++) {
      if (managers_[i]->state() == STARTED) {
        continue;
      }
      managers_[i]->start();
      managers_[i]->addWorker(counts_[i]);
    }
  }

  virtual void stop() {
    Guard g(mutex_);
    for (auto& m : managers_) {
      m->stop();
    }
  }

  virtual void join() {
    Guard g(mutex_);
    for (auto& m : managers_) {
      m->join();
    }
  }

  void setNamePrefix(const std::string& name) {
    for (auto& m : managers_) {
      m->setNamePrefix(name);
    }
  }

  virtual void addWorker(size_t value) {
    addWorker(NORMAL, value);
  }

  virtual void removeWorker(size_t value) {
    removeWorker(NORMAL, value);
  }

  virtual void addWorker(PRIORITY priority, size_t value) {
    managers_[priority]->addWorker(value);
  }

  virtual void removeWorker(PRIORITY priority, size_t value) {
    managers_[priority]->removeWorker(value);
  }

  virtual const STATE state() const {
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

  virtual std::shared_ptr<ThreadFactory> threadFactory() const {
    throw IllegalStateException("Not implemented");
    return std::shared_ptr<ThreadFactory>();
  }

  virtual void threadFactory(std::shared_ptr<ThreadFactory> value) {
    throw IllegalStateException("Not implemented");
  }

  virtual void add(std::shared_ptr<Runnable>task,
                   int64_t timeout=0LL,
                   int64_t expiration=0LL) {
    PriorityRunnable* p = dynamic_cast<PriorityRunnable*>(task.get());
    PRIORITY prio = p ? p->getPriority() : NORMAL;
    add(prio, task, timeout, expiration);
  }

  virtual void add(PRIORITY priority,
                   std::shared_ptr<Runnable>task,
                   int64_t timeout=0LL,
                   int64_t expiration=0LL) {
    managers_[priority]->add(task, timeout, expiration);
  }

  template <typename T>
  size_t sum(T method) const {
    size_t count = 0;
    for (const auto& m : managers_) {
      count += ((*m).*method)();
    }
    return count;
  }

  virtual size_t idleWorkerCount() const {
    return sum(&ThreadManager::idleWorkerCount);
  }

  virtual size_t workerCount() const {
    return sum(&ThreadManager::workerCount);
  }

  virtual size_t pendingTaskCount() const {
    return sum(&ThreadManager::pendingTaskCount);
  }

  virtual size_t totalTaskCount() const {
    return sum(&ThreadManager::totalTaskCount);
  }

  virtual size_t pendingTaskCountMax() const {
    throw IllegalStateException("Not implemented");
    return 0;
  }

  virtual size_t expiredTaskCount() {
    return sum(&ThreadManager::expiredTaskCount);
  }

  virtual void remove(std::shared_ptr<Runnable> task) {
    throw IllegalStateException("Not implemented");
  }

  virtual std::shared_ptr<Runnable> removeNextPending() {
    throw IllegalStateException("Not implemented");
    return std::shared_ptr<Runnable>();
  }

  virtual void setExpireCallback(ExpireCallback expireCallback) {
    throw IllegalStateException("Not implemented");
  }

  virtual void setThreadInitCallback(InitCallback initCallback) {
    throw IllegalStateException("Not implemented");
  }

private:
  unique_ptr<ThreadManager> managers_[N_PRIORITIES];
  size_t counts_[N_PRIORITIES];
  Mutex mutex_;
};

static inline shared_ptr<ThreadFactory> Factory(PosixThreadFactory::PRIORITY prio) {
  return make_shared<PosixThreadFactory>(PosixThreadFactory::OTHER, prio);
}

template <typename SemType>
shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    std::array<size_t,N_PRIORITIES> counts,
    bool enableTaskStats) {

  assert(N_PRIORITIES == 5);
  // Note that priorities for HIGH and IMPORTANT are the same, the difference
  // is in the number of threads

  std::array<std::pair<shared_ptr<ThreadFactory>, size_t>,
             N_PRIORITIES> factories
  {{
      {Factory(PosixThreadFactory::HIGHER), counts[0]},   // HIGH_IMPORTANT
      {Factory(PosixThreadFactory::HIGH),   counts[1]},   // HIGH
      {Factory(PosixThreadFactory::HIGH),   counts[2]},   // IMPORTANT
      {Factory(PosixThreadFactory::NORMAL), counts[3]},   // NORMAL
      {Factory(PosixThreadFactory::LOW),    counts[4]}   // BEST_EFFORT
  }};

  return shared_ptr<PriorityThreadManager>(
    new PriorityThreadManager::PriorityImplT<SemType>(factories, enableTaskStats));
}

template <typename SemType>
shared_ptr<PriorityThreadManager>
PriorityThreadManager::newPriorityThreadManager(
    size_t normalThreadsCount,
    bool enableTaskStats) {
  return newPriorityThreadManager<SemType>({{2, 2, 2, normalThreadsCount, 2}},
                                  enableTaskStats);
}

}}} // apache::thrift::concurrency
