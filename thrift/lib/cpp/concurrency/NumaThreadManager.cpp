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

#include "NumaThreadManager.h"

#include <glog/logging.h>
#include <numa.h>
#ifdef LIBNUMA_API_VERSION
#include <numacompat1.h>
#endif

#include <folly/Memory.h>
#include <folly/ScopeGuard.h>

DEFINE_bool(thrift_numa_enabled, false, "Enable NumaThreadManager in thrift");

namespace apache { namespace thrift { namespace concurrency {

class NumaContextData : public folly::RequestData {
 public:
  explicit NumaContextData(int n) : node(n) {}
  int node{-1};
  static std::string ContextDataVal;
};

// Used with NumaThreadFactory
class NumaRunnable : public Runnable {
 public:

  explicit NumaRunnable(int setNode, std::shared_ptr<Runnable> runnable)
      : setNode_(setNode)
      , runnable_(runnable) {}

  void run();

  virtual std::shared_ptr<Thread> thread() {
    return runnable_->thread();
  }

  virtual void thread(std::shared_ptr<Thread> value) {
    runnable_->thread(value);
  }

 private:
  int setNode_;
  std::shared_ptr<Runnable> runnable_;
};

void NumaRunnable::run() {

  if (numa_available() >= 0 && FLAGS_thrift_numa_enabled) {
    static std::atomic<int> counter;
    int node = counter++ % (numa_max_node() + 1);

    // If a node is given, use the given node instead of interleaving.
    // Used to make threadpools that are local to one node.
    if (setNode_ != -1) {
      node = setNode_;
    }

    // Bind both cpu and mem to node
    nodemask_t nodes;
    nodemask_zero(&nodes);
    nodemask_set(&nodes, node);
    numa_bind(&nodes);

    // Bind pthread stack to node.
    pthread_attr_t thread_attrs;
    void *stack_addr;
    size_t stack_size;
    size_t guard_size;

    SCOPE_EXIT{     pthread_attr_destroy(&thread_attrs); };

    if (pthread_getattr_np(pthread_self(), &thread_attrs) != 0) {
      LOG(ERROR) << "Failed to get attributes of the current thread.";
    }

    if (pthread_attr_getstack(&thread_attrs, &stack_addr, &stack_size) != 0) {
      LOG(ERROR) << "Failed to get stack size for the current thread.";
    }

    if (pthread_attr_getguardsize(&thread_attrs, &guard_size) != 0) {
      LOG(ERROR) << "Failed to get stack guard size for the current thread.";
    }

    static const size_t page_sz = sysconf(_SC_PAGESIZE);

    if (((uint64_t)stack_addr) % page_sz == 0) {
      numa_tonode_memory(stack_addr, stack_size, node);
    } else {
      LOG(ERROR) << "Thread stack is not page aligned. "
        "Cannot bind to NUMA node.";
    }

    // Set the thread-local node to the node we are running on.
    NumaThreadFactory::node_ = node;
  }

  runnable_->run();
}

std::string NumaContextData::ContextDataVal = "numa";

void NumaThreadFactory::setNumaNode() {
  if (node_ != -1) {
    RequestContext::get()->setContextData(
      NumaContextData::ContextDataVal,
      folly::make_unique<NumaContextData>(node_));
  }
}

__thread int NumaThreadFactory::node_{-1};
int NumaThreadFactory::workerNode_{0};

int NumaThreadFactory::getNumaNode() {
  auto data = RequestContext::get()->getContextData(
    NumaContextData::ContextDataVal);
  if (data) {
    return static_cast<NumaContextData*>(data)->node;
  } else {
    auto node = node_;
    if (node == -1) {
      node = workerNode_++ % (numa_max_node() + 1);
    }

    return node;
  }
}

std::shared_ptr<Thread> NumaThreadFactory::newThread(
  const std::shared_ptr<Runnable>& runnable) const {
  return PosixThreadFactory::newThread(
    std::make_shared<NumaRunnable>(setNode_, runnable));
}

std::shared_ptr<Thread> NumaThreadFactory::newThread(
  const std::shared_ptr<Runnable>& runnable,
  DetachState detachState) const {
  return PosixThreadFactory::newThread(
    std::make_shared<NumaRunnable>(setNode_, runnable),
    detachState);
}

NumaThreadManager::NumaThreadManager(size_t normalThreadsCount,
                                     bool enableTaskStats,
                                     size_t maxQueueLen) {
  int nodes = 1;
  if (numa_available() >= 0 && FLAGS_thrift_numa_enabled) {
    nodes = (numa_max_node() + 1);
  }
  for (int i = 0; i < nodes; i++) {
    auto factory = std::make_shared<NumaThreadFactory>(i);
    // Choose the number of threads: Round up, so we don't end up
    // with any 0-thread managers.
    size_t threads = (size_t)
      ((((double)normalThreadsCount) / (nodes - i)) + .5);
    normalThreadsCount -= threads;
    managers_.push_back(PriorityThreadManager::newPriorityThreadManager(
                          {{0, 0, 1, threads, 0}},
                          enableTaskStats,
                          maxQueueLen));
    managers_[managers_.size() - 1]->threadFactory(factory);
    // If we've allocated all the threads, escape.  This may
    // mean we don't use all the numa nodes.
    if (normalThreadsCount <= 0) {
      break;
    }
  }
  workerNode_ = managers_.size() - 1;
}

void NumaThreadManager::add(PRIORITY priority,
                            std::shared_ptr<Runnable>task,
                            int64_t timeout,
                            int64_t expiration,
                            bool cancellable,
                            bool numa) {
  if (numa && managers_.size() > 1) {
    auto node = NumaThreadFactory::getNumaNode() % managers_.size();
    managers_[node]->add(
      priority, task, timeout, expiration, cancellable, false);
  } else {
    managers_[node_++ % managers_.size()]->add(
      priority, task, timeout, expiration, cancellable, false);
  }
}

void NumaThreadManager::add(
  std::shared_ptr<Runnable>task,
  int64_t timeout,
  int64_t expiration,
  bool cancellable,
  bool numa) {

  PriorityRunnable* p = dynamic_cast<PriorityRunnable*>(task.get());
  PRIORITY prio = p ? p->getPriority() : NORMAL;
  add(prio, task, timeout, expiration, cancellable, numa);
}

void NumaThreadManager::addWorker(size_t t) {
  for (size_t i = 0; i < t; i++) {
    managers_[workerNode_++ % managers_.size()]->addWorker(1);
  }
}

void NumaThreadManager::removeWorker(size_t t) {
  for (size_t i = 0; i < t; i++) {
    managers_[workerNode_-- % managers_.size()]->removeWorker(1);
  }
}

}}}
