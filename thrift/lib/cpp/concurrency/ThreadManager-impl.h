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
#ifndef THREADMANAGERIMPL_H
#define THREADMANAGERIMPL_H

namespace apache { namespace thrift { namespace concurrency {

using std::shared_ptr;
using std::make_shared;
using std::dynamic_pointer_cast;
using std::unique_ptr;
using apache::thrift::async::RequestContext;

class ThreadManager::Task {

 public:
  enum STATE {
    WAITING,
    EXECUTING,
    CANCELLED,
    COMPLETE
  };

  Task(shared_ptr<Runnable> runnable,
       int64_t expirationMs = 0,
       bool enableStats = false)
    : runnable_(runnable),
      expireTimeUs_(0),
      entryTimeUs_(0),
      context_(RequestContext::saveContext()) {
    if (enableStats || expirationMs > 0) {
      int64_t nowUs(Util::currentTimeUsec());
      if (enableStats) {
        entryTimeUs_ = nowUs;
      }
      if (expirationMs > 0) {
        expireTimeUs_ = nowUs + (expirationMs * Util::US_PER_MS);
      }
    }
  }

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

  int64_t getExpireTime() const {
    return expireTimeUs_;
  }

  int64_t getEntryTime() const {
    return entryTimeUs_;
  }

 private:
  shared_ptr<Runnable> runnable_;
  int64_t expireTimeUs_;
  int64_t entryTimeUs_;
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
    waitingTimeUs_(0),
    executingTimeUs_(0),
    numTasks_(0),
    state_(ThreadManager::UNINITIALIZED),
    tasks_(maxQueueLen == 0 ?
           ThreadManager::DEFAULT_MAX_QUEUE_SIZE : maxQueueLen),
    monitor_(&mutex_),
    stopNotificationThread_(false),
    maxMonitor_(&mutex_),
    deadWorkerMonitor_(&mutex_),
    deadWorkers_(),
    namePrefix_(""),
    namePrefixCounter_(0) {
      RequestContext::getStaticContext();
  }

  ~ImplT() { stop(); }

  void start();

  void stop() { stopImpl(false); }

  void join() { stopImpl(true); }

  const ThreadManager::STATE state() const {
    return state_;
  }

  shared_ptr<ThreadFactory> threadFactory() const {
    Guard g(mutex_);
    return threadFactory_;
  }

  void threadFactory(shared_ptr<ThreadFactory> value) {
    Guard g(mutex_);
    threadFactory_ = value;
  }

  void setNamePrefix(const std::string& name) {
    Guard g(mutex_);
    namePrefix_ = name;
  }

  void addWorker(size_t value);

  void removeWorker(size_t value);

  size_t idleWorkerCount() const {
    return idleCount_;
  }

  size_t workerCount() const {
    return workerCount_;
  }

  size_t pendingTaskCount() const {
    return tasks_.size();
  }

  size_t totalTaskCount() const {
    return totalTaskCount_;
  }

  size_t pendingTaskCountMax() const {
    return pendingTaskCountMax_;
  }

  size_t expiredTaskCount() {
    Guard g(mutex_);
    size_t result = expiredCount_;
    expiredCount_ = 0;
    return result;
  }

  bool canSleep();

  void add(shared_ptr<Runnable> value, int64_t timeout, int64_t expiration);

  void remove(shared_ptr<Runnable> task);

  shared_ptr<Runnable> removeNextPending();

  void setExpireCallback(ExpireCallback expireCallback);
  void setThreadInitCallback(InitCallback initCallback) {
    initCallback_ = initCallback;
  }

  void getStats(int64_t& waitTimeUs, int64_t& runTimeUs, int64_t maxItems);

  // Methods to be invoked by workers
  void workerStarted(Worker<SemType>* worker);
  void workerExiting(Worker<SemType>* worker);
  void reportTaskStats(int64_t waitTimeUs, int64_t runTimeUs);
  Task* waitOnTask();
  void taskExpired(Task* task);

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
  int64_t waitingTimeUs_;
  int64_t executingTimeUs_;
  int64_t numTasks_;

  ExpireCallback expireCallback_;
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
  PosixSemaphore notifySem_;
  std::atomic<bool> stopNotificationThread_;
  std::shared_ptr<Thread> notificationThread_;
  // maxMonitor_ is signalled when the number of pending tasks drops below
  // pendingTaskCountMax_
  Monitor maxMonitor_;
  // deadWorkerMonitor_ is signaled whenever a worker thread exits
  Monitor deadWorkerMonitor_;
  std::deque<shared_ptr<Thread> > deadWorkers_;

  std::map<const Thread::id_t, shared_ptr<Thread> > idMap_;
  std::string namePrefix_;
  uint32_t namePrefixCounter_;

  class NotificationWorker;
};


}}}

#endif
