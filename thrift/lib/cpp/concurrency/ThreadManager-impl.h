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

} // namespace concurrency
} // namespace thrift
} // namespace apache
