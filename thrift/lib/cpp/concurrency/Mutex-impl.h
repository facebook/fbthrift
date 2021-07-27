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

#ifndef THRIFT_CONCURRENCY_MUTEXIMPL_H_
#define THRIFT_CONCURRENCY_MUTEXIMPL_H_ 1

#include <glog/logging.h>

#include <folly/portability/PThread.h>

#include <thrift/lib/cpp/concurrency/Mutex-portability.h>
#include <thrift/lib/cpp/concurrency/Util.h>

namespace apache {
namespace thrift {
namespace concurrency {

/*
 * mutex implementations
 *
 * The motivation for these mutexes is that you can use the same template for
 * plain mutex and recursive mutex behavior. Behavior is defined at runtime.
 * Mostly, this exists for backwards compatibility in the
 * apache::thrift::concurrency::Mutex class.
 */
class PthreadMutex {
 public:
  explicit PthreadMutex(int type) {
    pthread_mutexattr_t mutexattr;
    CHECK(0 == pthread_mutexattr_init(&mutexattr));
    CHECK(0 == pthread_mutexattr_settype(&mutexattr, type));
    CHECK(0 == pthread_mutex_init(&pthread_mutex_, &mutexattr));
    CHECK(0 == pthread_mutexattr_destroy(&mutexattr));
  }
  ~PthreadMutex() { CHECK(0 == pthread_mutex_destroy(&pthread_mutex_)); }

  void lock() {
    int ret = pthread_mutex_lock(&pthread_mutex_);
    CHECK(ret != EDEADLK);
  }

  bool try_lock() { return (0 == pthread_mutex_trylock(&pthread_mutex_)); }

  template <class Rep, class Period>
  bool try_lock_for(
      const std::chrono::duration<Rep, Period>& timeout_duration) {
#if defined(_POSIX_TIMEOUTS) && _POSIX_TIMEOUTS >= 200112L
    auto durationMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout_duration);
    struct timespec ts;
    Util::toTimespec(ts, Util::currentTime() + durationMs.count());
    return 0 == pthread_mutex_timedlock(&pthread_mutex_, &ts);
#else
    (void)timeout_duration;
    // Some systems, notably macOS, don't have timeouts for pthreads,
    // so we fall back to just trying once.
    return try_lock();
#endif
  }

  void unlock() {
    int ret = pthread_mutex_unlock(&pthread_mutex_);
    CHECK(ret != EPERM);
  }

 private:
  mutable pthread_mutex_t pthread_mutex_;
};

} // namespace concurrency
} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_CONCURRENCY_MUTEXIMPL_H_
