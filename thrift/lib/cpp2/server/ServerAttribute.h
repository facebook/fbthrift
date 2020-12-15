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
#include <folly/experimental/observer/SimpleObservable.h>
#include <folly/synchronization/CallOnce.h>

namespace apache {
namespace thrift {

/*
 * ServerAttribute provides a mechanism for setting values which have varying
 * precedence depending on who set it. The resolved value (`.get()`)
 * prioritizes the value in the following order, falling back to the next one
 * if the value is unset:
 *   1. explicit application override
 *   2. baseline from configuration mechanism
 *   3. default provided in constructor
 */

// source of a server's attribute, precedence takes place in descending order
// (APP will override CONF). see comment on ServerAttribute to learn more
enum class AttributeSource : uint32_t {
  OVERRIDE, // when set directly in application code
  BASELINE, // e.g., may come from external configuration mechanism
};

/**
 * A thread-safe ServerAttribute which uses folly::observer internally but reads
 * are cached via `folly::observer::AtomicObserver`.
 */
template <typename T>
struct ServerAttributeAtomic;

/**
 * A thread-safe ServerAttribute which uses folly::observer internally but reads
 * are cached via `folly::observer::TLObserver`.
 */
template <typename T>
struct ServerAttributeThreadLocal;

/**
 * A thread-safe ServerAttribute of suitable type.
 */
template <typename T>
using ServerAttribute = std::conditional_t<
    sizeof(T) <= sizeof(std::uint64_t) && std::is_trivially_copyable<T>::value,
    ServerAttributeAtomic<T>,
    ServerAttributeThreadLocal<T>>;

namespace detail {

template <typename T>
struct ServerAttributeRawValues {
  T baseline_;
  T override_;

  template <typename U, typename V>
  ServerAttributeRawValues(U&& baseline, V&& override)
      : baseline_(std::forward<U>(baseline)),
        override_(std::forward<V>(override)) {}

  FOLLY_ALWAYS_INLINE T& choose(AttributeSource source) {
    return source == AttributeSource::OVERRIDE ? override_ : baseline_;
  }
};

template <typename T>
FOLLY_ALWAYS_INLINE T& mergeServerAttributeRawValues(
    folly::Optional<T>& override,
    folly::Optional<T>& baseline,
    T& defaultValue) {
  return override ? *override : baseline ? *baseline : defaultValue;
}

template <typename T>
struct ServerAttributeObservable {
  explicit ServerAttributeObservable(T defaultValue)
      : ServerAttributeObservable(
            folly::observer::makeStaticObserver<T>(std::move(defaultValue))) {}
  explicit ServerAttributeObservable(folly::observer::Observer<T> defaultValue)
      : default_(std::move(defaultValue)) {}

  T get() const {
    return **getObserver();
  }

  const folly::observer::Observer<T>& getObserver() const {
    folly::call_once(mergedObserverInit_, [&] {
      mergedObserver_ = folly::observer::makeObserver(
          [overrideObserver = rawValues_.override_.getObserver(),
           baselineObserver = rawValues_.baseline_.getObserver(),
           defaultObserver =
               std::move(default_)]() mutable -> std::shared_ptr<T> {
            folly::Optional<folly::observer::Observer<T>> override =
                **overrideObserver;
            folly::Optional<folly::observer::Observer<T>> baseline =
                **baselineObserver;
            return std::make_shared<T>(
                **apache::thrift::detail::mergeServerAttributeRawValues(
                    override, baseline, defaultObserver));
          });
    });
    return *mergedObserver_;
  }

  /**
   * Asynchronously update the observed value. The observed value will be
   * updated on a background thread.
   */
  void setAsync(folly::observer::Observer<T> value, AttributeSource source) {
    rawValues_.choose(source).setValue(folly::Optional{std::move(value)});
  }
  void setAsync(T value, AttributeSource source) {
    setAsync(folly::observer::makeStaticObserver<T>(std::move(value)), source);
  }
  /**
   * Synchronously update the observed value. This function blocks until all
   * observed values have been updated.
   */
  void set(folly::observer::Observer<T> value, AttributeSource source) {
    setAsync(std::move(value), source);
    folly::observer::waitForAllUpdates();
  }
  void set(T value, AttributeSource source) {
    set(folly::observer::makeStaticObserver<T>(std::move(value)), source);
  }

  void unsetAsync(AttributeSource source) {
    rawValues_.choose(source).setValue(folly::none);
  }
  void unset(AttributeSource source) {
    unsetAsync(source);
    folly::observer::waitForAllUpdates();
  }

 protected:
  ServerAttributeRawValues<folly::observer::SimpleObservable<
      folly::Optional<folly::observer::Observer<T>>>>
      rawValues_{folly::none, folly::none};
  folly::observer::Observer<T> default_;
  mutable folly::once_flag mergedObserverInit_;
  mutable folly::Optional<folly::observer::Observer<T>> mergedObserver_;
};

} // namespace detail

template <typename T>
struct ServerAttributeAtomic
    : private apache::thrift::detail::ServerAttributeObservable<T> {
  using apache::thrift::detail::ServerAttributeObservable<
      T>::ServerAttributeObservable;
  using apache::thrift::detail::ServerAttributeObservable<T>::set;
  using apache::thrift::detail::ServerAttributeObservable<T>::setAsync;
  using apache::thrift::detail::ServerAttributeObservable<T>::unset;
  using apache::thrift::detail::ServerAttributeObservable<T>::unsetAsync;
  using apache::thrift::detail::ServerAttributeObservable<T>::getObserver;

  T get() const {
    return *getAtomicObserver();
  }

  const folly::observer::AtomicObserver<T>& getAtomicObserver() const {
    folly::call_once(
        atomicObserverInit_, [&] { atomicObserver_.emplace(getObserver()); });
    return *atomicObserver_;
  }

 private:
  mutable folly::once_flag atomicObserverInit_;
  mutable folly::Optional<folly::observer::AtomicObserver<T>> atomicObserver_;
};

template <typename T>
struct ServerAttributeThreadLocal
    : private apache::thrift::detail::ServerAttributeObservable<T> {
  using apache::thrift::detail::ServerAttributeObservable<
      T>::ServerAttributeObservable;
  using apache::thrift::detail::ServerAttributeObservable<T>::set;
  using apache::thrift::detail::ServerAttributeObservable<T>::setAsync;
  using apache::thrift::detail::ServerAttributeObservable<T>::unset;
  using apache::thrift::detail::ServerAttributeObservable<T>::unsetAsync;
  using apache::thrift::detail::ServerAttributeObservable<T>::getObserver;

  const T& get() const {
    return **getTLObserver();
  }

  const folly::observer::TLObserver<T>& getTLObserver() const {
    folly::call_once(
        tlObserverInit_, [&] { tlObserver_.emplace(getObserver()); });
    return *tlObserver_;
  }

 private:
  mutable folly::once_flag tlObserverInit_;
  mutable folly::Optional<folly::observer::TLObserver<T>> tlObserver_;
};

} // namespace thrift
} // namespace apache
