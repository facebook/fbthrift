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

#include <thrift/lib/cpp2/async/Stream.h>

namespace apache {
namespace thrift {

template <typename T>
class SemiStream {
 public:
  SemiStream() {}

  /* implicit */ SemiStream(Stream<T> stream) : impl_(std::move(stream.impl_)) {
    if (impl_) {
      impl_->subscribeVia(stream.executor_);
    }
  }

  operator bool() const {
    return (bool)impl_;
  }

  template <typename F>
  SemiStream<folly::invoke_result_t<F, T&&>> map(F&&) &&;

  Stream<T> via(folly::SequencedExecutor*) &&;

 private:
  template <typename U>
  friend class SemiStream;

  std::unique_ptr<detail::StreamImplIf> impl_;
  std::vector<
      folly::Function<detail::StreamImplIf::Value(detail::StreamImplIf::Value)>>
      mapFuncs_;
};

template <typename Response, typename StreamElement>
struct ResponseAndSemiStream {
  using ResponseType = Response;
  using StreamElementType = StreamElement;

  Response response;
  SemiStream<StreamElement> stream;
};
} // namespace thrift
} // namespace apache

#include "thrift/lib/cpp2/async/SemiStream-inl.h"
