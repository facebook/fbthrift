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
namespace concurrency {} // namespace concurrency
} // namespace thrift
} // namespace apache
