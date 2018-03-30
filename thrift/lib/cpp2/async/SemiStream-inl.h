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

template <typename T>
template <typename F>
SemiStream<folly::invoke_result_t<F, T&&>> SemiStream<T>::map(F&& f) && {
  using U = folly::invoke_result_t<F, T&&>;

  SemiStream<U> result;
  result.impl_ = std::move(impl_);
  result.mapFuncs_ = std::move(mapFuncs_);
  result.mapFuncs_.push_back(
      [f = std::forward<F>(f)](std::unique_ptr<detail::ValueIf> value) mutable
      -> std::unique_ptr<detail::ValueIf> {
        assert(dynamic_cast<detail::Value<T>*>(value.get()));
        auto* valuePtr = static_cast<detail::Value<T>*>(value.get());
        return std::make_unique<detail::Value<U>>(
            std::forward<F>(f)(std::move(valuePtr->value)));
      });
  return result;
}

template <typename T>
Stream<T> SemiStream<T>::via(folly::SequencedExecutor* executor) && {
  auto impl = std::move(impl_);
  impl->observeVia(executor);
  for (auto& mapFunc : mapFuncs_) {
    impl = std::move(*impl).map(std::move(mapFunc));
  }
  return Stream<T>(std::move(impl), executor);
}
} // namespace thrift
} // namespace apache
