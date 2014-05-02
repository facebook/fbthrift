/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef THRIFT_CONCURRENCY_MUTEXIMPL_H_
#define THRIFT_CONCURRENCY_MUTEXIMPL_H_ 1

#include "thrift/lib/cpp/concurrency/Mutex.h"

#include <pthread.h>
#include <signal.h>

#include "thrift/lib/cpp/concurrency/Util.h"

namespace apache { namespace thrift { namespace concurrency {

extern sig_atomic_t mutexProfilingSampleRate;
extern MutexWaitCallback mutexProfilingCallback;
extern sig_atomic_t mutexProfilingCounter;

#ifndef THRIFT_NO_CONTENTION_PROFILING
#define PROFILE_MUTEX_START_LOCK() \
    auto _lock_startTime = maybeGetProfilingStartTime();

#define PROFILE_MUTEX_NOT_LOCKED() \
  do { \
    if (_lock_startTime > 0) { \
      int64_t endTime = Util::currentTimeUsec(); \
      (*mutexProfilingCallback)(this, (endTime - _lock_startTime)); \
    } \
  } while (0)

#define PROFILE_MUTEX_LOCKED() \
  do { \
    this->profileTime_ = _lock_startTime; \
    if (this->profileTime_ > 0) { \
      this->profileTime_ = Util::currentTimeUsec() - this->profileTime_; \
    } \
  } while (0)

#define PROFILE_MUTEX_START_UNLOCK() \
  int64_t _temp_profileTime = this->profileTime_; \
  this->profileTime_ = 0;

#define PROFILE_MUTEX_UNLOCKED() \
  do { \
    if (_temp_profileTime > 0) { \
      (*mutexProfilingCallback)(this, _temp_profileTime); \
    } \
  } while (0)

inline int64_t maybeGetProfilingStartTime() {
  if (mutexProfilingSampleRate && mutexProfilingCallback) {
    // This block is unsynchronized, but should produce a reasonable sampling
    // rate on most architectures.  The main race conditions are the gap
    // between the decrement and the test, the non-atomicity of decrement, and
    // potential caching of different values at different CPUs.
    //
    // - if two decrements race, the likeliest result is that the counter
    //      decrements slowly (perhaps much more slowly) than intended.
    //
    // - many threads could potentially decrement before resetting the counter
    //      to its large value, causing each additional incoming thread to
    //      profile every call.  This situation is unlikely to persist for long
    //      as the critical gap is quite short, but profiling could be bursty.
    sig_atomic_t localValue = --mutexProfilingCounter;
    if (localValue <= 0) {
      mutexProfilingCounter = mutexProfilingSampleRate;
      return Util::currentTimeUsec();
    }
  }

  return 0;
}

#else
#  define PROFILE_MUTEX_START_LOCK()
#  define PROFILE_MUTEX_NOT_LOCKED()
#  define PROFILE_MUTEX_LOCKED()
#  define PROFILE_MUTEX_START_UNLOCK()
#  define PROFILE_MUTEX_UNLOCKED()
#endif // THRIFT_NO_CONTENTION_PROFILING

template <class T>
class ProfiledMutex {
 public:
  template <class... Args>
  explicit ProfiledMutex(Args&&... args)
    : impl_(std::forward<Args>(args)...) {}
  void lock() {
    PROFILE_MUTEX_START_LOCK();
    impl_.lock();
    PROFILE_MUTEX_LOCKED();
  }

  bool try_lock() {
    return impl_.try_lock();
  }

  void unlock() {
    PROFILE_MUTEX_START_UNLOCK();
    impl_.unlock();
    PROFILE_MUTEX_UNLOCKED();
  }

  T& getImpl() {
    return impl_;
  }

 protected:
  T impl_;
#ifndef THRIFT_NO_CONTENTION_PROFILING
  mutable int64_t profileTime_;
#endif
};

template <class T>
class ProfiledTimedMutex : public ProfiledMutex<T> {
 public:
  template <class... Args>
  explicit ProfiledTimedMutex(Args&&... args)
    : ProfiledMutex<T>(std::forward<Args>(args)...) {}

  template<class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep,Period>& timeout_duration) {
    PROFILE_MUTEX_START_LOCK();
    if (this->impl_.try_lock_for(timeout_duration)) {
      PROFILE_MUTEX_LOCKED();
      return true;
    }

    PROFILE_MUTEX_NOT_LOCKED();
    return false;
  }

  template<class Clock, class Duration>
  bool try_lock_until(
      const std::chrono::time_point<Clock,Duration>& timeout_time) {
    PROFILE_MUTEX_START_LOCK();
    if (this->impl_.try_lock_until(timeout_time)) {
      PROFILE_MUTEX_LOCKED();
      return true;
    }

    PROFILE_MUTEX_NOT_LOCKED();
    return false;
  }
};

template <class T>
class ProfiledSharedTimedMutex : public ProfiledTimedMutex<T> {
 public:
  template <class... Args>
  explicit ProfiledSharedTimedMutex(Args&&... args)
    : ProfiledTimedMutex<T>(std::forward<Args>(args)...) {}
  void lock_shared() {
    PROFILE_MUTEX_START_LOCK();
    this->impl_.lock_shared();
    PROFILE_MUTEX_NOT_LOCKED();
  }

