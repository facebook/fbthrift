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
#include <thrift/lib/cpp/concurrency/Mutex.h>

#include <thrift/lib/cpp/concurrency/Exception.h>
#include <thrift/lib/cpp/concurrency/Mutex-impl.h>
#include <thrift/lib/cpp/concurrency/Util.h>

#include <folly/portability/PThread.h>

using std::shared_ptr;

namespace apache { namespace thrift { namespace concurrency {

int Mutex::DEFAULT_INITIALIZER = PTHREAD_MUTEX_NORMAL;
int Mutex::RECURSIVE_INITIALIZER = PTHREAD_MUTEX_RECURSIVE;

Mutex::Mutex(int type) : impl_(std::make_shared<PthreadMutex>(type)) {}

void* Mutex::getUnderlyingImpl() const {
  return impl_->getUnderlyingImpl();
}

void Mutex::lock() const { impl_->lock(); }

bool Mutex::trylock() const { return impl_->try_lock(); }

bool Mutex::timedlock(std::chrono::milliseconds ms) const {
  return impl_->try_lock_for(ms);
}

void Mutex::unlock() const { impl_->unlock(); }

bool Mutex::isLocked() const {
  return impl_->isLocked();
}

ReadWriteMutex::ReadWriteMutex() : impl_(std::make_shared<PthreadRWMutex>()) {}

void ReadWriteMutex::acquireRead() const { impl_->lock_shared(); }

void ReadWriteMutex::acquireWrite() const { impl_->lock(); }

bool ReadWriteMutex::timedRead(std::chrono::milliseconds milliseconds) const {
  return impl_->try_lock_shared_for(milliseconds);
}

bool ReadWriteMutex::timedWrite(std::chrono::milliseconds milliseconds) const {
  return impl_->try_lock_for(milliseconds);
}

bool ReadWriteMutex::attemptRead() const { return impl_->try_lock_shared(); }

bool ReadWriteMutex::attemptWrite() const { return impl_->try_lock(); }

void ReadWriteMutex::release() const { impl_->unlock(); }

NoStarveReadWriteMutex::NoStarveReadWriteMutex() : writerWaiting_(false) {}

void NoStarveReadWriteMutex::acquireRead() const
{
  if (writerWaiting_) {
    // writer is waiting, block on the writer's mutex until he's done with it
    mutex_.lock();
    mutex_.unlock();
  }

  ReadWriteMutex::acquireRead();
}

void NoStarveReadWriteMutex::acquireWrite() const
{
  // if we can acquire the rwlock the easy way, we're done
  if (attemptWrite()) {
    return;
  }

  // failed to get the rwlock, do it the hard way:
  // locking the mutex and setting writerWaiting will cause all new readers to
  // block on the mutex rather than on the rwlock.
  mutex_.lock();
  writerWaiting_ = true;
  ReadWriteMutex::acquireWrite();
  writerWaiting_ = false;
  mutex_.unlock();
}

bool NoStarveReadWriteMutex::timedRead(std::chrono::milliseconds milliseconds)
 const {
  if (writerWaiting_) {
    // writer is waiting, block on the writer's mutex until he's done with it
    if (!mutex_.timedlock(milliseconds)) {
      return false;
    }
    mutex_.unlock();
  }

  return ReadWriteMutex::timedRead(milliseconds);
}

bool NoStarveReadWriteMutex::timedWrite(std::chrono::milliseconds milliseconds)
 const {
  // if we can acquire the rwlock the easy way, we're done
  if (attemptWrite()) {
    return true;
  }

  // failed to get the rwlock, do it the hard way:
  // locking the mutex and setting writerWaiting will cause all new readers to
  // block on the mutex rather than on the rwlock.
  if (!mutex_.timedlock(milliseconds)) {
    return false;
  }
  writerWaiting_ = true;
  bool ret = ReadWriteMutex::timedWrite(milliseconds);
  writerWaiting_ = false;
  mutex_.unlock();
  return ret;
}

}}} // apache::thrift::concurrency
