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

#pragma once

#include <map>
#include <memory>
#include <scoped_allocator>
#include <set>
#include <string>
#include <vector>

#include "folly/sorted_vector_types.h"

template <class T>
struct ThrowingAllocator : private std::allocator<T> {
  using value_type = T;

  ThrowingAllocator() = default;
  ThrowingAllocator(const ThrowingAllocator&) noexcept = default;
  ThrowingAllocator& operator=(const ThrowingAllocator&) noexcept = default;
  template <class U>
  explicit ThrowingAllocator(const ThrowingAllocator<U>&) noexcept {}
  ~ThrowingAllocator() = default;

  T* allocate(size_t) {
    throw std::bad_alloc();
  }

  void deallocate(T*, size_t) {}

  template <class U>
  friend bool operator==(
      ThrowingAllocator<T> const&,
      ThrowingAllocator<U> const&) noexcept {
    return true;
  }

  template <class U>
  friend bool operator!=(
      ThrowingAllocator<T> const&,
      ThrowingAllocator<U> const&) noexcept {
    return false;
  }
};

using ScopedThrowingAlloc =
    std::scoped_allocator_adaptor<ThrowingAllocator<char>>;

template <class T>
using ThrowingVector = std::vector<T, ScopedThrowingAlloc>;

template <class T>
using ThrowingSet = std::set<T, std::less<T>, ScopedThrowingAlloc>;

template <class K, class V>
using ThrowingMap = std::map<K, V, std::less<K>, ScopedThrowingAlloc>;

using ThrowingString =
    std::basic_string<char, std::char_traits<char>, ScopedThrowingAlloc>;

template <class T>
struct StatefulAlloc : private std::allocator<T> {
  using value_type = T;

  StatefulAlloc() = default;
  StatefulAlloc(const StatefulAlloc&) = default;
  StatefulAlloc& operator=(const StatefulAlloc&) noexcept = default;
  explicit StatefulAlloc(int state) : state_(state) {}
  template <class U>
  explicit StatefulAlloc(const StatefulAlloc<U>& other) noexcept
      : state_(other.state_) {}

  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;

  int state_ = 0;

  T* allocate(size_t size) {
    return std::allocator<T>::allocate(size);
  }

  void deallocate(T* p, size_t size) {
    std::allocator<T>::deallocate(p, size);
  }

  template <class U>
  friend bool operator==(
      StatefulAlloc<T> const& a,
      StatefulAlloc<U> const& b) noexcept {
    return a.state_ == b.state_;
  }

  template <class U>
  friend bool operator!=(
      StatefulAlloc<T> const& a,
      StatefulAlloc<U> const& b) noexcept {
    return a.state_ != b.state_;
  }
};

using ScopedStatefulAlloc = std::scoped_allocator_adaptor<StatefulAlloc<char>>;

template <class T>
using StatefulAllocVector = std::vector<T, ScopedStatefulAlloc>;

template <class T>
using StatefulAllocSet = std::set<T, std::less<T>, ScopedStatefulAlloc>;

template <class K, class V>
using StatefulAllocMap = std::map<K, V, std::less<K>, ScopedStatefulAlloc>;

template <class T>
using StatefulAllocSortedVectorSet =
    folly::sorted_vector_set<T, std::less<T>, ScopedStatefulAlloc>;

template <class K, class V>
using StatefulAllocSortedVectorMap =
    folly::sorted_vector_map<K, V, std::less<K>, ScopedStatefulAlloc>;
