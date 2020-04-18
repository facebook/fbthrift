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

#include <atomic>
#include <mutex>
#include <type_traits>

#include <folly/Optional.h>
#include <folly/SharedMutex.h>

namespace apache {
namespace thrift {

// source of a server's attribute, precedence takes place in descending order
// (APP will override CONF). see comment on ServerAttribute to learn more
enum class AttributeSource : uint32_t {
  OVERRIDE, // when set directly in application code
  BASELINE, // e.g., may come from external configuration mechanism
};

namespace detail {

template <typename T, typename D>
class ServerAttributeBase {
 public:
  template <typename U>
  explicit ServerAttributeBase(U value) : default_(std::move(value)) {}

  template <typename U>
  void set(U value, AttributeSource source) {
    folly::SharedMutex::WriteHolder h(&lock_);
    this->choose(source) = value;
    static_cast<D*>(this)->mergeImpl();
  }

  void unset(AttributeSource source) {
    folly::SharedMutex::WriteHolder h(&lock_);
    this->choose(source).reset();
    static_cast<D*>(this)->mergeImpl();
  }

 protected:
  T& getMergedValue() {
    return fromOverride_ ? *fromOverride_
                         : fromBaseline_ ? *fromBaseline_ : default_;
  }

  folly::Optional<T>& choose(AttributeSource source) {
    if (source == AttributeSource::OVERRIDE) {
      return fromOverride_;
    } else {
      return fromBaseline_;
    }
  }

  folly::Optional<T> fromBaseline_;
  folly::Optional<T> fromOverride_;
  T default_;
  folly::SharedMutex lock_;
};

} // namespace detail

/*
 * ServerAttribute helps to keep track of who set what value on this
 * attribute. there can be two sources for now. One is from explicit
 * application override, the other may from configuration mechanism. The
 * former one will have higher precedence than the latter one.
 */

template <typename T>
class ServerAttributeAtomic final
    : public detail::ServerAttributeBase<T, ServerAttributeAtomic<T>> {
 public:
  template <typename U>
  explicit ServerAttributeAtomic(U value)
      : detail::ServerAttributeBase<T, ServerAttributeAtomic<T>>(value),
        deduced_(this->default_) {}

  T get() const {
    return deduced_.load(std::memory_order_relaxed);
  }

  void mergeImpl() {
    deduced_.store(this->getMergedValue(), std::memory_order_relaxed);
  }

 protected:
  std::atomic<T> deduced_;
};

template <typename T>
class ServerAttributeSharedMutex final
    : public detail::ServerAttributeBase<T, ServerAttributeSharedMutex<T>> {
 public:
  template <typename U>
  explicit ServerAttributeSharedMutex(U value)
      : detail::ServerAttributeBase<T, ServerAttributeSharedMutex<T>>(value),
        deduced_(this->default_) {}

  T get() const {
    folly::SharedMutex::ReadHolder h(&this->lock_);
    return deduced_;
  }

  void mergeImpl() {
    deduced_ = this->getMergedValue();
  }

 protected:
  std::reference_wrapper<T> deduced_;
};

template <typename T>
class ServerAttributeUnsafe final
    : public detail::ServerAttributeBase<T, ServerAttributeUnsafe<T>> {
 public:
  explicit ServerAttributeUnsafe(const T& value)
      : detail::ServerAttributeBase<T, ServerAttributeUnsafe<T>>(value),
        deduced_(this->default_) {}

  const T& get() const {
    return deduced_;
  }

  void mergeImpl() {
    deduced_ = this->getMergedValue();
  }

 protected:
  T deduced_;
};

template <typename T>
using ServerAttribute = std::conditional_t<
    sizeof(T) <= sizeof(std::uint64_t) && std::is_trivially_copyable<T>::value,
    ServerAttributeAtomic<T>,
    ServerAttributeSharedMutex<T>>;

} // namespace thrift
} // namespace apache
