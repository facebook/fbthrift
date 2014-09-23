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

#include <thrift/lib/cpp/concurrency/Mutex.h>

#include <thrift/lib/cpp/concurrency/Exception.h>
#include <thrift/lib/cpp/concurrency/Mutex-impl.h>
#include <thrift/lib/cpp/concurrency/ProfiledMutex.h>
#include <thrift/lib/cpp/concurrency/Util.h>

#include <assert.h>
#include <pthread.h>

using std::shared_ptr;

namespace apache { namespace thrift { namespace concurrency {

sig_atomic_t mutexProfilingSampleRate = 0;
MutexWaitCallback mutexProfilingCallback = 0;

sig_atomic_t mutexProfilingCounter = 0;

void enableMutexProfiling(int32_t profilingSampleRate,
                          MutexWaitCallback callback) {
  mutexProfilingSampleRate = profilingSampleRate;
  mutexProfilingCallback = callback;
}

int Mutex::DEFAULT_INITIALIZER = PTHREAD_MUTEX_NORMAL;
int Mutex::RECURSIVE_INITIALIZER = PTHREAD_MUTEX_RECURSIVE;

class ProfiledPthreadMutex : public ProfiledTimedMutex<PthreadMutex> {
 public:
  ProfiledPthreadMutex(int type) : ProfiledTimedMutex<PthreadMutex>(type) {}
};

class ProfiledPthreadRWMutex : public ProfiledSharedTimedMutex<PthreadRWMutex> {
};

Mutex::Mutex(int type) : impl_(std::make_shared<ProfiledPthreadMutex>(type)) {}

void* Mutex::getUnderlyingImpl() const {
  return impl_->getImpl().getUnderlyingImpl();
}

void Mutex::lock() const { impl_->lock(); }

bool Mutex::trylock() const { return impl_->try_lock(); }

bool Mutex::timedlock(int64_t ms) const {
  return impl_->try_lock_for(std::chrono::milliseconds {ms});
}

void Mutex::unlock() const { impl_->unlock(); }

bool Mutex::isLocked() const { return impl_->getImpl().isLocked(); }

ReadWriteMutex::ReadWriteMutex()
  : impl_(std::make_shared<ProfiledPthreadRWMutex>()) {}

void ReadWriteMutex::acquireRead() const { impl_->lock_shared(); }

void ReadWriteMutex::acquireWrite() const { impl_->lock(); }

bool ReadWriteMutex::timedRead(int64_t milliseconds) const {
  return impl_->try_lock_shared_for(std::chrono::milliseconds {milliseconds});
}

bool ReadWriteMutex::timedWrite(int64_t milliseconds) const {
  return impl_->try_lock_for(std::chrono::milliseconds {milliseconds});
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

bool NoStarveReadWriteMutex::timedRead(int64_t milliseconds) const
{
  if (writerWaiting_) {
    // writer is waiting, block on the writer's mutex until he's done with it
    if (!mutex_.timedlock(milliseconds)) {
      return false;
    }
    mutex_.unlock();
  }

  return ReadWriteMutex::timedRead(milliseconds);
}

bool NoStarveReadWriteMutex::timedWrite(int64_t milliseconds) const
{
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
  int ret = ReadWriteMutex::timedWrite(milliseconds);
  writerWaiting_ = false;
  mutex_.unlock();
  return ret == 0;
}

}}} // apache::thrift::concurrency
