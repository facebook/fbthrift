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
#include "ThreadManager.h"

#include <thrift/lib/cpp/concurrency/ThreadManager-impl.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>

#include <folly/Conv.h>
#include <folly/Logging.h>
#include <folly/portability/GFlags.h>

#include <assert.h>
#include <atomic>
#include <memory>
#include <queue>
#include <set>

#if defined(DEBUG)
#include <iostream>
#endif //defined(DEBUG)

DEFINE_bool(codel_enabled, false, "Enable codel queue timeout algorithm");

namespace apache { namespace thrift { namespace concurrency {

using std::shared_ptr;
using std::make_shared;
using std::dynamic_pointer_cast;
using std::unique_ptr;
using folly::RequestContext;

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
}}} // apache::thrift::concurrency
