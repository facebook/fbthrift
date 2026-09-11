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

#include <folly/Likely.h>

namespace apache::thrift::fast_thrift::mem {

// Forward declaration
struct Page;
template <typename T>
class evb_shared_ptr;

/// evb_local_ptr - HFT-style RAII handle for EventBase-local bump allocator.
///
/// This is a zero-overhead smart pointer (just a raw T*) that assumes the
/// object is always destroyed on the EventBase thread. It DCHECKs this
/// invariant in debug builds and will crash the program if violated.
///
/// Characteristics:
/// - Size: 8 bytes (just a pointer)
/// - No KeepAlive token overhead
/// - Fast destruction: just decrement outstanding count
/// - NOT safe to pass across threads
/// - Can be upgraded to evb_shared_ptr via upgrade()
///
/// Usage:
///   auto ptr = allocator.make_local<MyObj>(args);
///   // ... use on EventBase thread ...
///   // Destructor DCHECKs we're still on EventBase thread
template <typename T>
class evb_local_ptr {
 public:
  evb_local_ptr() noexcept : ptr_(nullptr) {}
  explicit evb_local_ptr(T* ptr) noexcept : ptr_(ptr) {}

  ~evb_local_ptr() { reset(); }

  // Move-only
  evb_local_ptr(evb_local_ptr&& other) noexcept : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }
  evb_local_ptr& operator=(evb_local_ptr&& other) noexcept {
    if (this != &other) {
      reset();
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
    }
    return *this;
  }

  evb_local_ptr(const evb_local_ptr&) = delete;
  evb_local_ptr& operator=(const evb_local_ptr&) = delete;

  // Accessors
  T* get() const noexcept { return ptr_; }
  T& operator*() const noexcept { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }
  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  /// Release ownership without destroying.
  /// Returns the raw pointer and sets this to nullptr.
  T* release() noexcept {
    T* tmp = ptr_;
    ptr_ = nullptr;
    return tmp;
  }

  /// Reset to nullptr, destroying the current object if any.
  void reset() noexcept;

  /// Reset to a new pointer, destroying the current object if any.
  void reset(T* ptr) noexcept {
    reset();
    ptr_ = ptr;
  }

  /// Upgrade to evb_shared_ptr - consumes this local_ptr.
  /// The returned shared_ptr can be safely passed across threads.
  evb_shared_ptr<T> upgrade() &&;

 private:
  T* ptr_;
};

} // namespace apache::thrift::fast_thrift::mem
