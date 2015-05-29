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

#ifndef THREADMANAGERIMPL_H
#define THREADMANAGERIMPL_H

#include <folly/SmallLocks.h>
#include <folly/MPMCQueue.h>
#include <folly/LifoSem.h>
#include <thrift/lib/cpp/async/Request.h>
#include <thrift/lib/cpp/concurrency/Monitor.h>
#include <deque>
namespace apache { namespace thrift { namespace concurrency {

using std::shared_ptr;
using std::make_shared;
using folly::wangle::Codel;
using std::dynamic_pointer_cast;
using std::unique_ptr;
using async::RequestContext;

class ThreadManager::Task {
 public:
  enum STATE {
    WAITING,
    EXECUTING,
    CANCELLED,
    COMPLETE
  };

  Task(shared_ptr<Runnable> runnable,
       const std::chrono::milliseconds& expiration)
    : runnable_(std::move(runnable))
    , queueBeginTime_(SystemClock::now())
    , expireTime_(expiration > std::chrono::milliseconds::zero() ?
                  queueBeginTime_ + expiration : SystemClockTimePoint())
    , context_(RequestContext::saveContext()) {}

  ~Task() {}

  void run() {
    auto old_ctx =
      RequestContext::setContext(context_);
    runnable_->run();
    RequestContext::setContext(old_ctx);
  }

  const shared_ptr<Runnable>& getRunnable() const {
    return runnable_;
  }

  SystemClockTimePoint getExpireTime() const {
    return expireTime_;
  }

  SystemClockTimePoint getQueueBeginTime() const {
    return queueBeginTime_;
  }

  bool canExpire() const {
    return expireTime_ != SystemClockTimePoint();
  }

  bool statsEnabled() const {
    return queueBeginTime_ != SystemClockTimePoint();
  }

 private:
  shared_ptr<Runnable> runnable_;
  SystemClockTimePoint queueBeginTime_;
  SystemClockTimePoint expireTime_;
  std::shared_ptr<RequestContext> context_;
};

template <typename SemType>
class ThreadManager::ImplT : public ThreadManager  {

  template <typename SemTypeT>
  class Worker;

 public:
  ImplT(size_t pendingTaskCountMaxArg = 0,
       bool enableTaskStats = false,
       size_t maxQueueLen = 0) :
    workerCount_(0),
    intendedWorkerCount_(0),
    idleCount_(0),
    totalTaskCount_(0),
    pendingTaskCountMax_(pendingTaskCountMaxArg),
    expiredCount_(0),
    workersToStop_(0),
    enableTaskStats_(enableTaskStats),
    statsLock_{0},
    waitingTimeUs_(0),
    executingTimeUs_(0),
    numTasks_(0),
    state_(ThreadManager::UNINITIALIZED),
    tasks_(maxQueueLen == 0
             ? (pendingTaskCountMax_ > 0
                  // TODO(philipp): Fix synchronization issues between "pending"
                  // and queuing logic.  For now, if pendingTaskCountMax_ > 0,
                  // let's have more room in the queue to avoid issues in most
                  // of cases.
                  ? pendingTaskCountMax_ + 1024
                  : ThreadManager::DEFAULT_MAX_QUEUE_SIZE)
             : maxQueueLen),
    monitor_(&mutex_),
    maxMonitor_(&mutex_),
    deadWorkerMonitor_(&mutex_),
    deadWorkers_(),
    namePrefix_(""),
    namePrefixCounter_(0),
    codelEnabled_(false || FLAGS_codel_enabled) {
      RequestContext::get();
  }

  ~ImplT() { stop(); }

  void start() override;

  void stop() override { stopImpl(false); }

  void join() override { stopImpl(true); }

  ThreadManager::STATE state() const override {
    return state_;
  }

  shared_ptr<ThreadFactory> threadFactory() const override {
    Guard g(mutex_);
    return threadFactory_;
  }

