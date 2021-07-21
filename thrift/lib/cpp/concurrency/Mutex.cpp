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

#include <thrift/lib/cpp/concurrency/Mutex.h>

#include <folly/portability/PThread.h>

#include <thrift/lib/cpp/concurrency/Exception.h>
#include <thrift/lib/cpp/concurrency/Mutex-impl.h>
#include <thrift/lib/cpp/concurrency/Util.h>

using std::shared_ptr;

namespace apache {
namespace thrift {
namespace concurrency {

Mutex::Mutex() : impl_(std::make_shared<PthreadMutex>(PTHREAD_MUTEX_NORMAL)) {}

void Mutex::lock() const {
  impl_->lock();
}

bool Mutex::trylock() const {
  return impl_->try_lock();
}

bool Mutex::try_lock() {
  return impl_->try_lock();
}

bool Mutex::timedlock(std::chrono::milliseconds ms) const {
  return impl_->try_lock_for(ms);
}

bool Mutex::try_lock_for(std::chrono::milliseconds timeout) {
  return impl_->try_lock_for(timeout);
}

void Mutex::unlock() const {
  impl_->unlock();
}

} // namespace concurrency
} // namespace thrift
} // namespace apache
