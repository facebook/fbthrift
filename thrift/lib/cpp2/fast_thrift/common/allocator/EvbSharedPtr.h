/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#pragma once

#include <cstddef>
#include <utility>

#include <glog/logging.h>

#include <folly/Executor.h>
#include <folly/Likely.h>
#include <folly/io/async/EventBase.h>

namespace apache::thrift::fast_thrift::mem {

// Forward declaration
struct Page;
template <typename T>
class evb_local_ptr;

/// evb_shared_ptr - RAII handle that can be safely destroyed off EventBase.
///
/// This smart pointer holds a KeepAlive token to the EventBase, allowing it
/// to be passed across threads. When destroyed off the EventBase thread, it
/// schedules the destruction back to the EventBase.
///
/// Characteristics:
/// - Size: 16 bytes (pointer + KeepAlive)
/// - Safe to pass across threads
/// - Destruction off EB thread schedules back to EB (slower)
/// - Can be downgraded to evb_local_ptr via downgrade()
///
/// Usage:
///   auto ptr = allocator.make_shared<MyObj>(args);
///   // ... can pass to other threads ...
///   // Destructor schedules back to EventBase if needed
template <typename T>
class evb_shared_ptr {
 public:
  evb_shared_ptr() noexcept : ptr_(nullptr) {}
  evb_shared_ptr(T* ptr, folly::Executor::KeepAlive<folly::EventBase> keepAlive)
      : ptr_(ptr), keepAlive_(std::move(keepAlive)) {}

  ~evb_shared_ptr() { reset(); }

  // Move-only
  evb_shared_ptr(evb_shared_ptr&& other) noexcept
      : ptr_(other.ptr_), keepAlive_(std::move(other.keepAlive_)) {
    other.ptr_ = nullptr;
  }
  evb_shared_ptr& operator=(evb_shared_ptr&& other) noexcept {
    if (this != &other) {
      reset();
      ptr_ = other.ptr_;
      keepAlive_ = std::move(other.keepAlive_);
      other.ptr_ = nullptr;
    }
    return *this;
  }

  evb_shared_ptr(const evb_shared_ptr&) = delete;
  evb_shared_ptr& operator=(const evb_shared_ptr&) = delete;

  // Accessors
  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  /// Release ownership without destroying.
  /// Returns the raw pointer and sets this to nullptr.
  /// The KeepAlive is also released.
  T* release() noexcept {
    T* tmp = ptr_;
    ptr_ = nullptr;
    keepAlive_.reset();
    return tmp;
  }

  /// Reset to nullptr, destroying the current object if any.
  /// If off EventBase thread, schedules destruction back to EventBase.
  void reset() noexcept;

  /// Reset to a new pointer with KeepAlive, destroying current if any.
  void reset(
      T* ptr, folly::Executor::KeepAlive<folly::EventBase> keepAlive) noexcept {
    reset();
    ptr_ = ptr;
    keepAlive_ = std::move(keepAlive);
  }

  /// Downgrade to evb_local_ptr - consumes this shared_ptr.
  /// DCHECKs that we're on the EventBase thread.
  /// The returned local_ptr assumes EventBase thread affinity.
  evb_local_ptr<T> downgrade() &&;

 private:
  T* ptr_;
  folly::Executor::KeepAlive<folly::EventBase> keepAlive_;
};

} // namespace apache::thrift::fast_thrift::mem
