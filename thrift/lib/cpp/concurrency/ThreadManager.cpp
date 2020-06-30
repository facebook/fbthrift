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
#include <chrono>
#include <memory>
#include <queue>
#include <set>
#include <string>

#include <glog/logging.h>

#include <folly/Conv.h>
#include <folly/portability/GFlags.h>
#include <folly/tracing/StaticTracepoint.h>

#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager-impl.h>

DEFINE_bool(codel_enabled, false, "Enable codel queue timeout algorithm");

namespace apache {
namespace thrift {
namespace concurrency {

using folly::RequestContext;
using std::dynamic_pointer_cast;
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;

folly::SharedMutex ThreadManager::observerLock_;
std::shared_ptr<ThreadManager::Observer> ThreadManager::observer_;

shared_ptr<ThreadManager> ThreadManager::newThreadManager() {
  return make_shared<ThreadManager::Impl>();
}

void ThreadManager::setObserver(
    std::shared_ptr<ThreadManager::Observer> observer) {
  {
    folly::SharedMutex::WriteHolder g(observerLock_);
    observer_.swap(observer);
  }
}

void ThreadManager::traceTask(
    const std::string& namePrefix,
    intptr_t rootContextId,
    std::chrono::steady_clock::time_point queueBegin,
    std::chrono::steady_clock::duration waitTime,
    std::chrono::steady_clock::duration runTime) {
  // Times in this USDT use granularity of std::chrono::steady_clock::duration,
  // which is platform dependent. On Facebook servers, the granularity is
  // nanoseconds. We explicitly do not perform any unit conversions to avoid
  // unneccessary costs and leave it to consumers of this data to know what
  // effective clock resolution is.
  FOLLY_SDT(
      thrift,
      thread_manager_task_stats,
      namePrefix.c_str(),
      rootContextId,
      queueBegin.time_since_epoch().count(),
      waitTime.count(),
      runTime.count());
}

} // namespace concurrency
} // namespace thrift
} // namespace apache
