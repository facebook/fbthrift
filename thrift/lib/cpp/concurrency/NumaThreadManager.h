/*
 * Copyright 2014-present Facebook, Inc.
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

#pragma once

#include <memory>

#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>

namespace apache { namespace thrift { namespace concurrency {

// ThreadFactory that ties threads to NUMA nodes.
class NumaThreadFactory : public PosixThreadFactory {
 public:
  // if setNode is -1, threads will be round robin spread
  // over nodes.  Otherwise, all threads will be created on
  // setNode node.
  explicit NumaThreadFactory(int setNode = -1,
                             int stackSize
                             = PosixThreadFactory::kDefaultStackSizeMB)
      : PosixThreadFactory(
          PosixThreadFactory::kDefaultPolicy,
          PosixThreadFactory::kDefaultPriority,
          stackSize),
        setNode_(setNode) {}

  // Overridden methods to implement numa binding
  std::shared_ptr<Thread> newThread(
      const std::shared_ptr<Runnable>& runnable) const override;

  std::shared_ptr<Thread> newThread(const std::shared_ptr<Runnable>& runnable,
                                    DetachState detachState) const override;

  // Get the threadlocal describing which node
  // this thread is bound to.
  static int getNumaNode();

  // Sets the threadlocal descrbing which node this thrad
  // is bound to.  *Does not actually call numa bind*,
  // but NumaThreadManager calls will always run requests on
  // the current node.
  static void setNumaNode();

 private:
  friend class NumaRunnable;

  int setNode_{-1};
  static __thread int node_;
  static int workerNode_;
};

// ThreadManager that is NUMA-aware.  Jobs added with numaAdd()
// will be run on the same numa node.  Jobs added with add() will
// be round-robin added to numa nodes.

// The intent is that requests that share data will always be run
// on the same NUMA node, decreasing latency.

class NumaThreadManager : public ThreadManager {
 public:
  typedef apache::thrift::concurrency::PRIORITY PRIORITY;

  virtual bool tryAdd(PRIORITY priority, std::shared_ptr<Runnable> task);
  virtual void add(PRIORITY priority,
                   std::shared_ptr<Runnable> task,
                   int64_t timeout = 0,
                   int64_t expiration = 0,
                   bool cancellable = false,
                   bool numa = false);

  bool tryAdd(std::shared_ptr<Runnable> task) override;
  void add(std::shared_ptr<Runnable> task,
           int64_t timeout = 0,
           int64_t expiration = 0,
           bool cancellable = false,
           bool numa = false) override;

  /**
   * Implements folly::Executor::add()
   */
  void add(folly::Func f) override {
    add(FunctionRunner::create(std::move(f)), 0LL, 0LL, false, true);
  }

  explicit NumaThreadManager(size_t normalThreadsCount
                             = sysconf(_SC_NPROCESSORS_ONLN),
                             bool enableTaskStats = false,
                             size_t maxQueueLen = 0,
                             int threadStackSizeMB
                             = PosixThreadFactory::kDefaultStackSizeMB);

  void start() override {
    for (auto& manager : managers_) {
      manager->start();
    }
  }

  void stop() override {
    for (auto& manager : managers_) {
      manager->stop();
    }
  }

  void join() override {
    for (auto& manager : managers_) {
      manager->join();
    }
  }

  void threadFactory(std::shared_ptr<ThreadFactory>) override {
    // The thread factories must be node-specific.
    throw IllegalStateException("Setting threadFactory not implemented");
  }

  std::shared_ptr<ThreadFactory> threadFactory() const override {
    // Since each manager has its own node-local thread factory,
    // there is no reasonable factory to return.
    throw IllegalStateException("Getting threadFactory not implemented");
    return nullptr;
  }

  STATE state() const override {
    // States *should* all be the same
    return managers_[0]->state();
  }

  std::string getNamePrefix() const override {
    return managers_[0]->getNamePrefix();
  }

  void setNamePrefix(const std::string& prefix) override {
    for (auto& manager : managers_) {
      manager->setNamePrefix(prefix);
    }
  }

  void addWorker(size_t t) override;

  void removeWorker(size_t t) override;

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

  void clearPending() override {
    throw IllegalStateException("Not implemented");
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

  void enableCodel(bool codel) override {
    for (auto& manager : managers_) {
      manager->enableCodel(codel);
    }
  }

  folly::Codel* getCodel() override {
    // They *should* be roughtly the same, just return one for now.
    return managers_[0]->getCodel();
  }

 private:
  template <typename T>
  size_t sum(T method) const {
    size_t count = 0;
    for (const auto& m : managers_) {
      count += ((*m).*method)();
    }
    return count;
  }

  std::vector<std::shared_ptr<PriorityThreadManager>> managers_;
  int node_{0};
  int workerNode_{0};
};

}}}