  void threadFactory(shared_ptr<ThreadFactory> value) override {
    Guard g(mutex_);
    threadFactory_ = value;
  }

  std::string getNamePrefix() const override {
    Guard g(mutex_);
    return namePrefix_;
  }

  void setNamePrefix(const std::string& name) override {
    Guard g(mutex_);
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

  size_t totalTaskCount() const override {
    return totalTaskCount_;
  }

  size_t pendingTaskCountMax() const override {
    return pendingTaskCountMax_;
  }

  size_t expiredTaskCount() override {
    Guard g(mutex_);
    size_t result = expiredCount_;
    expiredCount_ = 0;
    return result;
  }

  bool canSleep();

  void add(shared_ptr<Runnable> value, int64_t timeout, int64_t expiration,
           bool cancellable, bool numa) override;

  /**
   * Implements folly::Executor::add()
   */
  void add(folly::Func f) override {
    add(FunctionRunner::create(std::move(f)), 0LL, 0LL, false, false);
  }

  void remove(shared_ptr<Runnable> task) override;

  shared_ptr<Runnable> removeNextPending() override;

  void setExpireCallback(ExpireCallback expireCallback) override;
  void setCodelCallback(ExpireCallback expireCallback) override;
  void setThreadInitCallback(InitCallback initCallback) override {
    initCallback_ = initCallback;
  }

  void getStats(int64_t& waitTimeUs, int64_t& runTimeUs, int64_t maxItems)
    override;
  void enableCodel(bool) override;
  Codel* getCodel() override;

  // Methods to be invoked by workers
  void workerStarted(Worker<SemType>* worker);
  void workerExiting(Worker<SemType>* worker);
  void reportTaskStats(const SystemClockTimePoint& queueBegin,
                       const SystemClockTimePoint& workBegin,
                       const SystemClockTimePoint& workEnd);
  Task* waitOnTask();
  void taskExpired(Task* task);

  Codel codel_;

 private:
  void stopImpl(bool joinArg);
  void removeWorkerImpl(size_t value, bool afterTasks = false);
  void maybeNotifyMaxMonitor(bool shouldLock);
  bool shouldStop();

  size_t workerCount_;
  // intendedWorkerCount_ tracks the number of worker threads that we currently
  // want to have.  This may be different from workerCount_ while we are
  // attempting to remove workers:  While we are waiting for the workers to
  // exit, intendedWorkerCount_ will be smaller than workerCount_.
  size_t intendedWorkerCount_;
  std::atomic<size_t> idleCount_;
  std::atomic<size_t> totalTaskCount_;
  const size_t pendingTaskCountMax_;
  size_t expiredCount_;
  std::atomic<int> workersToStop_;

  const bool enableTaskStats_;
  folly::MicroSpinLock statsLock_;
  int64_t waitingTimeUs_;
  int64_t executingTimeUs_;
  int64_t numTasks_;

  ExpireCallback expireCallback_;
  ExpireCallback codelCallback_;
  InitCallback initCallback_;

  ThreadManager::STATE state_;
  shared_ptr<ThreadFactory> threadFactory_;

  folly::MPMCQueue<Task*> tasks_;

  Mutex mutex_;
  // monitor_ is signaled on any of the following events:
  // - a new task is added to the task queue
  // - state_ changes
  Monitor monitor_;
  SemType waitSem_;
  // maxMonitor_ is signalled when the number of pending tasks drops below
  // pendingTaskCountMax_
  Monitor maxMonitor_;
  // deadWorkerMonitor_ is signaled whenever a worker thread exits
  Monitor deadWorkerMonitor_;
  std::deque<shared_ptr<Thread> > deadWorkers_;

  std::map<const Thread::id_t, shared_ptr<Thread> > idMap_;
  std::string namePrefix_;
  uint32_t namePrefixCounter_;

  bool codelEnabled_;
};


}}}

#endif
