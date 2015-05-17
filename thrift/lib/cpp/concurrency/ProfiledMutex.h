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

#pragma once

#include <mutex>
#include <signal.h>
#include <thrift/lib/cpp/concurrency/Util.h>
#include <utility>

namespace apache { namespace thrift { namespace concurrency {

#ifndef THRIFT_NO_CONTENTION_PROFILING

/**
 * Determines if the mutex class will attempt to
 * profile their blocking acquire methods. If this value is set to non-zero,
 * Thrift will attempt to invoke the callback once every profilingSampleRate
 * times.  However, as the sampling is not synchronized the rate is not
 * guaranteed, and could be subject to big bursts and swings.  Please ensure
 * your sampling callback is as performant as your application requires.
 *
 * The callback will get called with the wait time taken to lock the mutex in
 * usec and a (void*) that uniquely identifies the Mutex (or ReadWriteMutex)
 * being locked.
 *
 * The enableMutexProfiling() function is unsynchronized; calling this function
 * while profiling is already enabled may result in race conditions.  On
 * architectures where a pointer assignment is atomic, this is safe but there
 * is no guarantee threads will agree on a single callback within any
 * particular time period.
 */
typedef void (*MutexWaitCallback)(const void* id, int64_t waitTimeMicros);
void enableMutexProfiling(int32_t profilingSampleRate,
                          MutexWaitCallback callback);

#endif

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
    PROFILE_MUTEX_START_LOCK();
    if (impl_.try_lock()) {
      PROFILE_MUTEX_LOCKED();
      return true;
    }
    PROFILE_MUTEX_NOT_LOCKED();
    return false;
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

}}} // apache::thrift::concurrency
