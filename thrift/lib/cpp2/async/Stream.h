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
#include <folly/futures/Future.h>

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

  virtual std::unique_ptr<StreamImplIf> map(
      folly::Function<Value(Value)>) && = 0;
  virtual std::unique_ptr<StreamImplIf> subscribeVia(
      folly::SequencedExecutor*) && = 0;
  virtual std::unique_ptr<StreamImplIf> observeVia(
      folly::SequencedExecutor*) && = 0;

  virtual void subscribe(std::unique_ptr<SubscriberIf<Value>>) && = 0;
};
} // namespace detail

template <typename T>
class SemiStream;

class Subscription {
 public:
  Subscription(Subscription&&) = default;
  ~Subscription() {
    if (joinFuture_) {
      LOG(FATAL) << "Subscription has to be joined/detached";
    }
  }

  void detach() && {
    joinFuture_.clear();
  }

  void join() && {
    std::move(*this).futureJoin().get();
  }

  folly::Future<folly::Unit> futureJoin() && {
    auto joinFuture = std::move(*joinFuture_);
    joinFuture_.clear();
    return joinFuture;
  }

  void cancel() {
    if (auto cancel = std::exchange(cancel_, nullptr)) {
      cancel();
    }
  }

 private:
  Subscription(
      folly::Function<void()> cancel,
      folly::Future<folly::Unit> joinFuture)
      : cancel_(std::move(cancel)), joinFuture_(std::move(joinFuture)) {}

  template <typename T>
  friend class Stream;

  folly::Function<void()> cancel_;
  folly::Optional<folly::Future<folly::Unit>> joinFuture_;
};

template <typename T>
class Stream {
 public:
  static constexpr auto kNoFlowControl = std::numeric_limits<int64_t>::max();

  Stream() {}

  static Stream create(
      std::unique_ptr<detail::StreamImplIf> impl,
      folly::SequencedExecutor* executor) {
    Stream stream(std::move(*impl).observeVia(executor), executor);
    return stream;
  }

  explicit operator bool() const {
    return (bool)impl_;
  }

  folly::SequencedExecutor* getExecutor() const {
    return executor_;
  }

  template <typename F>
  Stream<folly::invoke_result_t<F, T&&>> map(F&&) &&;

  template <
      typename OnNext,
      typename = typename std::enable_if<
          folly::is_invocable<OnNext, T&&>::value>::type>
  Subscription subscribe(OnNext&& onNext, int64_t batch = kNoFlowControl) &&;

  template <
      typename OnNext,
      typename OnError,
      typename = typename std::enable_if<
          folly::is_invocable<OnNext, T&&>::value &&
          folly::is_invocable<OnError, folly::exception_wrapper>::value>::type>
  Subscription subscribe(
      OnNext&& onNext,
      OnError&& onError,
      int64_t batch = kNoFlowControl) &&;

  template <
      typename OnNext,
      typename OnError,
      typename OnComplete,
      typename = typename std::enable_if<
          folly::is_invocable<OnNext, T&&>::value &&
          folly::is_invocable<OnError, folly::exception_wrapper>::value &&
          folly::is_invocable<OnComplete>::value>::type>
  Subscription subscribe(
      OnNext&& onNext,
      OnError&& onError,
      OnComplete&& onComplete,
      int64_t batch = kNoFlowControl) &&;

  FOLLY_CREATE_MEMBER_INVOKE_TRAITS(onNextInvokeTraits, onNext);
  FOLLY_CREATE_MEMBER_INVOKE_TRAITS(onErrorInvokeTraits, onError);
  FOLLY_CREATE_MEMBER_INVOKE_TRAITS(onCompleteInvokeTraits, onComplete);

  template <
      typename Subscriber,
      typename = typename std::enable_if<
          onNextInvokeTraits::template is_invocable<Subscriber, T&&>::value &&
          onErrorInvokeTraits::template is_invocable<
              Subscriber,
              folly::exception_wrapper>::value &&
          onCompleteInvokeTraits::template is_invocable<Subscriber>::value>::
          type,
      typename = void>
  Subscription subscribe(
      Subscriber&& subscriber,
      int64_t batch = kNoFlowControl) &&;

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

#include <thrift/lib/cpp2/async/Stream-inl.h>
