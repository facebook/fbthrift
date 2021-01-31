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

#include <folly/Portability.h>

namespace apache {
namespace thrift {
namespace concurrency {
class PthreadMutex;
class PthreadRWMutex;

/**
 * A simple mutex class
 *
 * @version $Id:$
 */
class Mutex {
 public:
  Mutex();

  virtual ~Mutex() {}
  virtual void lock() const;
  virtual bool trylock() const;
  virtual bool timedlock(std::chrono::milliseconds milliseconds) const;
  virtual void unlock() const;

  /**
   * Determine if the mutex is locked.
   *
   * This is intended to be used primarily as a debugging aid, and is not
   * guaranteed to be a fast operation.  For example, a common use case is to
   * assert(mutex.isLocked()) in functions that may only be called with a
   * particular mutex already locked.
   *
   * TODO: This method currently always returns false for recursive mutexes.
   * Avoid calling this method on recursive mutexes.
   */
  virtual bool isLocked() const;

  void* getUnderlyingImpl() const;

 private:
  std::shared_ptr<PthreadMutex> impl_;
};

class FOLLY_NODISCARD Guard {
 public:
  explicit Guard(
      const Mutex& value,
      std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
      : mutex_(&value) {
    if (timeout == std::chrono::milliseconds::zero()) {
      value.lock();
    } else if (timeout < std::chrono::milliseconds::zero()) {
      if (!value.trylock()) {
        mutex_ = nullptr;
      }
    } else {
      if (!value.timedlock(timeout)) {
        mutex_ = nullptr;
      }
    }
  }
  ~Guard() {
    release();
  }

  Guard(const Guard&) = delete;
  Guard& operator=(const Guard&) = delete;

  // Move constructor/assignment.
  Guard(Guard&& other) noexcept {
    *this = std::move(other);
  }
  Guard& operator=(Guard&& other) noexcept {
    if (&other != this) {
      release();
      using std::swap;
      swap(mutex_, other.mutex_);
    }
    return *this;
  }

  bool release() {
    if (!mutex_) {
      return false;
    }
    mutex_->unlock();
    mutex_ = nullptr;
    return true;
  }

  explicit operator bool() const {
    return mutex_ != nullptr;
  }

 private:
  const Mutex* mutex_ = nullptr;
};

} // namespace concurrency
} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_CONCURRENCY_MUTEX_H_
