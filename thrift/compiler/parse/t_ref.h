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

#include <cassert>
#include <cstddef>
#include <type_traits>

namespace apache {
namespace thrift {
namespace compiler {

// A const pointer to an AST node.
//
// This is needed to avoid ambiguity in the parser code gen for const pointers.
template <typename T>
class t_ref {
 public:
  constexpr t_ref() = default;
  constexpr t_ref(const t_ref&) noexcept = default;
  constexpr t_ref(std::nullptr_t) noexcept {}
  constexpr t_ref& operator=(const t_ref&) = default;

  template <typename U>
  constexpr /* implicit */ t_ref(const t_ref<U>& u) noexcept : ptr_(u.get()) {}

  // Require an explicit cast for a non-const pointer, as non-const pointers
  // likely need to be owned, so this is probably a bug.
  template <typename U, std::enable_if_t<std::is_const<U>::value, int> = 0>
  constexpr /* implicit */ t_ref(U* ptr) : ptr_(ptr) {}
  template <typename U, std::enable_if_t<!std::is_const<U>::value, int> = 0>
  constexpr explicit t_ref(U* ptr) : ptr_(ptr) {}

  constexpr const T* get() const { return ptr_; }

  constexpr const T* operator->() const {
    assert(ptr_ != nullptr);
    return ptr_;
  }
  constexpr const T& operator*() const {
    assert(ptr_ != nullptr);
    return *ptr_;
  }

  constexpr explicit operator bool() const { return ptr_ != nullptr; }
  constexpr /* implicit */ operator const T*() const { return ptr_; }

 private:
  const T* ptr_ = nullptr;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
