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

using MyAlloc = std::scoped_allocator_adaptor<ThrowingAllocator<char>>;

template <class T>
using MyVector = std::vector<T, MyAlloc>;

template <class T>
using MySet = std::set<T, std::less<T>, MyAlloc>;

template <class K, class V>
using MyMap = std::map<K, V, std::less<K>, MyAlloc>;

using MyString = std::basic_string<char, std::char_traits<char>, MyAlloc>;
