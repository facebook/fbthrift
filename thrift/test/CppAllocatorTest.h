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
struct MaybeThrowAllocator : private std::allocator<T> {
  using value_type = T;

  MaybeThrowAllocator() = default;
  MaybeThrowAllocator(const MaybeThrowAllocator&) noexcept = default;
  MaybeThrowAllocator& operator=(const MaybeThrowAllocator&) noexcept = default;
  template <class U>
  explicit MaybeThrowAllocator(const MaybeThrowAllocator<U>& other) noexcept
      : armed(other.armed) {}
  ~MaybeThrowAllocator() = default;

  T* allocate(size_t size) {
    if (armed) {
      throw std::bad_alloc();
    }
    return std::allocator<T>::allocate(size);
  }

  void deallocate(T* mem, size_t size) {
    if (armed) {
      return;
    }
    return std::allocator<T>::deallocate(mem, size);
  }

  template <class U>
  friend bool operator==(
      MaybeThrowAllocator<T> const&,
      MaybeThrowAllocator<U> const&) noexcept {
    return true;
  }

  template <class U>
  friend bool operator!=(
      MaybeThrowAllocator<T> const&,
      MaybeThrowAllocator<U> const&) noexcept {
    return false;
  }

  bool armed = false;
};

using ScopedMaybeThrowAlloc =
    std::scoped_allocator_adaptor<MaybeThrowAllocator<char>>;

template <class T>
using MaybeThrowVector = std::vector<T, ScopedMaybeThrowAlloc>;

template <class T>
using MaybeThrowSet = std::set<T, std::less<T>, ScopedMaybeThrowAlloc>;

template <class K, class V>
using MaybeThrowMap = std::map<K, V, std::less<K>, ScopedMaybeThrowAlloc>;

using MaybeThrowString =
    std::basic_string<char, std::char_traits<char>, ScopedMaybeThrowAlloc>;

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
