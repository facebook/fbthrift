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

#include <cassert>

namespace apache {
namespace thrift {
namespace detail {

template <typename T>
class SubscriberAdaptor : public SubscriberIf<std::unique_ptr<ValueIf>> {
 public:
  explicit SubscriberAdaptor(std::unique_ptr<SubscriberIf<T>> impl)
      : impl_(std::move(impl)) {}

  void onSubscribe(std::unique_ptr<SubscriptionIf> subscription) override {
    impl_->onSubscribe(std::move(subscription));
  }

  void onNext(std::unique_ptr<ValueIf>&& value) override {
    assert(dynamic_cast<detail::Value<T>*>(value.get()));
    auto* valuePtr = static_cast<detail::Value<T>*>(value.get());
    impl_->onNext(std::move(valuePtr->value));
  }

  void onComplete() override {
    impl_->onComplete();
  }

  void onError(folly::exception_wrapper e) {
    impl_->onError(std::move(e));
  }

 private:
  std::unique_ptr<SubscriberIf<T>> impl_;
};

} // namespace detail

template <typename T>
template <typename F>
Stream<folly::invoke_result_t<F, T&&>> Stream<T>::map(F&& f) && {
  using U = folly::invoke_result_t<F, T&&>;
  auto impl = std::move(impl_);
  return Stream<U>(
      impl->map(
          [f = std::forward<F>(f)](
              std::unique_ptr<detail::ValueIf> value) mutable
          -> std::unique_ptr<detail::ValueIf> {
            assert(dynamic_cast<detail::Value<T>*>(value.get()));
            auto* valuePtr = static_cast<detail::Value<T>*>(value.get());
            return std::make_unique<detail::Value<U>>(
                std::forward<F>(f)(std::move(valuePtr->value)));
          }),
      executor_);
}

template <typename T>
void Stream<T>::subscribe(std::unique_ptr<SubscriberIf<T>> subscriber) && {
  impl_->subscribeVia(executor_);
  impl_->subscribe(
      std::make_unique<detail::SubscriberAdaptor<T>>(std::move(subscriber)));
  impl_.reset();
}
} // namespace thrift
} // namespace apache
