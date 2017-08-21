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
#ifndef THRIFT_CONCURRENCY_MUTEX_H_
#define THRIFT_CONCURRENCY_MUTEX_H_ 1

#include <cstdint>
#include <chrono>
#include <memory>
#include <boost/noncopyable.hpp>

namespace apache { namespace thrift { namespace concurrency {
class ProfiledPthreadMutex;
class ProfiledPthreadRWMutex;

/**
 * A simple mutex class
 *
 * @version $Id:$
 */
class Mutex {
 public:
  // Specifying the type of the mutex with an integer. The value has
  // to be supported by the underlying implementation, currently
  // pthread_mutex. So the possible values are PTHREAD_MUTEX_NORMAL,
  // PTHREAD_MUTEX_ERRORCHECK, PTHREAD_MUTEX_RECURSIVE and
  // PTHREAD_MUTEX_DEFAULT.
  //
  // Backwards compatibility: pass DEFAULT_INITIALIZER for PTHREAD_MUTEX_NORMAL
  // or RECURSIVE_INITIALIZER for PTHREAD_MUTEX_RECURSIVE.
  explicit Mutex(int type = DEFAULT_INITIALIZER);

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

  static int DEFAULT_INITIALIZER;
  static int RECURSIVE_INITIALIZER;

 private:
  std::shared_ptr<ProfiledPthreadMutex> impl_;
};

class ReadWriteMutex {
public:
  ReadWriteMutex();
  virtual ~ReadWriteMutex() {}

  // these get the lock and block until it is done successfully
  virtual void acquireRead() const;
  virtual void acquireWrite() const;

  // these get the lock and block until it is done successfully
  // or run out of time
  virtual bool timedRead(std::chrono::milliseconds milliseconds) const;
  virtual bool timedWrite(std::chrono::milliseconds milliseconds) const;

  // these attempt to get the lock, returning false immediately if they fail
  virtual bool attemptRead() const;
  virtual bool attemptWrite() const;

  // this releases both read and write locks
  virtual void release() const;

private:
  std::shared_ptr<ProfiledPthreadRWMutex> impl_;
};

/**
 * A ReadWriteMutex that guarantees writers will not be starved by readers:
 * When a writer attempts to acquire the mutex, all new readers will be
 * blocked from acquiring the mutex until the writer has acquired and
 * released it. In some operating systems, this may already be guaranteed
 * by a regular ReadWriteMutex.
 */
class NoStarveReadWriteMutex : public ReadWriteMutex {
public:
  NoStarveReadWriteMutex();

  void acquireRead() const override;
  void acquireWrite() const override;

  // these get the lock and block until it is done successfully
  // or run out of time
  bool timedRead(std::chrono::milliseconds milliseconds) const override;
  bool timedWrite(std::chrono::milliseconds milliseconds) const override;

private:
  Mutex mutex_;
  mutable volatile bool writerWaiting_;
};

class Guard : boost::noncopyable {
 public:
  explicit Guard(const Mutex& value,
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

// Can be used as second argument to RWGuard to make code more readable
// as to whether we're doing acquireRead() or acquireWrite().
enum RWGuardType {
  RW_READ = 0,
  RW_WRITE = 1,
};


class RWGuard : boost::noncopyable {
 public:
  explicit RWGuard(const ReadWriteMutex& value, bool write = false,
                   std::chrono::milliseconds timeout =
                    std::chrono::milliseconds::zero())
      : rw_mutex_(&value) {
    bool locked = true;
    if (write) {
      if (timeout != std::chrono::milliseconds::zero()) {
        locked = rw_mutex_->timedWrite(timeout);
      } else {
        rw_mutex_->acquireWrite();
      }
    } else {
      if (timeout != std::chrono::milliseconds::zero()) {
        locked = rw_mutex_->timedRead(timeout);
      } else {
        rw_mutex_->acquireRead();
      }
    }
    if (!locked) {
      rw_mutex_ = nullptr;
    }
  }
  RWGuard(const ReadWriteMutex& value, RWGuardType type,
          std::chrono::milliseconds timeout = std::chrono::milliseconds::zero())
      : RWGuard(value, type == RW_WRITE, timeout) {
  }

  ~RWGuard() {
    release();
  }

  // Move constructor/assignment.
  RWGuard(RWGuard&& other) noexcept {
    *this = std::move(other);
  }
  RWGuard& operator=(RWGuard&& other) noexcept {
    if (&other != this) {
      release();
      using std::swap;
      swap(rw_mutex_, other.rw_mutex_);
    }
    return *this;
  }

  explicit operator bool() const {
    return rw_mutex_ != nullptr;
  }

  bool release() {
    if (rw_mutex_ == nullptr) return false;
    rw_mutex_->release();
    rw_mutex_ = nullptr;
    return true;
  }

 private:
  const ReadWriteMutex* rw_mutex_ = nullptr;
};

}}} // apache::thrift::concurrency

#endif // #ifndef THRIFT_CONCURRENCY_MUTEX_H_
