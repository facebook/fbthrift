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

folly::Synchronized<ThreadManager::ObserverFactoryWithWatchers, std::mutex>
  ThreadManager::observerFactoryWithWatchers_{};

shared_ptr<ThreadManager> ThreadManager::newThreadManager() {
  return make_shared<ThreadManager::Impl>();
}

void ThreadManager::setObserver(shared_ptr<Observer> observer) {
  setObserverFactory(
      !observer ? nullptr :
      std::make_shared<SingletonObserverFactory>(std::move(observer)));
}

void ThreadManager::setObserverFactory(shared_ptr<ObserverFactory> factory) {
  auto locked = observerFactoryWithWatchers_.lock();
  for (auto watcher : locked->watchers) {
    watcher->setObserverFromFactory(factory);
  }
  locked->factory = std::move(factory);
}

void ThreadManager::setObserverFromFactory(
    const shared_ptr<ObserverFactory>& factory) {
  observer_ = !factory ? nullptr : factory->getObserver(*this);
}

}}} // apache::thrift::concurrency