  bool try_lock_shared() {
    return this->impl_.try_lock_shared();
  }

  template<class Rep, class Period>
  bool try_lock_shared_for(
      const std::chrono::duration<Rep,Period>& timeout_duration) {
    PROFILE_MUTEX_START_LOCK();
    bool rc = this->impl_.try_lock_shared_for(timeout_duration);
    PROFILE_MUTEX_NOT_LOCKED();
    return rc;
  }

  template<class Clock, class Duration>
  bool try_lock_shared_until(
      const std::chrono::time_point<Clock,Duration>& timeout_time) {
    PROFILE_MUTEX_START_LOCK();
    bool rc = this->impl_.try_lock_shared_for(timeout_time);
    PROFILE_MUTEX_NOT_LOCKED();
    return rc;
  }

  void unlock_shared() {
    PROFILE_MUTEX_START_UNLOCK();
    this->impl_.unlock_shared();
    PROFILE_MUTEX_UNLOCKED();
  }
};

/*
 * mutex implementations
 *
 * The motivation for these mutexes is that you can use the same template for
 * plain mutex and recursive mutex behavior. Behavior is defined at runtime.
 * Mostly, this exists for backwards compatibility in the
 * apache::thrift::concurrency::Mutex and ReadWriteMutex classes.
 */
class PthreadMutex {
 public:
  explicit PthreadMutex(int type) {
    pthread_mutexattr_t mutexattr;
    assert(0 == pthread_mutexattr_init(&mutexattr));
    assert(0 == pthread_mutexattr_settype(&mutexattr, type));
    assert(0 == pthread_mutex_init(&pthread_mutex_, &mutexattr));
    assert(0 == pthread_mutexattr_destroy(&mutexattr));
  }
  ~PthreadMutex() { assert(0 == pthread_mutex_destroy(&pthread_mutex_)); }

  void lock() { pthread_mutex_lock(&pthread_mutex_); }

  bool try_lock() { return (0 == pthread_mutex_trylock(&pthread_mutex_)); }

  template<class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep,Period>& timeout_duration) {
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        timeout_duration);
    struct timespec ts;
    Util::toTimespec(ts, Util::currentTime() + durationMs.count());
    return 0 == pthread_mutex_timedlock(&pthread_mutex_, &ts);
  }

  void unlock() { pthread_mutex_unlock(&pthread_mutex_); }

  bool isLocked() {
    // TODO: this doesn't work with recursive locks
    // We would probably need to track additional state to handle this
    // correctly for recursive locks.
    if (try_lock()) {
      unlock();
      return false;
    }
    return true;
  }

  void* getUnderlyingImpl() const { return (void*) &pthread_mutex_; }

 private:
   mutable pthread_mutex_t pthread_mutex_;
};

/**
 *  * Implementation of ReadWriteMutex class using POSIX rw lock
 *   *
 *    * @version $Id:$
 *     */
class PthreadRWMutex {
 public:
  PthreadRWMutex() {
    assert(0 == pthread_rwlock_init(&rw_lock_, nullptr));
  }

  ~PthreadRWMutex() {
    assert(0 == pthread_rwlock_destroy(&rw_lock_));
  }

  void lock() {
    int ret = pthread_rwlock_wrlock(&rw_lock_);
    assert(ret != EDEADLK);
  }

  bool try_lock() {
    return !pthread_rwlock_trywrlock(&rw_lock_);
  }

  template<class Rep, class Period>
  bool try_lock_for(const std::chrono::duration<Rep,Period>& timeout_duration) {
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        timeout_duration);
    struct timespec ts;
    Util::toTimespec(ts, Util::currentTime() + durationMs.count());
    return 0 == pthread_rwlock_timedwrlock(&rw_lock_, &ts);
  }

  void unlock() {
     pthread_rwlock_unlock(&rw_lock_);
  }

  void lock_shared() {
    int ret = pthread_rwlock_rdlock(&rw_lock_);
    assert(ret != EDEADLK);
  }

  bool try_lock_shared() {
    return !pthread_rwlock_tryrdlock(&rw_lock_);
  }

  template<class Rep, class Period>
  bool try_lock_shared_for(
      const std::chrono::duration<Rep,Period>& timeout_duration) {
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        timeout_duration);
    struct timespec ts;
    Util::toTimespec(ts, Util::currentTime() + durationMs.count());
    return 0 == pthread_rwlock_timedrdlock(&rw_lock_, &ts);
  }

  void unlock_shared() {
    unlock();
  }

 private:
  mutable pthread_rwlock_t rw_lock_;
};



}}} // apache::thrift::concurrency

#endif // #ifndef THRIFT_CONCURRENCY_MUTEXIMPL_H_
