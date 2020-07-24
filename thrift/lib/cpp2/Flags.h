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

#include <folly/CPortability.h>
#include <folly/Indestructible.h>
#include <folly/Optional.h>
#include <folly/experimental/observer/Observer.h>
#include <folly/synchronization/CallOnce.h>

namespace apache {
namespace thrift {
namespace detail {

class FlagsBackend {
 public:
  virtual ~FlagsBackend() = default;

  virtual folly::observer::Observer<folly::Optional<bool>> getFlagObserverBool(
      folly::StringPiece name) = 0;
  virtual folly::observer::Observer<folly::Optional<int64_t>>
  getFlagObserverInt64(folly::StringPiece name) = 0;
};

#if FOLLY_HAVE_WEAK_SYMBOLS
FOLLY_ATTR_WEAK std::unique_ptr<FlagsBackend> createFlagsBackend();
#else
constexpr std::unique_ptr<FlagsBackend> (*createFlagsBackend)() = nullptr;
#endif

FlagsBackend& getFlagsBackend();

template <typename T>
folly::observer::Observer<folly::Optional<T>> getFlagObserver(
    folly::StringPiece name);

template <>
inline folly::observer::Observer<folly::Optional<bool>> getFlagObserver<bool>(
    folly::StringPiece name) {
  return getFlagsBackend().getFlagObserverBool(name);
}

template <>
inline folly::observer::Observer<folly::Optional<int64_t>>
getFlagObserver<int64_t>(folly::StringPiece name) {
  return getFlagsBackend().getFlagObserverInt64(name);
}

template <typename T>
class FlagWrapper {
 public:
  FlagWrapper(folly::StringPiece name, T defaultValue)
      : name_(name), defaultValue_(defaultValue) {}

  T get() {
    init();
    return snapshot_.load(std::memory_order_relaxed);
  }

  folly::observer::Observer<T> observe() {
    init();
    return *observer_;
  }

 private:
  void init() {
    if (UNLIKELY(!folly::test_once(initFlag_))) {
      initSlow();
    }
  }

  FOLLY_NOINLINE void initSlow() {
    folly::call_once(initFlag_, [&] {
      observer_ = folly::observer::makeValueObserver(
          [overrideObserver = getFlagObserver<T>(name_),
           defaultValue = defaultValue_] {
            auto overrideSnapshot = overrideObserver.getSnapshot();
            if (*overrideSnapshot) {
              return **overrideSnapshot;
            }
            return defaultValue;
          });
      snapshotUpdateCallback_ =
          observer_->addCallback([&](auto snapshot) { snapshot_ = *snapshot; });
    });
  }

  folly::once_flag initFlag_;
  std::atomic<T> snapshot_{};
  folly::Optional<folly::observer::Observer<T>> observer_;
  folly::observer::CallbackHandle snapshotUpdateCallback_;

  folly::StringPiece name_;
  const T defaultValue_;
};

} // namespace detail

#define THRIFT_FLAG_DEFINE(_name, _type, _default)                             \
  apache::thrift::detail::FlagWrapper<_type>& THRIFT_FLAG_WRAPPER__##_name() { \
    static constexpr folly::StringPiece flagName = #_name;                     \
    static folly::Indestructible<apache::thrift::detail::FlagWrapper<_type>>   \
        flagWrapper(flagName, _default);                                       \
    return *flagWrapper;                                                       \
  }                                                                            \
  /* This is here just to force a semicolon */                                 \
  apache::thrift::detail::FlagWrapper<_type>& THRIFT_FLAG_WRAPPER__##_name()

#define THRIFT_FLAG_DECLARE(_name, _type) \
  apache::thrift::detail::FlagWrapper<_type>& THRIFT_FLAG_WRAPPER__##_name()

#define THRIFT_FLAG_DEFINE_int64(_name, _default) \
  THRIFT_FLAG_DEFINE(_name, int64_t, _default)

#define THRIFT_FLAG_DEFINE_bool(_name, _default) \
  THRIFT_FLAG_DEFINE(_name, bool, _default)

#define THRIFT_FLAG_DECLARE_int64(_name) THRIFT_FLAG_DECLARE(_name, int64_t)

#define THRIFT_FLAG_DECLARE_bool(_name) THRIFT_FLAG_DECLARE(_name, bool)

#define THRIFT_FLAG(_name) THRIFT_FLAG_WRAPPER__##_name().get()

#define THRIFT_FLAG_OBSERVE(_name) THRIFT_FLAG_WRAPPER__##_name().observe()

} // namespace thrift
} // namespace apache
