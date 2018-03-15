/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <folly/ExceptionWrapper.h>
#include <folly/Function.h>
#include <folly/executors/SequencedExecutor.h>

namespace apache {
namespace thrift {

class SubscriptionIf {
 public:
  virtual ~SubscriptionIf() = default;

  virtual void request(int64_t n) = 0;
  virtual void cancel() = 0;
};

template <typename T>
class SubscriberIf {
 public:
  virtual ~SubscriberIf() = default;

  virtual void onSubscribe(std::unique_ptr<SubscriptionIf>) = 0;
  virtual void onNext(T&&) = 0;
  virtual void onComplete() = 0;
  virtual void onError(folly::exception_wrapper) = 0;
};

namespace detail {
class ValueIf {
 public:
  virtual ~ValueIf() = default;
};

template <typename T>
struct Value : public ValueIf {
  explicit Value(T&& value_) : value(std::move(value_)) {}
  T value;
};

class StreamImplIf {
 public:
  using Value = std::unique_ptr<ValueIf>;

  virtual ~StreamImplIf() = default;

  virtual std::unique_ptr<StreamImplIf> map(folly::Function<Value(Value)>) = 0;
  virtual void subscribe(std::unique_ptr<SubscriberIf<Value>>) = 0;
  virtual void subscribeVia(folly::SequencedExecutor*) = 0;
  virtual void observeVia(folly::SequencedExecutor*) = 0;
};
} // namespace detail

template <typename T>
class SemiStream;

template <typename T>
class Stream {
 public:
  Stream() {}

  static Stream create(
      std::unique_ptr<detail::StreamImplIf> impl,
      folly::SequencedExecutor* executor) {
    Stream stream(std::move(impl), executor);
    stream.impl_->observeVia(executor);
    return stream;
  }

  operator bool() const {
    return (bool)impl_;
  }

  folly::SequencedExecutor* getExecutor() const {
    return executor_;
  }

  template <typename U>
  Stream<U> map(folly::Function<U(T&&)>) &&;

  void subscribe(std::unique_ptr<SubscriberIf<T>>) &&;

 private:
  template <typename U>
  friend class Stream;
  friend class SemiStream<T>;

  Stream(
      std::unique_ptr<detail::StreamImplIf> impl,
      folly::SequencedExecutor* executor)
      : impl_(std::move(impl)), executor_(executor) {}

  std::unique_ptr<detail::StreamImplIf> impl_;
  folly::SequencedExecutor* executor_{nullptr};
};

template <typename Response, typename StreamElement>
struct ResponseAndStream {
  using ResponseType = Response;
  using StreamElementType = StreamElement;

  Response response;
  Stream<StreamElement> stream;
};
} // namespace thrift
} // namespace apache

#include "thrift/lib/cpp2/async/Stream-inl.h"
