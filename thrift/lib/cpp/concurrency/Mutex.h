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

#ifndef THRIFT_CONCURRENCY_MUTEX_H_
#define THRIFT_CONCURRENCY_MUTEX_H_ 1

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>

#include <folly/Portability.h>

namespace apache {
namespace thrift {
namespace concurrency {
class PthreadMutex;

/**
 * A simple mutex class
 *
 * @version $Id:$
 */
class Mutex final {
 public:
  Mutex();

  void lock();
  bool try_lock();
  bool try_lock_for(std::chrono::milliseconds timeout);
  void unlock();

 private:
  std::shared_ptr<PthreadMutex> impl_;
};

class FOLLY_NODISCARD Guard {
 public:
  explicit Guard(
      Mutex& value,
      std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
      : lock_(value, std::defer_lock) {
    if (timeout == std::chrono::milliseconds::zero()) {
      lock_.lock();
    } else if (timeout < std::chrono::milliseconds::zero()) {
      (void)lock_.try_lock();
    } else {
      (void)value.try_lock_for(timeout);
    }
  }

  Guard(const Guard&) = delete;
  Guard& operator=(const Guard&) = delete;

  Guard(Guard&&) = default;
  Guard& operator=(Guard&&) = default;

  bool release() {
    bool held = lock_.owns_lock();
    if (held) {
      lock_.unlock();
    }
    return held;
  }

  explicit operator bool() const { return !!lock_; }

 private:
  std::unique_lock<Mutex> lock_;
};

} // namespace concurrency
} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_CONCURRENCY_MUTEX_H_
