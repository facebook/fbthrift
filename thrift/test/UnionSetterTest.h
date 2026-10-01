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
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace apache::thrift::test::union_setter {

inline bool& allocationsThrow() {
  thread_local bool armed = false;
  return armed;
}

// While one is alive, every ThrowWhenArmedAlloc allocation on this thread
// throws std::bad_alloc.
struct ScopedThrowingAllocations {
  ScopedThrowingAllocations() { allocationsThrow() = true; }
  ~ScopedThrowingAllocations() { allocationsThrow() = false; }
  ScopedThrowingAllocations(const ScopedThrowingAllocations&) = delete;
  ScopedThrowingAllocations& operator=(const ScopedThrowingAllocations&) =
      delete;
};

template <class T>
struct ThrowWhenArmedAlloc {
  using value_type = T;

  ThrowWhenArmedAlloc() = default;
  template <class U>
  explicit ThrowWhenArmedAlloc(const ThrowWhenArmedAlloc<U>&) noexcept {}

  T* allocate(std::size_t n) {
    if (allocationsThrow()) {
      throw std::bad_alloc();
    }
    return std::allocator<T>().allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept {
    std::allocator<T>().deallocate(p, n);
  }

  template <class U>
  friend bool operator==(
      const ThrowWhenArmedAlloc&, const ThrowWhenArmedAlloc<U>&) noexcept {
    return true;
  }
};

template <class T>
using ThrowWhenArmedVector = std::vector<T, ThrowWhenArmedAlloc<T>>;

using ThrowWhenArmedString =
    std::basic_string<char, std::char_traits<char>, ThrowWhenArmedAlloc<char>>;

} // namespace apache::thrift::test::union_setter
