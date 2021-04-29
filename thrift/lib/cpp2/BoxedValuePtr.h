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

#include <memory>

#include <folly/CPortability.h>

namespace apache {
namespace thrift {
namespace detail {

template <typename T>
class boxed_value_ptr {
 public:
  using element_type = T;

  FOLLY_ERASE boxed_value_ptr() noexcept = default;

  template <typename... Args, typename = decltype(T(std::declval<Args>()...))>
  FOLLY_ERASE boxed_value_ptr(Args&&... args)
      : value_(std::make_unique<T>(static_cast<Args&&>(args)...)) {}

  FOLLY_ERASE boxed_value_ptr(const boxed_value_ptr<T>& other)
      : value_(other.value_ ? std::make_unique<T>(*other.value_) : nullptr) {}

  FOLLY_ERASE
  boxed_value_ptr<T>& operator=(const boxed_value_ptr<T>& other) {
    if (other.value_) {
      *this = *other.value_;
    } else {
      reset();
    }

    return *this;
  }

  template <typename U>
  FOLLY_ERASE
      std::enable_if_t<std::is_assignable<T&, U&&>::value, boxed_value_ptr&>
      operator=(U&& value) {
    if (!value_) {
      value_ = std::make_unique<T>();
    }

    *value_ = static_cast<U&&>(value);
    return *this;
  }

  FOLLY_ERASE boxed_value_ptr(boxed_value_ptr<T>&& other) = default;
  FOLLY_ERASE boxed_value_ptr<T>& operator=(boxed_value_ptr<T>&& other) =
      default;

  template <typename... Args>
  FOLLY_ERASE T& emplace(Args&&... args) {
    if (value_) {
      *value_ = T(static_cast<Args&&>(args)...);
    } else {
      value_ = std::make_unique<T>(static_cast<Args&&>(args)...);
    }

    return *value_;
  }

  FOLLY_ERASE void reset() noexcept { value_ = nullptr; }

  FOLLY_ERASE T& operator*() const noexcept { return *value_; }

  FOLLY_ERASE T* operator->() const noexcept { return &*value_; }

  FOLLY_ERASE explicit operator bool() const noexcept {
    return value_ != nullptr;
  }

 private:
  friend void swap(boxed_value_ptr<T>& lhs, boxed_value_ptr<T>& rhs) noexcept {
    using std::swap;
    swap(lhs.value_, rhs.value_);
  }

  friend bool operator==(
      const boxed_value_ptr<T>& lhs, const boxed_value_ptr<T>& rhs) noexcept {
    return lhs.value_ == rhs.value_;
  }

  friend bool operator!=(
      const boxed_value_ptr<T>& lhs, const boxed_value_ptr<T>& rhs) noexcept {
    return !(lhs == rhs);
  }

  std::unique_ptr<T> value_;
};

} // namespace detail
} // namespace thrift
} // namespace apache
